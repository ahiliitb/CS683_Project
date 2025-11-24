#ifndef VICTIM_CACHE_H
#define VICTIM_CACHE_H

#include <cstdint>
#include <vector>
#include <string>

#define DEFAULT_VICTIM_SIZE 128
#define MIN_VICTIM_SIZE 64
#define MAX_VICTIM_SIZE 256
#define BLOCK_SIZE 64
#define PHASE_WINDOW 10000

#define REUSE_PREDICTION_THRESHOLD 2
#define BYPASS_STREAMING_THRESHOLD 10
struct VictimBlock {
    uint64_t address;
    uint64_t tag;
    uint8_t data[BLOCK_SIZE];
    bool valid;
    uint64_t lru_counter;
    uint64_t insertion_time;
    uint32_t access_count;
    uint32_t reuse_distance;  // Track reuse patterns
    bool high_reuse_block;    // Predicted high reuse
    
    VictimBlock() : address(0), tag(0), valid(false), lru_counter(0), 
                    insertion_time(0), access_count(0), reuse_distance(0),
                    high_reuse_block(false) {}
};

struct VictimStats {
    uint64_t victim_hits;
    uint64_t victim_misses;
    uint64_t victim_insertions;
    uint64_t victim_evictions;
    uint64_t llc_promotions;
    uint64_t total_accesses;
    uint64_t bypassed_insertions;
    uint64_t predicted_reuses;
    
    double occupancy_rate;
    double hit_rate;
    double reuse_frequency;
    double avg_access_count;
    
    std::vector<double> miss_ratio_trend;
    std::vector<double> hit_rate_history;
    std::vector<double> occupancy_history;
    
    VictimStats() : victim_hits(0), victim_misses(0), victim_insertions(0),
                    victim_evictions(0), llc_promotions(0), total_accesses(0),
                    bypassed_insertions(0), predicted_reuses(0),
                    occupancy_rate(0.0), hit_rate(0.0), reuse_frequency(0.0),
                    avg_access_count(0.0) {}
    
    void update_rates(uint32_t current_size);
    void record_phase();
};

class VictimCache {
private:
    std::vector<VictimBlock> entries;
    uint32_t current_size;
    uint32_t max_size;
    uint64_t global_lru_counter;
    VictimStats stats;
    
    std::vector<uint64_t> access_history;
    std::vector<uint32_t> reuse_counters;
    uint64_t last_sequential_addr;
    uint32_t sequential_count;
    
    uint32_t find_lru_way();
    void update_lru(uint32_t way);
    
    int32_t find_victim_entry(uint64_t address);
    void invalidate_entry(uint32_t way);
    
    bool should_insert(uint64_t address, uint32_t l2_access_count);
    bool is_streaming_access(uint64_t address);
    void update_access_history(uint64_t address);
    uint32_t predict_reuse_potential(uint64_t address);

public:
    VictimCache(uint32_t size = DEFAULT_VICTIM_SIZE);
    ~VictimCache();
    
    bool lookup(uint64_t address);
    void insert(uint64_t address, uint64_t tag, uint8_t* data);
    void insert_smart(uint64_t address, uint64_t tag, uint8_t* data, uint32_t access_count);
    bool evict_lru(uint64_t& evicted_address, uint8_t* evicted_data);
    void promote_to_llc(uint32_t way);
    
    void resize(uint32_t new_size);
    uint32_t get_current_size() const { return current_size; }
    
    const VictimStats& get_stats() const { return stats; }
    VictimStats& get_stats() { return stats; }
    void print_stats() const;
    void reset_stats();
    void update_phase_stats(uint64_t instruction_count);
    
    double get_occupancy() const;
    uint32_t get_valid_entries() const;
};

#endif

