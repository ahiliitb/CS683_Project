#include "victim_cache.h"
#include <iostream>
#include <iomanip>
#include <cstring>
#include <algorithm>

VictimCache::VictimCache(uint32_t size) 
    : current_size(size), max_size(MAX_VICTIM_SIZE), global_lru_counter(0),
      last_sequential_addr(0), sequential_count(0) {
    entries.resize(max_size);
    access_history.reserve(1000);
    reuse_counters.resize(10000, 0);
    std::cout << "[VictimCache] Initialized with " << current_size << " entries (SMART INSERTION ENABLED)" << std::endl;
}

VictimCache::~VictimCache() {
    std::cout << "[VictimCache] Final statistics:" << std::endl;
    print_stats();
}

uint32_t VictimCache::find_lru_way() {
    uint32_t lru_way = 0;
    uint64_t min_lru = UINT64_MAX;
    
    for (uint32_t i = 0; i < current_size; i++) {
        if (!entries[i].valid) {
            return i;
        }
        if (entries[i].lru_counter < min_lru) {
            min_lru = entries[i].lru_counter;
            lru_way = i;
        }
    }
    return lru_way;
}

void VictimCache::update_lru(uint32_t way) {
    entries[way].lru_counter = global_lru_counter++;
}

int32_t VictimCache::find_victim_entry(uint64_t address) {
    for (uint32_t i = 0; i < current_size; i++) {
        if (entries[i].valid && entries[i].address == address) {
            return i;
        }
    }
    return -1;
}

void VictimCache::invalidate_entry(uint32_t way) {
    entries[way].valid = false;
    entries[way].access_count = 0;
}

bool VictimCache::lookup(uint64_t address) {
    stats.total_accesses++;
    
    int32_t way = find_victim_entry(address);
    if (way >= 0) {
        stats.victim_hits++;
        entries[way].access_count++;
        update_lru(way);
        return true;
    }
    
    stats.victim_misses++;
    return false;
}
bool VictimCache::should_insert(uint64_t address, uint32_t l2_access_count) {
    if (is_streaming_access(address)) {
        stats.bypassed_insertions++;
        return false;
    }
    
    uint32_t reuse_score = predict_reuse_potential(address);
    
    if (l2_access_count >= REUSE_PREDICTION_THRESHOLD || reuse_score >= 2) {
        return true;
    }
    
    stats.bypassed_insertions++;
    return false;
}

bool VictimCache::is_streaming_access(uint64_t address) {
    if (address == last_sequential_addr + BLOCK_SIZE) {
        sequential_count++;
        if (sequential_count > BYPASS_STREAMING_THRESHOLD) {
            last_sequential_addr = address;
            return true;
        }
    } else {
        sequential_count = 0;
    }
    last_sequential_addr = address;
    return false;
}

void VictimCache::update_access_history(uint64_t address) {
    access_history.push_back(address);
    if (access_history.size() > 1000) {
        access_history.erase(access_history.begin());
    }
    
    uint32_t idx = (address / BLOCK_SIZE) % reuse_counters.size();
    reuse_counters[idx]++;
}

uint32_t VictimCache::predict_reuse_potential(uint64_t address) {
    uint32_t reuse_count = 0;
    uint32_t idx = (address / BLOCK_SIZE) % reuse_counters.size();
    
    if (reuse_counters[idx] > 0) {
        reuse_count = reuse_counters[idx];
    }
    
    for (auto it = access_history.rbegin(); 
         it != access_history.rend() && it != access_history.rbegin() + 100; ++it) {
        if (*it == address) {
            reuse_count++;
        }
    }
    
    return reuse_count;
}
void VictimCache::insert(uint64_t address, uint64_t tag, uint8_t* data) {
    insert_smart(address, tag, data, 0);
}
void VictimCache::insert_smart(uint64_t address, uint64_t tag, uint8_t* data, uint32_t access_count) {
    update_access_history(address);
    
    if (!should_insert(address, access_count)) {
        return;
    }
    
    stats.victim_insertions++;
    
    int32_t existing_way = find_victim_entry(address);
    if (existing_way >= 0) {
        update_lru(existing_way);
        entries[existing_way].access_count++;
        entries[existing_way].high_reuse_block = true;
        stats.predicted_reuses++;
        return;
    }
    
    uint32_t victim_way = find_lru_way();
    
    if (entries[victim_way].valid) {
        stats.victim_evictions++;
    }
    
    entries[victim_way].address = address;
    entries[victim_way].tag = tag;
    if (data) {
        memcpy(entries[victim_way].data, data, BLOCK_SIZE);
    }
    entries[victim_way].valid = true;
    entries[victim_way].insertion_time = global_lru_counter;
    entries[victim_way].access_count = access_count;
    entries[victim_way].reuse_distance = 0;
    entries[victim_way].high_reuse_block = (access_count >= REUSE_PREDICTION_THRESHOLD);
    update_lru(victim_way);
}

bool VictimCache::evict_lru(uint64_t& evicted_address, uint8_t* evicted_data) {
    uint32_t lru_way = find_lru_way();
    
    if (!entries[lru_way].valid) {
        return false;
    }
    
    evicted_address = entries[lru_way].address;
    if (evicted_data) {
        memcpy(evicted_data, entries[lru_way].data, BLOCK_SIZE);
    }
    
    invalidate_entry(lru_way);
    stats.victim_evictions++;
    
    return true;
}

void VictimCache::promote_to_llc(uint32_t way) {
    if (way < current_size && entries[way].valid) {
        stats.llc_promotions++;
        invalidate_entry(way);
    }
}

void VictimCache::resize(uint32_t new_size) {
    if (new_size < MIN_VICTIM_SIZE || new_size > MAX_VICTIM_SIZE) {
        std::cerr << "[VictimCache] Invalid size: " << new_size << std::endl;
        return;
    }
    
    if (new_size < current_size) {
        for (uint32_t i = new_size; i < current_size; i++) {
            if (entries[i].valid) {
                invalidate_entry(i);
                stats.victim_evictions++;
            }
        }
    }
    
    std::cout << "[VictimCache] Resized from " << current_size 
              << " to " << new_size << " entries" << std::endl;
    current_size = new_size;
}
void VictimStats::update_rates(uint32_t /* current_size */) {
    if (total_accesses > 0) {
        hit_rate = (double)victim_hits / total_accesses;
    }
    
    if (victim_insertions > 0) {
        reuse_frequency = (double)victim_hits / victim_insertions;
        avg_access_count = (double)victim_hits / victim_insertions;
    }
}

void VictimStats::record_phase() {
    hit_rate_history.push_back(hit_rate);
    occupancy_history.push_back(occupancy_rate);
    
    if (total_accesses > 0) {
        double miss_ratio = (double)victim_misses / total_accesses;
        miss_ratio_trend.push_back(miss_ratio);
    }
}

void VictimCache::update_phase_stats(uint64_t instruction_count) {
    if (instruction_count % PHASE_WINDOW == 0) {
        stats.occupancy_rate = get_occupancy();
        stats.update_rates(current_size);
        stats.record_phase();
    }
}

double VictimCache::get_occupancy() const {
    uint32_t valid_count = get_valid_entries();
    return (double)valid_count / current_size;
}

uint32_t VictimCache::get_valid_entries() const {
    uint32_t count = 0;
    for (uint32_t i = 0; i < current_size; i++) {
        if (entries[i].valid) {
            count++;
        }
    }
    return count;
}

void VictimCache::print_stats() const {
    std::cout << "\n=== Victim Cache Statistics ===" << std::endl;
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Current Size: " << current_size << " entries" << std::endl;
    std::cout << "  Valid Entries: " << get_valid_entries() << std::endl;
    std::cout << "  Occupancy: " << std::fixed << std::setprecision(2) 
              << (get_occupancy() * 100) << "%" << std::endl;
    
    std::cout << "\nAccess Statistics:" << std::endl;
    std::cout << "  Total Accesses: " << stats.total_accesses << std::endl;
    std::cout << "  Hits: " << stats.victim_hits << std::endl;
    std::cout << "  Misses: " << stats.victim_misses << std::endl;
    std::cout << "  Hit Rate: " << (stats.hit_rate * 100) << "%" << std::endl;
    
    std::cout << "\nSmart Insertion:" << std::endl;
    std::cout << "  Insertions: " << stats.victim_insertions << std::endl;
    std::cout << "  Bypassed: " << stats.bypassed_insertions << std::endl;
    std::cout << "  Predicted Reuses: " << stats.predicted_reuses << std::endl;
    if (stats.victim_insertions + stats.bypassed_insertions > 0) {
        double bypass_rate = (double)stats.bypassed_insertions / 
                            (stats.victim_insertions + stats.bypassed_insertions);
        std::cout << "  Bypass Rate: " << (bypass_rate * 100) << "%" << std::endl;
    }
    
    std::cout << "\nOperations:" << std::endl;
    std::cout << "  Evictions: " << stats.victim_evictions << std::endl;
    std::cout << "  LLC Promotions: " << stats.llc_promotions << std::endl;
    std::cout << "  Reuse Frequency: " << stats.reuse_frequency << std::endl;
    
    std::cout << "================================\n" << std::endl;
}

void VictimCache::reset_stats() {
    stats = VictimStats();
}

