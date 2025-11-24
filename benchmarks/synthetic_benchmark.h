#ifndef SYNTHETIC_BENCHMARK_H
#define SYNTHETIC_BENCHMARK_H

#include "../src/cache/victim_cache.h"
#include "../src/monitoring/phase_detector.h"
#include "../src/adaptive/adaptive_controller.h"
#include <cstdint>
#include <vector>
#include <string>

#define BENCHMARK_ITERATIONS 1000000
#define ADDRESS_SPACE_SIZE 1024
#define WORKING_SET_SIZE 256

class SyntheticBenchmark {
protected:
    VictimCache* victim_cache;
    PhaseDetector* phase_detector;
    AdaptiveController* adaptive_controller;
    std::string benchmark_name;
    
    uint64_t total_accesses;
    uint64_t hits;
    uint64_t misses;
    
public:
    SyntheticBenchmark(const std::string& name);
    virtual ~SyntheticBenchmark();
    
    virtual void setup();
    virtual void run() = 0;
    virtual void teardown();
    
    void print_results() const;
    double get_hit_rate() const;
};

class SequentialBenchmark : public SyntheticBenchmark {
public:
    SequentialBenchmark() : SyntheticBenchmark("Sequential Access") {}
    void run() override;
};

class RandomBenchmark : public SyntheticBenchmark {
public:
    RandomBenchmark() : SyntheticBenchmark("Random Access") {}
    void run() override;
};

class RepeatedBenchmark : public SyntheticBenchmark {
public:
    RepeatedBenchmark() : SyntheticBenchmark("Repeated Access") {}
    void run() override;
};

class StridedBenchmark : public SyntheticBenchmark {
private:
    uint32_t stride;
public:
    StridedBenchmark(uint32_t s = 16) 
        : SyntheticBenchmark("Strided Access"), stride(s) {}
    void run() override;
};

class MixedBenchmark : public SyntheticBenchmark {
public:
    MixedBenchmark() : SyntheticBenchmark("Mixed Access") {}
    void run() override;
};

class PhaseBenchmark : public SyntheticBenchmark {
public:
    PhaseBenchmark() : SyntheticBenchmark("Phase-Changing Workload") {}
    void run() override;
};

class BenchmarkSuite {
private:
    std::vector<SyntheticBenchmark*> benchmarks;
    
public:
    BenchmarkSuite();
    ~BenchmarkSuite();
    
    void add_benchmark(SyntheticBenchmark* bench);
    void run_all();
    void print_summary() const;
};

#endif

