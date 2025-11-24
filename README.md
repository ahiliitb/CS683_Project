# Adaptive Victim Cache Simulator

CS683 Final Project - Adaptive victim cache with smart insertion policy and dynamic sizing.

## Overview

Implementation of an adaptive victim cache that dynamically adjusts its size based on workload characteristics:
- **Static Victim Cache** with LRU replacement (Checkpoint 1)
- **Smart Insertion Policy** that bypasses low-reuse blocks
- **Phase Detection** to identify workload patterns
- **Adaptive Controller** for dynamic resizing (Checkpoint 2)

## Project Structure

```
CS683_Project/
├── src/                    # Core implementation
│   ├── cache/              # Victim cache (victim_cache.cpp/.h)
│   ├── monitoring/         # Phase detection (phase_detector.cpp/.h)
│   ├── adaptive/           # Adaptive controller (adaptive_controller.cpp/.h)
│   └── main.cpp            # Simulator entry point
├── benchmarks/             # Synthetic workloads
├── config/                 # Configuration (victim_cache_config.h)
├── test/                   # Full cache hierarchy tests
├── scripts/                # Analysis tools
├── build/                  # Build artifacts (generated)
├── bin/                    # Executables (generated)
└── results/                # Test outputs (generated)
```

## Quick Start

### Build
```bash
make                        # Build simulator
make help                   # Show all targets
```

### Run Tests
```bash
make checkpoint1            # Test static victim cache
make checkpoint2            # Test adaptive victim cache
make test                   # Run all benchmarks
```

### Full Simulation
```bash
cd test/
make run                    # Run baseline, static, and adaptive configs
```

## Running Benchmarks

### Main Simulator
```bash
./bin/victim_cache_sim --checkpoint1           # Checkpoint 1 tests
./bin/victim_cache_sim --checkpoint2           # Checkpoint 2 tests
./bin/victim_cache_sim --all                   # All benchmarks
```

### Individual Benchmarks
```bash
./bin/victim_cache_sim --benchmark sequential  # Linear access
./bin/victim_cache_sim --benchmark random      # Random access
./bin/victim_cache_sim --benchmark repeated    # High locality
./bin/victim_cache_sim --benchmark strided     # Stride patterns
./bin/victim_cache_sim --benchmark mixed       # Combined patterns
./bin/victim_cache_sim --benchmark phase       # Phase-changing
```

## Key Files

### Core Components
- **`src/cache/victim_cache.cpp`** - Cache implementation with smart insertion
- **`src/monitoring/phase_detector.cpp`** - Workload phase detection
- **`src/adaptive/adaptive_controller.cpp`** - Dynamic size adaptation
- **`config/victim_cache_config.h`** - All configuration parameters

### Tests
- **`test/cache_simulator.cpp`** - Full L2/LLC/victim cache hierarchy
- **`test/run_simulation.cpp`** - Comprehensive performance tests

### Key Functions
- `VictimCache::lookup()` - Check cache for address
- `VictimCache::insert_smart()` - Smart insertion with reuse prediction
- `PhaseDetector::classify_phase()` - Identify workload phase
- `AdaptiveController::make_decision()` - Decide on size adjustment

## Configuration

Edit `config/victim_cache_config.h` to modify parameters:

```cpp
ADAPTIVE_MIN_SIZE       64      // Minimum cache size
ADAPTIVE_MAX_SIZE       256     // Maximum cache size
SIZE_STEP              32       // Adjustment increment
HIT_RATE_HIGH_THRESH   0.08     // High hit rate threshold
OCCUPANCY_HIGH_THRESH  0.80     // High occupancy threshold
ADAPT_CHECK_INTERVAL   50000    // Adaptation check frequency
```

## Results

### Output Locations
- `results/checkpoint1_results.txt` - Static cache results
- `results/checkpoint2_results.txt` - Adaptive cache results
- `test/results/REAL_RESULTS_COMPARISON.txt` - Full comparison

### View Results
```bash
cat results/checkpoint1_results.txt
cat test/results/REAL_RESULTS_COMPARISON.txt
```

## Features

**Checkpoint 1: Static Victim Cache**
- 128-entry fully-associative cache
- LRU replacement policy
- Smart insertion (bypasses streaming)
- Reuse prediction

**Checkpoint 2: Adaptive Victim Cache**
- Dynamic sizing (64-256 entries)
- Phase detection (memory/compute/mixed)
- Hybrid adaptation policy
- 8-15% performance improvement

## Performance Targets

- **Static Cache**: 78% occupancy, 2.1-15.3% hit rate
- **Adaptive Cache**: 8-15% improvement over static, 2.2% LLC miss reduction