#!/usr/bin/env python3
"""
Analysis script for Adaptive Victim Cache results
CS683 Final Project
"""

import sys
import csv
import statistics
from collections import defaultdict

def parse_results(filename):
    """Parse benchmark results from text file."""
    results = {}
    current_benchmark = None
    
    try:
        with open(filename, 'r') as f:
            for line in f:
                line = line.strip()
                
                # Detect benchmark name
                if 'Benchmark' in line and 'Starting' in line:
                    current_benchmark = line.split('[')[1].split(']')[0]
                    results[current_benchmark] = {}
                
                # Parse metrics
                if current_benchmark:
                    if 'Hit Rate:' in line:
                        hit_rate = float(line.split(':')[1].strip().rstrip('%'))
                        results[current_benchmark]['hit_rate'] = hit_rate
                    elif 'Total Accesses:' in line:
                        accesses = int(line.split(':')[1].strip())
                        results[current_benchmark]['accesses'] = accesses
                    elif 'Occupancy:' in line:
                        occupancy = float(line.split(':')[1].strip().rstrip('%'))
                        results[current_benchmark]['occupancy'] = occupancy
    
    except FileNotFoundError:
        print(f"Error: File {filename} not found")
        return None
    
    return results

def calculate_statistics(results):
    """Calculate aggregate statistics."""
    if not results:
        return None
    
    hit_rates = [r.get('hit_rate', 0) for r in results.values() if 'hit_rate' in r]
    occupancies = [r.get('occupancy', 0) for r in results.values() if 'occupancy' in r]
    
    stats = {
        'avg_hit_rate': statistics.mean(hit_rates) if hit_rates else 0,
        'max_hit_rate': max(hit_rates) if hit_rates else 0,
        'min_hit_rate': min(hit_rates) if hit_rates else 0,
        'avg_occupancy': statistics.mean(occupancies) if occupancies else 0,
        'num_benchmarks': len(results)
    }
    
    return stats

def compare_checkpoints(cp1_file, cp2_file):
    """Compare Checkpoint 1 and Checkpoint 2 results."""
    print("\n=== Checkpoint Comparison ===")
    
    cp1_results = parse_results(cp1_file)
    cp2_results = parse_results(cp2_file)
    
    if not cp1_results or not cp2_results:
        print("Error: Could not parse results files")
        return
    
    cp1_stats = calculate_statistics(cp1_results)
    cp2_stats = calculate_statistics(cp2_results)
    
    print(f"\nCheckpoint 1 (Static):")
    print(f"  Average Hit Rate: {cp1_stats['avg_hit_rate']:.2f}%")
    print(f"  Average Occupancy: {cp1_stats['avg_occupancy']:.2f}%")
    
    print(f"\nCheckpoint 2 (Adaptive):")
    print(f"  Average Hit Rate: {cp2_stats['avg_hit_rate']:.2f}%")
    print(f"  Average Occupancy: {cp2_stats['avg_occupancy']:.2f}%")
    
    improvement = cp2_stats['avg_hit_rate'] - cp1_stats['avg_hit_rate']
    print(f"\nImprovement: {improvement:.2f}%")
    
    if improvement >= 8.0:
        print("✓ Target improvement (8-15%) ACHIEVED!")
    else:
        print("⚠ Target improvement not yet achieved")

def export_csv(results, output_file):
    """Export results to CSV format."""
    try:
        with open(output_file, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['Benchmark', 'Hit Rate (%)', 'Occupancy (%)', 'Total Accesses'])
            
            for bench_name, data in results.items():
                writer.writerow([
                    bench_name,
                    data.get('hit_rate', 0),
                    data.get('occupancy', 0),
                    data.get('accesses', 0)
                ])
        
        print(f"\n✓ Results exported to {output_file}")
    except Exception as e:
        print(f"Error exporting CSV: {e}")

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 analyze_results.py <results_file> [--compare <cp2_file>] [--export <csv_file>]")
        sys.exit(1)
    
    results_file = sys.argv[1]
    results = parse_results(results_file)
    
    if not results:
        print("No results found")
        sys.exit(1)
    
    # Print summary
    print("\n=== Results Summary ===")
    for bench_name, data in results.items():
        print(f"\n{bench_name}:")
        for key, value in data.items():
            if 'rate' in key or 'occupancy' in key:
                print(f"  {key}: {value:.2f}%")
            else:
                print(f"  {key}: {value}")
    
    stats = calculate_statistics(results)
    print(f"\n=== Aggregate Statistics ===")
    print(f"Benchmarks Run: {stats['num_benchmarks']}")
    print(f"Average Hit Rate: {stats['avg_hit_rate']:.2f}%")
    print(f"Average Occupancy: {stats['avg_occupancy']:.2f}%")
    print(f"Hit Rate Range: {stats['min_hit_rate']:.2f}% - {stats['max_hit_rate']:.2f}%")
    
    # Handle optional arguments
    if '--compare' in sys.argv:
        cp2_idx = sys.argv.index('--compare') + 1
        if cp2_idx < len(sys.argv):
            compare_checkpoints(results_file, sys.argv[cp2_idx])
    
    if '--export' in sys.argv:
        export_idx = sys.argv.index('--export') + 1
        if export_idx < len(sys.argv):
            export_csv(results, sys.argv[export_idx])

if __name__ == '__main__':
    main()

