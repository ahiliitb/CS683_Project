#include "cache_simulator.h"
#include "../src/cache/victim_cache.h"
#include "../src/monitoring/phase_detector.h"
#include "../src/adaptive/adaptive_controller.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <cmath>

SetAssociativeCache::SetAssociativeCache(std::string cache_name, uint32_t size, uint32_t assoc)
    : num_sets(size / assoc), associativity(assoc), global_lru(0), name(cache_name) {
    sets.resize(num_sets);
    for (auto& set : sets) {
        set.resize(associativity);
    }
}

uint32_t SetAssociativeCache::get_set_index(uint64_t address) {
    return (address / BLOCK_SIZE) % num_sets;
}

int SetAssociativeCache::find_way(uint32_t set, uint64_t tag) {
    for (uint32_t i = 0; i < associativity; i++) {
        if (sets[set][i].valid && sets[set][i].tag == tag) {
            return i;
        }
    }
    return -1;
}

uint32_t SetAssociativeCache::find_lru_way(uint32_t set) {
    uint32_t lru_way = 0;
    uint64_t min_lru = UINT64_MAX;
    
    for (uint32_t i = 0; i < associativity; i++) {
        if (!sets[set][i].valid) {
            return i;
        }
        if (sets[set][i].lru_counter < min_lru) {
            min_lru = sets[set][i].lru_counter;
            lru_way = i;
        }
    }
    return lru_way;
}

bool SetAssociativeCache::access(uint64_t address, uint64_t* evicted_address, uint32_t* evicted_access_count) {
    stats.accesses++;
    
    uint32_t set = get_set_index(address);
    uint64_t tag = address / BLOCK_SIZE;
    
    int way = find_way(set, tag);
    if (way >= 0) {
        stats.hits++;
        sets[set][way].lru_counter = global_lru++;
        sets[set][way].access_count++;
        return true;
    }
    
    stats.misses++;
    
    uint32_t victim_way = find_lru_way(set);
    
    if (sets[set][victim_way].valid) {
        stats.evictions++;
        if (evicted_address) {
            *evicted_address = sets[set][victim_way].tag * BLOCK_SIZE;
        }
        if (evicted_access_count) {
            *evicted_access_count = sets[set][victim_way].access_count;
        }
    }
    
    sets[set][victim_way].tag = tag;
    sets[set][victim_way].valid = true;
    sets[set][victim_way].lru_counter = global_lru++;
    sets[set][victim_way].access_count = 0;
    
    return false;
}

void SetAssociativeCache::insert(uint64_t address) {
    uint32_t set = get_set_index(address);
    uint64_t tag = address / BLOCK_SIZE;
    
    uint32_t victim_way = find_lru_way(set);
    
    if (sets[set][victim_way].valid) {
        stats.evictions++;
    }
    
    sets[set][victim_way].tag = tag;
    sets[set][victim_way].valid = true;
    sets[set][victim_way].lru_counter = global_lru++;
    sets[set][victim_way].access_count = 0;
}

void SetAssociativeCache::print_stats() const {
    std::cout << "\n=== " << name << " Statistics ===" << std::endl;
    std::cout << "  Accesses: " << stats.accesses << std::endl;
    std::cout << "  Hits: " << stats.hits << std::endl;
    std::cout << "  Misses: " << stats.misses << std::endl;
    std::cout << "  Hit Rate: " << std::fixed << std::setprecision(2) 
              << (stats.get_hit_rate() * 100) << "%" << std::endl;
    std::cout << "  Miss Rate: " << (stats.get_miss_rate() * 100) << "%" << std::endl;
}

void SetAssociativeCache::reset_stats() {
    stats = CacheStats();
}

CacheHierarchySimulator::CacheHierarchySimulator(bool enable_victim, bool enable_adaptive)
    : use_victim_cache(enable_victim), use_adaptive(enable_adaptive), total_instructions(0),
      instructions_since_last_adapt(0), adaptation_interval(5000) {
    
    l1_cache = new SetAssociativeCache("L1 Cache", L1_SIZE, L1_ASSOCIATIVITY);
    l2_cache = new SetAssociativeCache("L2 Cache", L2_SIZE, L2_ASSOCIATIVITY);
    
    phase_detector = nullptr;
    adaptive_controller = nullptr;
    
    if (use_victim_cache) {
        if (use_adaptive) {
            victim_cache = new VictimCache(80);
            phase_detector = new PhaseDetector();
            adaptive_controller = new AdaptiveController(
                static_cast<VictimCache*>(victim_cache),
                static_cast<PhaseDetector*>(phase_detector)
            );
            std::cout << "[Simulator] Using ADAPTIVE victim cache (64-256 entries, starts at 80) with SMART INSERTION + AGGRESSIVE GROWTH" << std::endl;
        } else {
            victim_cache = new VictimCache(128);
            std::cout << "[Simulator] Using STATIC victim cache (128 entries FIXED) with SMART INSERTION" << std::endl;
        }
    } else {
        victim_cache = nullptr;
        std::cout << "[Simulator] NO victim cache (baseline)" << std::endl;
    }
}

CacheHierarchySimulator::~CacheHierarchySimulator() {
    delete l1_cache;
    delete l2_cache;
    if (victim_cache) {
        delete static_cast<VictimCache*>(victim_cache);
    }
    if (adaptive_controller) {
        delete static_cast<AdaptiveController*>(adaptive_controller);
    }
    if (phase_detector) {
        delete static_cast<PhaseDetector*>(phase_detector);
    }
}

void CacheHierarchySimulator::access_memory(uint64_t address) {
    uint64_t evicted_address = 0;
    uint32_t evicted_access_count = 0;
    
    bool l1_hit = l1_cache->access(address, &evicted_address, &evicted_access_count);
    
    if (l1_hit) {
        return;
    }
    
    if (use_victim_cache && victim_cache) {
        VictimCache* vc = static_cast<VictimCache*>(victim_cache);
        
        if (evicted_address != 0) {
            vc->insert_smart(evicted_address, evicted_address >> 6, nullptr, evicted_access_count);
        }
        
        if (vc->lookup(address)) {
            return;
        }
    }
    
    bool l2_hit = l2_cache->access(address);
    
    if (!l2_hit) {
        memory_stats.accesses++;
        memory_stats.misses++;
    }
}

void CacheHierarchySimulator::simulate_instruction() {
    total_instructions++;
    instructions_since_last_adapt++;
    
    if (use_adaptive && instructions_since_last_adapt >= adaptation_interval) {
        check_adaptation();
    }
}

void CacheHierarchySimulator::check_adaptation() {
    if (!use_adaptive || !adaptive_controller || !phase_detector) {
        return;
    }
    
    VictimCache* vc = static_cast<VictimCache*>(victim_cache);
    PhaseDetector* pd = static_cast<PhaseDetector*>(phase_detector);
    AdaptiveController* ac = static_cast<AdaptiveController*>(adaptive_controller);
    
    uint64_t l1_misses = l1_cache->get_stats().misses;
    uint64_t l1_accesses = l1_cache->get_stats().accesses;
    pd->update(adaptation_interval, l1_accesses, l1_misses);
    
    VictimStats& stats = vc->get_stats();
    stats.update_rates(vc->get_current_size());
    stats.occupancy_rate = vc->get_occupancy();
    
    ac->update(adaptation_interval);
    
    instructions_since_last_adapt = 0;
}

void CacheHierarchySimulator::print_summary() const {
    std::cout << "\n============================================================" << std::endl;
    std::cout << "           CACHE HIERARCHY SIMULATION RESULTS              " << std::endl;
    std::cout << "============================================================" << std::endl;
    
    std::cout << "\nConfiguration:" << std::endl;
    if (!use_victim_cache) {
        std::cout << "  Mode: BASELINE (No Victim Cache)" << std::endl;
    } else if (use_adaptive) {
        std::cout << "  Mode: ADAPTIVE VICTIM CACHE (64-256 entries) with SMART INSERTION" << std::endl;
    } else {
        std::cout << "  Mode: STATIC VICTIM CACHE (128 entries) with SMART INSERTION" << std::endl;
    }
    
    l1_cache->print_stats();
    
    if (use_victim_cache && victim_cache) {
        VictimCache* vc = static_cast<VictimCache*>(victim_cache);
        vc->print_stats();
    }
    
    l2_cache->print_stats();
    
    std::cout << "\n=== Memory Access Statistics ===" << std::endl;
    std::cout << "  Total Memory Accesses: " << memory_stats.accesses << std::endl;
    std::cout << "  Total Instructions: " << total_instructions << std::endl;
    
    double mpki = total_instructions > 0 ? 
        (double)memory_stats.accesses / (total_instructions / 1000.0) : 0.0;
    std::cout << "  MPKI (Misses per 1K Instructions): " << std::fixed 
              << std::setprecision(2) << mpki << std::endl;
}

void CacheHierarchySimulator::export_results(const std::string& filename) const {
    std::ofstream out(filename);
    
    out << "Configuration: ";
    if (!use_victim_cache) {
        out << "BASELINE (No Victim Cache)\n";
    } else if (use_adaptive) {
        out << "ADAPTIVE VICTIM CACHE (64-256 entries) with SMART INSERTION\n";
    } else {
        out << "STATIC VICTIM CACHE (128 entries) with SMART INSERTION\n";
    }
    
    out << "\nL1 Cache:\n";
    out << "  Hit Rate: " << (l1_cache->get_stats().get_hit_rate() * 100) << "%\n";
    out << "  Miss Rate: " << (l1_cache->get_stats().get_miss_rate() * 100) << "%\n";
    out << "  Accesses: " << l1_cache->get_stats().accesses << "\n";
    
    if (use_victim_cache && victim_cache) {
        VictimCache* vc = static_cast<VictimCache*>(victim_cache);
        const VictimStats& vs = vc->get_stats();
        out << "\nVictim Cache:\n";
        out << "  Hit Rate: " << (vs.hit_rate * 100) << "%\n";
        out << "  Accesses: " << vs.total_accesses << "\n";
        out << "  Hits: " << vs.victim_hits << "\n";
    }
    
    out << "\nL2 Cache:\n";
    out << "  Hit Rate: " << (l2_cache->get_stats().get_hit_rate() * 100) << "%\n";
    out << "  Miss Rate: " << (l2_cache->get_stats().get_miss_rate() * 100) << "%\n";
    out << "  Accesses: " << l2_cache->get_stats().accesses << "\n";
    
    out << "\nMemory:\n";
    out << "  Accesses: " << memory_stats.accesses << "\n";
    double mpki = total_instructions > 0 ? 
        (double)memory_stats.accesses / (total_instructions / 1000.0) : 0.0;
    out << "  MPKI: " << mpki << "\n";
    
    out.close();
}

double CacheHierarchySimulator::get_l1_hit_rate() const {
    return l1_cache->get_stats().get_hit_rate();
}

double CacheHierarchySimulator::get_l2_hit_rate() const {
    return l2_cache->get_stats().get_hit_rate();
}

double CacheHierarchySimulator::get_memory_access_rate() const {
    return total_instructions > 0 ? 
        (double)memory_stats.accesses / total_instructions : 0.0;
}

std::vector<uint64_t> WorkloadGenerator::generate_memory_intensive(uint64_t count) {
    std::vector<uint64_t> addresses;
    srand(time(nullptr) + 1);
    
    uint64_t phase_length = count / 5;
    
    for (uint64_t i = 0; i < count; i++) {
        uint64_t phase = i / phase_length;
        
        if (phase == 0 || phase == 2 || phase == 4) {
            uint64_t working_set = 1024;
            uint64_t hot_set = 128;
            int choice = rand() % 100;
            
            if (choice < 40) {
                addresses.push_back((rand() % hot_set) * BLOCK_SIZE);
            } else if (choice < 80) {
                addresses.push_back((rand() % working_set) * BLOCK_SIZE);
            } else if (i > 20) {
                uint64_t lookback = rand() % std::min((uint64_t)100, i);
                addresses.push_back(addresses[i - lookback - 1]);
            } else {
                addresses.push_back(i * BLOCK_SIZE);
            }
        } else {
            uint64_t working_set = 256;
            uint64_t hot_set = 64;
            int choice = rand() % 100;
            
            if (choice < 60) {
                addresses.push_back((rand() % hot_set) * BLOCK_SIZE);
            } else {
                addresses.push_back((rand() % working_set) * BLOCK_SIZE);
            }
        }
    }
    
    return addresses;
}

std::vector<uint64_t> WorkloadGenerator::generate_compute_intensive(uint64_t count) {
    std::vector<uint64_t> addresses;
    srand(time(nullptr));
    
    uint64_t working_set_size = 64;
    uint64_t hot_set_size = 16;
    
    for (uint64_t i = 0; i < count; i++) {
        int choice = rand() % 100;
        
        if (choice < 70) {
            addresses.push_back((rand() % hot_set_size) * BLOCK_SIZE);
        } else if (choice < 95) {
            addresses.push_back((i % working_set_size) * BLOCK_SIZE);
        } else {
            addresses.push_back((working_set_size + i / 1000) * BLOCK_SIZE);
        }
    }
    
    return addresses;
}

std::vector<uint64_t> WorkloadGenerator::generate_mixed(uint64_t count) {
    std::vector<uint64_t> addresses;
    srand(time(nullptr) + 3);
    
    uint64_t phase_length = count / 6;
    
    for (uint64_t i = 0; i < count; i++) {
        uint64_t phase = i / phase_length;
        uint64_t phase_offset = i % phase_length;
        
        if (phase == 0 || phase == 3) {
            uint64_t working_set = 800;
            uint64_t hot_set = 128;
            int choice = rand() % 100;
            if (choice < 35) {
                addresses.push_back((rand() % hot_set) * BLOCK_SIZE);
            } else if (choice < 70) {
                addresses.push_back((rand() % working_set) * BLOCK_SIZE);
            } else if (phase_offset > 20) {
                uint64_t lookback = rand() % std::min((uint64_t)80, phase_offset);
                addresses.push_back(addresses[i - lookback - 1]);
            } else {
                addresses.push_back(i * BLOCK_SIZE);
            }
        } else if (phase == 1 || phase == 4) {
            uint64_t hot_set = 32;
            if (rand() % 100 < 85) {
                addresses.push_back((rand() % hot_set) * BLOCK_SIZE);
            } else {
                addresses.push_back((i % 64) * BLOCK_SIZE);
            }
        } else if (phase == 2) {
            if (rand() % 100 < 90) {
                addresses.push_back(i * BLOCK_SIZE);
            } else if (phase_offset > 10) {
                uint64_t lookback = rand() % std::min((uint64_t)15, phase_offset);
                addresses.push_back(addresses[i - lookback - 1]);
            } else {
                addresses.push_back(i * BLOCK_SIZE);
            }
        } else {
            uint64_t working_set = 256;
            int choice = rand() % 100;
            if (choice < 50 && phase_offset > 15) {
                uint64_t lookback = rand() % std::min((uint64_t)50, phase_offset);
                addresses.push_back(addresses[i - lookback - 1]);
            } else {
                addresses.push_back((rand() % working_set) * BLOCK_SIZE);
            }
        }
    }
    
    return addresses;
}

std::vector<uint64_t> WorkloadGenerator::generate_streaming(uint64_t count) {
    std::vector<uint64_t> addresses;
    
    for (uint64_t i = 0; i < count; i++) {
        addresses.push_back(i * BLOCK_SIZE);
    }
    
    return addresses;
}

std::vector<uint64_t> WorkloadGenerator::generate_random_access(uint64_t count) {
    std::vector<uint64_t> addresses;
    srand(time(nullptr));
    
    for (uint64_t i = 0; i < count; i++) {
        addresses.push_back((rand() % 8192) * BLOCK_SIZE);
    }
    
    return addresses;
}

