#include "phase_detector.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>

PhaseDetector::PhaseDetector() 
    : window_start_instruction(0), global_instruction_count(0) {
    std::cout << "[PhaseDetector] Initialized with window size: " 
              << DETECTION_WINDOW << std::endl;
}

void PhaseDetector::record_instruction() {
    global_instruction_count++;
    current_metrics.instruction_count++;
}

void PhaseDetector::record_memory_access(bool is_miss) {
    current_metrics.memory_accesses++;
    if (is_miss) {
        current_metrics.cache_misses++;
    }
}

void PhaseDetector::update(uint64_t instructions, uint64_t mem_accesses, uint64_t misses) {
    current_metrics.instruction_count += instructions;
    current_metrics.memory_accesses += mem_accesses;
    current_metrics.cache_misses += misses;
    global_instruction_count += instructions;
}

WorkloadPhase PhaseDetector::classify_phase(const PhaseMetrics& metrics) {
    if (metrics.memory_accesses == 0) {
        return PHASE_COMPUTE_INTENSIVE;
    }
    
    double mem_intensity = (double)metrics.memory_accesses / 
                          (metrics.instruction_count / 1000.0);
    
    double miss_rate = (double)metrics.cache_misses / metrics.memory_accesses;
    
    const double HIGH_MEM_INTENSITY = 100.0;
    const double HIGH_MISS_RATE = 0.10;
    
    if (mem_intensity > HIGH_MEM_INTENSITY && miss_rate > HIGH_MISS_RATE) {
        return PHASE_MEMORY_INTENSIVE;
    } else if (mem_intensity < HIGH_MEM_INTENSITY / 2) {
        return PHASE_COMPUTE_INTENSIVE;
    } else {
        return PHASE_MIXED;
    }
}

double PhaseDetector::calculate_phase_similarity(const PhaseMetrics& m1, 
                                                  const PhaseMetrics& m2) {
    if (m1.memory_accesses == 0 || m2.memory_accesses == 0) {
        return 0.0;
    }
    
    double miss_rate_diff = std::abs(m1.miss_rate - m2.miss_rate);
    double intensity_diff = std::abs(m1.memory_intensity - m2.memory_intensity) / 
                           std::max(m1.memory_intensity, m2.memory_intensity);
    
    return 1.0 - (miss_rate_diff + intensity_diff) / 2.0;
}

bool PhaseDetector::detect_phase_transition() {
    if (phase_history.empty()) {
        return false;
    }
    
    const PhaseMetrics& prev = phase_history.back();
    double similarity = calculate_phase_similarity(prev, current_metrics);
    
    return similarity < (1.0 - PHASE_CHANGE_THRESHOLD);
}

bool PhaseDetector::check_phase_change() {
    if (current_metrics.instruction_count < DETECTION_WINDOW) {
        return false;
    }
    
    if (current_metrics.memory_accesses > 0) {
        current_metrics.miss_rate = (double)current_metrics.cache_misses / 
                                    current_metrics.memory_accesses;
        current_metrics.memory_intensity = (double)current_metrics.memory_accesses / 
                                           (current_metrics.instruction_count / 1000.0);
    }
    
    current_metrics.phase_type = classify_phase(current_metrics);
    current_metrics.timestamp = global_instruction_count;
    
    bool phase_changed = detect_phase_transition();
    
    phase_history.push_back(current_metrics);
    if (phase_history.size() > HISTORY_LENGTH) {
        phase_history.pop_front();
    }
    
    window_start_instruction = global_instruction_count;
    current_metrics = PhaseMetrics();
    
    return phase_changed;
}

double PhaseDetector::get_miss_rate_trend() const {
    if (phase_history.size() < 2) {
        return 0.0;
    }
    
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_xx = 0;
    size_t n = phase_history.size();
    
    for (size_t i = 0; i < n; i++) {
        double x = i;
        double y = phase_history[i].miss_rate;
        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_xx += x * x;
    }
    
    double slope = (n * sum_xy - sum_x * sum_y) / (n * sum_xx - sum_x * sum_x);
    return slope;
}

double PhaseDetector::get_memory_intensity() const {
    if (phase_history.empty()) {
        return 0.0;
    }
    return phase_history.back().memory_intensity;
}

bool PhaseDetector::is_memory_intensive() const {
    return get_current_phase() == PHASE_MEMORY_INTENSIVE;
}

void PhaseDetector::print_phase_info() const {
    std::cout << "\n=== Phase Detection Info ===" << std::endl;
    std::cout << "Current Phase: ";
    switch (current_metrics.phase_type) {
        case PHASE_MEMORY_INTENSIVE: std::cout << "MEMORY_INTENSIVE"; break;
        case PHASE_COMPUTE_INTENSIVE: std::cout << "COMPUTE_INTENSIVE"; break;
        case PHASE_MIXED: std::cout << "MIXED"; break;
        default: std::cout << "UNKNOWN"; break;
    }
    std::cout << std::endl;
    
    std::cout << "Memory Intensity: " << std::fixed << std::setprecision(2)
              << current_metrics.memory_intensity << " accesses/1K inst" << std::endl;
    std::cout << "Miss Rate: " << (current_metrics.miss_rate * 100) << "%" << std::endl;
    std::cout << "Phase History Length: " << phase_history.size() << std::endl;
    std::cout << "Miss Rate Trend: " << get_miss_rate_trend() << std::endl;
    std::cout << "============================\n" << std::endl;
}

std::vector<PhaseMetrics> PhaseDetector::get_history() const {
    return std::vector<PhaseMetrics>(phase_history.begin(), phase_history.end());
}

