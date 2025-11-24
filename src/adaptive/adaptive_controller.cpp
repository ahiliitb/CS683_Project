#include "adaptive_controller.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>

AdaptiveController::AdaptiveController(VictimCache* vc, PhaseDetector* pd)
    : victim_cache(vc), phase_detector(pd), last_adaptation_time(0), 
      instruction_count(0) {
    std::cout << "[AdaptiveController] Initialized" << std::endl;
}

void AdaptiveController::update(uint64_t instructions) {
    instruction_count += instructions;
    
    if (should_adapt()) {
        check_and_adapt();
    }
}

bool AdaptiveController::should_adapt() const {
    return (instruction_count - last_adaptation_time) >= ADAPTATION_INTERVAL;
}

AdaptiveDecision AdaptiveController::make_decision() {
    return hybrid_policy();
}

AdaptiveDecision AdaptiveController::hit_rate_based_policy() {
    VictimStats& stats = victim_cache->get_stats();
    double hit_rate = stats.hit_rate;
    double occupancy = victim_cache->get_occupancy();
    
    if (hit_rate > HIT_RATE_THRESHOLD_HIGH && occupancy > OCCUPANCY_THRESHOLD_HIGH) {
        return DECISION_INCREASE_SIZE;
    } else if (hit_rate < HIT_RATE_THRESHOLD_LOW && occupancy < OCCUPANCY_THRESHOLD_LOW) {
        return DECISION_DECREASE_SIZE;
    }
    
    return DECISION_MAINTAIN_SIZE;
}

AdaptiveDecision AdaptiveController::occupancy_based_policy() {
    double occupancy = victim_cache->get_occupancy();
    VictimStats& stats = victim_cache->get_stats();
    
    if (occupancy > OCCUPANCY_THRESHOLD_HIGH) {
        if (stats.hit_rate > 0.05 || stats.reuse_frequency > 0.1) {
            return DECISION_INCREASE_SIZE;
        }
    } else if (occupancy < OCCUPANCY_THRESHOLD_LOW) {
        return DECISION_DECREASE_SIZE;
    }
    
    return DECISION_MAINTAIN_SIZE;
}

AdaptiveDecision AdaptiveController::phase_aware_policy() {
    WorkloadPhase phase = phase_detector->get_current_phase();
    VictimStats& stats = victim_cache->get_stats();
    double hit_rate = stats.hit_rate;
    
    switch (phase) {
        case PHASE_MEMORY_INTENSIVE:
            if (hit_rate > HIT_RATE_THRESHOLD_LOW) {
                return DECISION_INCREASE_SIZE;
            }
            break;
            
        case PHASE_COMPUTE_INTENSIVE:
            return DECISION_DECREASE_SIZE;
            
        case PHASE_MIXED:
            return occupancy_based_policy();
            
        default:
            break;
    }
    
    return DECISION_MAINTAIN_SIZE;
}

AdaptiveDecision AdaptiveController::hybrid_policy() {
    AdaptiveDecision d1 = hit_rate_based_policy();
    AdaptiveDecision d2 = occupancy_based_policy();
    AdaptiveDecision d3 = phase_aware_policy();
    
    int increase_votes = 0, decrease_votes = 0;
    
    if (d1 == DECISION_INCREASE_SIZE) increase_votes++;
    else if (d1 == DECISION_DECREASE_SIZE) decrease_votes++;
    
    if (d2 == DECISION_INCREASE_SIZE) increase_votes++;
    else if (d2 == DECISION_DECREASE_SIZE) decrease_votes++;
    
    if (d3 == DECISION_INCREASE_SIZE) increase_votes++;
    else if (d3 == DECISION_DECREASE_SIZE) decrease_votes++;
    
    if (increase_votes >= 1) {
        return DECISION_INCREASE_SIZE;
    } else if (decrease_votes >= 2) {
        return DECISION_DECREASE_SIZE;
    } else {
        return DECISION_MAINTAIN_SIZE;
    }
}

void AdaptiveController::apply_decision(AdaptiveDecision decision) {
    uint32_t current_size = victim_cache->get_current_size();
    uint32_t new_size = current_size;
    
    switch (decision) {
        case DECISION_INCREASE_SIZE:
            new_size = std::min(current_size + SIZE_ADJUSTMENT_STEP, 
                               (uint32_t)MAX_VICTIM_SIZE);
            break;
            
        case DECISION_DECREASE_SIZE:
            new_size = std::max(current_size - SIZE_ADJUSTMENT_STEP, 
                               (uint32_t)MIN_VICTIM_SIZE);
            break;
            
        case DECISION_MAINTAIN_SIZE:
        case DECISION_NO_CHANGE:
        default:
            return;
    }
    
    if (new_size != current_size) {
        victim_cache->resize(new_size);
        
        AdaptationHistory record;
        record.timestamp = instruction_count;
        record.victim_size = new_size;
        record.hit_rate = victim_cache->get_stats().hit_rate;
        record.occupancy = victim_cache->get_occupancy();
        record.phase = phase_detector->get_current_phase();
        record.decision = decision;
        history.push_back(record);
        
        std::cout << "[AdaptiveController] Adapted size: " << current_size 
                  << " -> " << new_size << " (decision: " << decision << ")" << std::endl;
    }
}

void AdaptiveController::check_and_adapt() {
    AdaptiveDecision decision = make_decision();
    apply_decision(decision);
    last_adaptation_time = instruction_count;
}

void AdaptiveController::print_adaptation_history() const {
    std::cout << "\n=== Adaptation History ===" << std::endl;
    std::cout << std::setw(12) << "Timestamp" 
              << std::setw(10) << "Size"
              << std::setw(12) << "Hit Rate"
              << std::setw(12) << "Occupancy"
              << std::setw(15) << "Phase"
              << std::setw(10) << "Decision" << std::endl;
    std::cout << std::string(70, '-') << std::endl;
    
    for (const auto& record : history) {
        std::cout << std::setw(12) << record.timestamp
                  << std::setw(10) << record.victim_size
                  << std::setw(11) << std::fixed << std::setprecision(2) 
                  << (record.hit_rate * 100) << "%"
                  << std::setw(11) << (record.occupancy * 100) << "%"
                  << std::setw(15);
        
        switch (record.phase) {
            case PHASE_MEMORY_INTENSIVE: std::cout << "MEM_INTENSIVE"; break;
            case PHASE_COMPUTE_INTENSIVE: std::cout << "CPU_INTENSIVE"; break;
            case PHASE_MIXED: std::cout << "MIXED"; break;
            default: std::cout << "UNKNOWN"; break;
        }
        
        std::cout << std::setw(10);
        switch (record.decision) {
            case DECISION_INCREASE_SIZE: std::cout << "INCREASE"; break;
            case DECISION_DECREASE_SIZE: std::cout << "DECREASE"; break;
            case DECISION_MAINTAIN_SIZE: std::cout << "MAINTAIN"; break;
            default: std::cout << "NO_CHANGE"; break;
        }
        std::cout << std::endl;
    }
    std::cout << "==========================\n" << std::endl;
}

void AdaptiveController::export_results(const std::string& filename) const {
    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        std::cerr << "Failed to open " << filename << " for writing" << std::endl;
        return;
    }
    
    outfile << "timestamp,victim_size,hit_rate,occupancy,phase,decision\n";
    for (const auto& record : history) {
        outfile << record.timestamp << ","
                << record.victim_size << ","
                << record.hit_rate << ","
                << record.occupancy << ","
                << record.phase << ","
                << record.decision << "\n";
    }
    
    outfile.close();
    std::cout << "[AdaptiveController] Results exported to " << filename << std::endl;
}

