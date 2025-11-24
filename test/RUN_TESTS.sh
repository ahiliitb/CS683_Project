#!/bin/bash

# Simple script to run real cache simulations
# CS683 Adaptive Victim Cache Project

cd "$(dirname "$0")"

echo "============================================================"
echo "          RUNNING REAL CACHE SIMULATIONS                   "
echo "============================================================"
echo ""
echo "This will test THREE configurations:"
echo "  1. BASELINE    - No victim cache"
echo "  2. STATIC      - 64-entry victim cache  "
echo "  3. ADAPTIVE    - 64-256 entry adaptive victim cache (starts at 80)"
echo ""
echo "Testing on 3 workloads:"
echo "  • Memory-intensive (large working set)"
echo "  • Compute-intensive (small working set)"  
echo "  • Mixed (phase-changing workload)"
echo ""
echo "==============================================================="
echo ""

# Build if needed
if [ ! -f "simulations/cache_sim" ]; then
    echo "Building simulator..."
    make > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo "[ERROR] Build failed. Running make with output:"
        make
        exit 1
    fi
    echo "[OK] Build complete"
    echo ""
fi

# Run simulation
echo "Running simulations (this takes ~20 seconds)..."
echo ""

./simulations/cache_sim

echo ""
echo "============================================================"
echo "                  SIMULATION COMPLETE                       "
echo "============================================================"
echo ""
echo "Results saved to:"
echo "   test/results/REAL_RESULTS_COMPARISON.txt"
echo ""
echo "To view results:"
echo "   cat test/results/REAL_RESULTS_COMPARISON.txt"
echo ""

