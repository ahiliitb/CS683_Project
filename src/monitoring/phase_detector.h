#ifndef PHASE_DETECTOR_H
#define PHASE_DETECTOR_H

#include <cstdint>
#include <vector>
#include <deque>

#define DETECTION_WINDOW 50000
#define HISTORY_LENGTH 10
#define PHASE_CHANGE_THRESHOLD 0.15

enum WorkloadPhase {
    PHASE_MEMORY_INTENSIVE,
    PHASE_COMPUTE_INTENSIVE,
    PHASE_MIXED,
    PHASE_UNKNOWN
};

struct PhaseMetrics {
    uint64_t instruction_count;
    uint64_t memory_accesses;
    uint64_t cache_misses;
    double miss_rate;
    double memory_intensity;
    WorkloadPhase phase_type;
    uint64_t timestamp;
    
    PhaseMetrics() : instruction_count(0), memory_accesses(0), cache_misses(0),
                     miss_rate(0.0), memory_intensity(0.0), 
                     phase_type(PHASE_UNKNOWN), timestamp(0) {}
};

class PhaseDetector {
private:
    std::deque<PhaseMetrics> phase_history;
    PhaseMetrics current_metrics;
    uint64_t window_start_instruction;
    uint64_t global_instruction_count;
    
    WorkloadPhase classify_phase(const PhaseMetrics& metrics);
    bool detect_phase_transition();
    double calculate_phase_similarity(const PhaseMetrics& m1, const PhaseMetrics& m2);

public:
    PhaseDetector();
    
    void record_instruction();
    void record_memory_access(bool is_miss);
    void update(uint64_t instructions, uint64_t mem_accesses, uint64_t misses);
    
    bool check_phase_change();
    WorkloadPhase get_current_phase() const { return current_metrics.phase_type; }
    const PhaseMetrics& get_current_metrics() const { return current_metrics; }
    
    double get_miss_rate_trend() const;
    double get_memory_intensity() const;
    bool is_memory_intensive() const;
    
    void print_phase_info() const;
    std::vector<PhaseMetrics> get_history() const;
};

#endif

