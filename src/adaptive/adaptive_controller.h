#ifndef ADAPTIVE_CONTROLLER_H
#define ADAPTIVE_CONTROLLER_H

#include "../cache/victim_cache.h"
#include "../monitoring/phase_detector.h"
#include <cstdint>
#include <string>

#define ADAPTATION_INTERVAL 50000
#define SIZE_ADJUSTMENT_STEP 32
#define HIT_RATE_THRESHOLD_HIGH 0.08
#define HIT_RATE_THRESHOLD_LOW 0.02
#define OCCUPANCY_THRESHOLD_HIGH 0.75
#define OCCUPANCY_THRESHOLD_LOW 0.30

enum AdaptiveDecision {
    DECISION_INCREASE_SIZE,
    DECISION_DECREASE_SIZE,
    DECISION_MAINTAIN_SIZE,
    DECISION_NO_CHANGE
};

struct AdaptationHistory {
    uint64_t timestamp;
    uint32_t victim_size;
    double hit_rate;
    double occupancy;
    WorkloadPhase phase;
    AdaptiveDecision decision;
    
    AdaptationHistory() : timestamp(0), victim_size(0), hit_rate(0.0),
                         occupancy(0.0), phase(PHASE_UNKNOWN), 
                         decision(DECISION_NO_CHANGE) {}
};

class AdaptiveController {
private:
    VictimCache* victim_cache;
    PhaseDetector* phase_detector;
    
    uint64_t last_adaptation_time;
    uint64_t instruction_count;
    std::vector<AdaptationHistory> history;
    
    AdaptiveDecision make_decision();
    void apply_decision(AdaptiveDecision decision);
    
    AdaptiveDecision hit_rate_based_policy();
    AdaptiveDecision occupancy_based_policy();
    AdaptiveDecision phase_aware_policy();
    AdaptiveDecision hybrid_policy();
    
    bool should_adapt() const;
    uint32_t calculate_optimal_size();

public:
    AdaptiveController(VictimCache* vc, PhaseDetector* pd);
    
    void update(uint64_t instructions);
    void check_and_adapt();
    
    void set_victim_cache(VictimCache* vc) { victim_cache = vc; }
    void set_phase_detector(PhaseDetector* pd) { phase_detector = pd; }
    
    void print_adaptation_history() const;
    std::vector<AdaptationHistory> get_history() const { return history; }
    
    void export_results(const std::string& filename) const;
};

#endif

