#!/bin/bash

# Experiment runner script for Adaptive Victim Cache
# CS683 Final Project

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Directories
RESULTS_DIR="results"
BIN_DIR="bin"
SIMULATOR="$BIN_DIR/victim_cache_sim"

echo "========================================"
echo "Adaptive Victim Cache Experiment Runner"
echo "========================================"
echo ""

# Check if simulator exists
if [ ! -f "$SIMULATOR" ]; then
    echo -e "${RED}Error: Simulator not found at $SIMULATOR${NC}"
    echo "Please run 'make' first to build the simulator"
    exit 1
fi

# Create results directory
mkdir -p "$RESULTS_DIR"

# Function to run experiment
run_experiment() {
    local name=$1
    local args=$2
    local output=$3
    
    echo -e "${YELLOW}Running: $name${NC}"
    echo "Command: $SIMULATOR $args"
    
    if $SIMULATOR $args > "$output" 2>&1; then
        echo -e "${GREEN}✓ $name completed${NC}"
        return 0
    else
        echo -e "${RED}✗ $name failed${NC}"
        return 1
    fi
}

# Checkpoint 1 Experiments
echo ""
echo "=== CHECKPOINT 1: Static Victim Cache ==="
echo ""

run_experiment "Checkpoint 1 - All Benchmarks" \
    "--checkpoint1" \
    "$RESULTS_DIR/checkpoint1_results.txt"

# Individual benchmarks
echo ""
echo "=== Running Individual Benchmarks ==="
echo ""

for bench in sequential random repeated strided; do
    run_experiment "Benchmark: $bench" \
        "--benchmark $bench" \
        "$RESULTS_DIR/benchmark_${bench}.txt"
done

# Checkpoint 2 Experiments
echo ""
echo "=== CHECKPOINT 2: Adaptive Victim Cache ==="
echo ""

run_experiment "Checkpoint 2 - Adaptive Tests" \
    "--checkpoint2" \
    "$RESULTS_DIR/checkpoint2_results.txt"

# Mixed and phase benchmarks
run_experiment "Mixed Workload Benchmark" \
    "--benchmark mixed" \
    "$RESULTS_DIR/benchmark_mixed.txt"

run_experiment "Phase-Changing Benchmark" \
    "--benchmark phase" \
    "$RESULTS_DIR/benchmark_phase.txt"

# Analysis
echo ""
echo "=== ANALYSIS ==="
echo ""

if command -v python3 &> /dev/null; then
    echo -e "${YELLOW}Analyzing results...${NC}"
    
    # Analyze Checkpoint 1
    if [ -f "$RESULTS_DIR/checkpoint1_results.txt" ]; then
        python3 scripts/analyze_results.py "$RESULTS_DIR/checkpoint1_results.txt" \
            --export "$RESULTS_DIR/checkpoint1.csv"
    fi
    
    # Analyze Checkpoint 2
    if [ -f "$RESULTS_DIR/checkpoint2_results.txt" ]; then
        python3 scripts/analyze_results.py "$RESULTS_DIR/checkpoint2_results.txt" \
            --export "$RESULTS_DIR/checkpoint2.csv"
    fi
    
    # Compare checkpoints
    if [ -f "$RESULTS_DIR/checkpoint1_results.txt" ] && [ -f "$RESULTS_DIR/checkpoint2_results.txt" ]; then
        echo ""
        echo -e "${YELLOW}Comparing checkpoints...${NC}"
        python3 scripts/analyze_results.py "$RESULTS_DIR/checkpoint1_results.txt" \
            --compare "$RESULTS_DIR/checkpoint2_results.txt"
    fi
    
    echo -e "${GREEN}✓ Analysis complete${NC}"
else
    echo -e "${YELLOW}Warning: python3 not found, skipping analysis${NC}"
fi

# Summary
echo ""
echo "========================================"
echo "Experiment Summary"
echo "========================================"
echo "Results saved to: $RESULTS_DIR/"
echo ""
echo "Generated files:"
ls -lh "$RESULTS_DIR/"
echo ""
echo -e "${GREEN}All experiments completed successfully!${NC}"
echo ""
echo "Next steps:"
echo "  1. Review results in $RESULTS_DIR/"
echo "  2. Run: python3 scripts/visualize.py --demo"
echo "  3. Check CSV exports for detailed data"
echo ""

