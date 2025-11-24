#include "synthetic_benchmark.h"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <ctime>

SyntheticBenchmark::SyntheticBenchmark(const std::string& name)
    : benchmark_name(name), total_accesses(0), hits(0), misses(0) {
    victim_cache = nullptr;
    phase_detector = nullptr;
    adaptive_controller = nullptr;
}

SyntheticBenchmark::~SyntheticBenchmark() {
    teardown();
}

void SyntheticBenchmark::setup() {
    victim_cache = new VictimCache(DEFAULT_VICTIM_SIZE);
    phase_detector = new PhaseDetector();
    adaptive_controller = new AdaptiveController(victim_cache, phase_detector);
    
    total_accesses = 0;
    hits = 0;
    misses = 0;
    
    std::cout << "\n[" << benchmark_name << "] Starting benchmark..." << std::endl;
}

void SyntheticBenchmark::teardown() {
    if (victim_cache) {
        delete victim_cache;
        victim_cache = nullptr;
    }
    if (phase_detector) {
        delete phase_detector;
        phase_detector = nullptr;
    }
    if (adaptive_controller) {
        delete adaptive_controller;
        adaptive_controller = nullptr;
    }
}

void SyntheticBenchmark::print_results() const {
    std::cout << "\n=== " << benchmark_name << " Results ===" << std::endl;
    std::cout << "Total Accesses: " << total_accesses << std::endl;
    std::cout << "Hits: " << hits << std::endl;
    std::cout << "Misses: " << misses << std::endl;
    std::cout << "Hit Rate: " << std::fixed << std::setprecision(2) 
              << (get_hit_rate() * 100) << "%" << std::endl;
    std::cout << "=================================\n" << std::endl;
}

double SyntheticBenchmark::get_hit_rate() const {
    if (total_accesses == 0) return 0.0;
    return (double)hits / total_accesses;
}

void SequentialBenchmark::run() {
    setup();
    
    for (uint64_t i = 0; i < BENCHMARK_ITERATIONS; i++) {
        uint64_t address = (i % ADDRESS_SPACE_SIZE) * BLOCK_SIZE;
        
        if (victim_cache->lookup(address)) {
        } else {
            victim_cache->insert(address, address >> 6, nullptr);
        }
        
        phase_detector->record_instruction();
        phase_detector->record_memory_access(false);
    }
    
    // Get stats from victim cache
    VictimStats& stats = victim_cache->get_stats();
    total_accesses = stats.total_accesses;
    hits = stats.victim_hits;
    misses = stats.victim_misses;
    
    print_results();
    victim_cache->print_stats();
}

void RandomBenchmark::run() {
    setup();
    srand(time(nullptr));
    
    for (uint64_t i = 0; i < BENCHMARK_ITERATIONS; i++) {
        uint64_t address = (rand() % ADDRESS_SPACE_SIZE) * BLOCK_SIZE;
        
        if (victim_cache->lookup(address)) {
        } else {
            victim_cache->insert(address, address >> 6, nullptr);
        }
        
        phase_detector->record_instruction();
        phase_detector->record_memory_access(false);
    }
    
    // Get stats from victim cache
    VictimStats& stats = victim_cache->get_stats();
    total_accesses = stats.total_accesses;
    hits = stats.victim_hits;
    misses = stats.victim_misses;
    
    print_results();
    victim_cache->print_stats();
}

void RepeatedBenchmark::run() {
    setup();
    
    std::vector<uint64_t> working_set;
    for (uint32_t i = 0; i < WORKING_SET_SIZE / 4; i++) {
        working_set.push_back(i * BLOCK_SIZE);
    }
    
    for (uint64_t i = 0; i < BENCHMARK_ITERATIONS; i++) {
        uint64_t address = working_set[i % working_set.size()];
        
        if (victim_cache->lookup(address)) {
        } else {
            victim_cache->insert(address, address >> 6, nullptr);
        }
        
        phase_detector->record_instruction();
        phase_detector->record_memory_access(false);
    }
    
    // Get stats from victim cache
    VictimStats& stats = victim_cache->get_stats();
    total_accesses = stats.total_accesses;
    hits = stats.victim_hits;
    misses = stats.victim_misses;
    
    print_results();
    victim_cache->print_stats();
}

void StridedBenchmark::run() {
    setup();
    
    for (uint64_t i = 0; i < BENCHMARK_ITERATIONS; i++) {
        uint64_t address = ((i * stride) % ADDRESS_SPACE_SIZE) * BLOCK_SIZE;
        
        if (victim_cache->lookup(address)) {
        } else {
            victim_cache->insert(address, address >> 6, nullptr);
        }
        
        phase_detector->record_instruction();
        phase_detector->record_memory_access(false);
    }
    
    // Get stats from victim cache
    VictimStats& stats = victim_cache->get_stats();
    total_accesses = stats.total_accesses;
    hits = stats.victim_hits;
    misses = stats.victim_misses;
    
    print_results();
    victim_cache->print_stats();
}

void MixedBenchmark::run() {
    setup();
    srand(time(nullptr));
    
    for (uint64_t i = 0; i < BENCHMARK_ITERATIONS; i++) {
        uint64_t address;
        
        if (rand() % 100 < 70) {
            address = (i % ADDRESS_SPACE_SIZE) * BLOCK_SIZE;
        } else {
            address = (rand() % ADDRESS_SPACE_SIZE) * BLOCK_SIZE;
        }
        
        if (victim_cache->lookup(address)) {
        } else {
            victim_cache->insert(address, address >> 6, nullptr);
        }
        
        phase_detector->record_instruction();
        phase_detector->record_memory_access(false);
    }
    
    // Get stats from victim cache
    VictimStats& stats = victim_cache->get_stats();
    total_accesses = stats.total_accesses;
    hits = stats.victim_hits;
    misses = stats.victim_misses;
    
    print_results();
    victim_cache->print_stats();
}

void PhaseBenchmark::run() {
    setup();
    srand(time(nullptr));
    
    uint64_t phase_length = BENCHMARK_ITERATIONS / 4;
    
    for (uint64_t i = 0; i < BENCHMARK_ITERATIONS; i++) {
        uint64_t address;
        uint64_t phase = i / phase_length;
        
        switch (phase) {
            case 0:
                address = (i % ADDRESS_SPACE_SIZE) * BLOCK_SIZE;
                break;
            case 1:
                address = (rand() % ADDRESS_SPACE_SIZE) * BLOCK_SIZE;
                break;
            case 2:
                address = ((i % 64) * BLOCK_SIZE);
                break;
            case 3:
                address = ((i * 8) % ADDRESS_SPACE_SIZE) * BLOCK_SIZE;
                break;
            default:
                address = (rand() % ADDRESS_SPACE_SIZE) * BLOCK_SIZE;
                break;
        }
        
        bool is_hit = victim_cache->lookup(address);
        if (!is_hit) {
            victim_cache->insert(address, address >> 6, nullptr);
        }
        
        phase_detector->record_instruction();
        phase_detector->record_memory_access(!is_hit);
        
        if (phase_detector->check_phase_change()) {
            std::cout << "[Phase Change Detected at " << i << " instructions]" << std::endl;
            phase_detector->print_phase_info();
        }
        
        adaptive_controller->update(1);
    }
    
    // Get stats from victim cache
    VictimStats& stats = victim_cache->get_stats();
    total_accesses = stats.total_accesses;
    hits = stats.victim_hits;
    misses = stats.victim_misses;
    
    print_results();
    victim_cache->print_stats();
    phase_detector->print_phase_info();
    adaptive_controller->print_adaptation_history();
}

BenchmarkSuite::BenchmarkSuite() {
    std::cout << "=== Benchmark Suite Initialized ===" << std::endl;
}

BenchmarkSuite::~BenchmarkSuite() {
    for (auto* bench : benchmarks) {
        delete bench;
    }
}

void BenchmarkSuite::add_benchmark(SyntheticBenchmark* bench) {
    benchmarks.push_back(bench);
}

void BenchmarkSuite::run_all() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Starting Benchmark Suite" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    for (auto* bench : benchmarks) {
        bench->run();
    }
    
    print_summary();
}

void BenchmarkSuite::print_summary() const {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Benchmark Suite Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Total Benchmarks Run: " << benchmarks.size() << std::endl;
    std::cout << "========================================\n" << std::endl;
}

