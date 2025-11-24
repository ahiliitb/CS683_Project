#ifndef CACHE_SIMULATOR_H
#define CACHE_SIMULATOR_H

#include <cstdint>
#include <vector>
#include <string>
#include <map>

#define L1_SIZE 256
#define L1_ASSOCIATIVITY 8
#define L2_SIZE 2048
#define L2_ASSOCIATIVITY 16
#define BLOCK_SIZE 64
struct CacheBlock {
    uint64_t tag;
    bool valid;
    uint64_t lru_counter;
    uint64_t access_count;
    
    CacheBlock() : tag(0), valid(false), lru_counter(0), access_count(0) {}
};

struct CacheStats {
    uint64_t accesses;
    uint64_t hits;
    uint64_t misses;
    uint64_t evictions;
    
    CacheStats() : accesses(0), hits(0), misses(0), evictions(0) {}
    
    double get_hit_rate() const {
        return accesses > 0 ? (double)hits / accesses : 0.0;
    }
    
    double get_miss_rate() const {
        return accesses > 0 ? (double)misses / accesses : 0.0;
    }
};

class SetAssociativeCache {
private:
    uint32_t num_sets;
    uint32_t associativity;
    std::vector<std::vector<CacheBlock>> sets;
    uint64_t global_lru;
    CacheStats stats;
    std::string name;
    
    uint32_t get_set_index(uint64_t address);
    uint32_t find_lru_way(uint32_t set);
    int find_way(uint32_t set, uint64_t tag);

public:
    SetAssociativeCache(std::string cache_name, uint32_t size, uint32_t assoc);
    
    bool access(uint64_t address, uint64_t* evicted_address = nullptr, uint32_t* evicted_access_count = nullptr);
    void insert(uint64_t address);
    
    const CacheStats& get_stats() const { return stats; }
    void print_stats() const;
    void reset_stats();
};

class CacheHierarchySimulator {
private:
    SetAssociativeCache* l1_cache;
    SetAssociativeCache* l2_cache;
    
    void* victim_cache;
    void* phase_detector;
    void* adaptive_controller;
    
    bool use_victim_cache;
    bool use_adaptive;
    
    CacheStats memory_stats;
    uint64_t total_instructions;
    uint64_t instructions_since_last_adapt;
    uint64_t adaptation_interval;

public:
    CacheHierarchySimulator(bool enable_victim, bool enable_adaptive);
    ~CacheHierarchySimulator();
    
    void access_memory(uint64_t address);
    void simulate_instruction();
    void check_adaptation();
    
    void print_summary() const;
    void export_results(const std::string& filename) const;
    
    double get_l1_hit_rate() const;
    double get_l2_hit_rate() const;
    double get_memory_access_rate() const;
};

class WorkloadGenerator {
public:
    static std::vector<uint64_t> generate_memory_intensive(uint64_t count);
    static std::vector<uint64_t> generate_compute_intensive(uint64_t count);
    static std::vector<uint64_t> generate_mixed(uint64_t count);
    static std::vector<uint64_t> generate_streaming(uint64_t count);
    static std::vector<uint64_t> generate_random_access(uint64_t count);
};

#endif

