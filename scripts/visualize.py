#!/usr/bin/env python3
"""
Visualization script for Adaptive Victim Cache
Generates plots for performance analysis
"""

import sys
import csv
import matplotlib.pyplot as plt
import numpy as np

def read_adaptation_history(filename):
    """Read adaptation history from CSV file."""
    timestamps = []
    sizes = []
    hit_rates = []
    occupancies = []
    phases = []
    
    try:
        with open(filename, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                timestamps.append(int(row['timestamp']))
                sizes.append(int(row['victim_size']))
                hit_rates.append(float(row['hit_rate']) * 100)
                occupancies.append(float(row['occupancy']) * 100)
                phases.append(int(row['phase']))
    except FileNotFoundError:
        print(f"Error: File {filename} not found")
        return None
    
    return {
        'timestamps': timestamps,
        'sizes': sizes,
        'hit_rates': hit_rates,
        'occupancies': occupancies,
        'phases': phases
    }

def plot_adaptation_over_time(data, output_file='results/adaptation_plot.png'):
    """Plot victim cache size, hit rate, and occupancy over time."""
    fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(12, 10))
    
    timestamps = np.array(data['timestamps']) / 1000  # Convert to K instructions
    
    # Plot 1: Victim cache size
    ax1.plot(timestamps, data['sizes'], 'b-', linewidth=2)
    ax1.set_ylabel('Victim Cache Size', fontsize=12)
    ax1.set_title('Adaptive Victim Cache - Dynamic Size Adjustment', fontsize=14, fontweight='bold')
    ax1.grid(True, alpha=0.3)
    ax1.set_ylim([0, 140])
    
    # Plot 2: Hit rate
    ax2.plot(timestamps, data['hit_rates'], 'g-', linewidth=2)
    ax2.set_ylabel('Hit Rate (%)', fontsize=12)
    ax2.axhline(y=12, color='r', linestyle='--', alpha=0.5, label='Target (12%)')
    ax2.legend()
    ax2.grid(True, alpha=0.3)
    
    # Plot 3: Occupancy
    ax3.plot(timestamps, data['occupancies'], 'orange', linewidth=2)
    ax3.set_xlabel('Instructions (K)', fontsize=12)
    ax3.set_ylabel('Occupancy (%)', fontsize=12)
    ax3.axhline(y=85, color='r', linestyle='--', alpha=0.5, label='High Threshold (85%)')
    ax3.axhline(y=40, color='b', linestyle='--', alpha=0.5, label='Low Threshold (40%)')
    ax3.legend()
    ax3.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"✓ Plot saved to {output_file}")
    plt.close()

def plot_hit_rate_comparison(benchmarks, hit_rates, output_file='results/hit_rate_comparison.png'):
    """Plot hit rate comparison across benchmarks."""
    fig, ax = plt.subplots(figsize=(10, 6))
    
    x = np.arange(len(benchmarks))
    bars = ax.bar(x, hit_rates, color=['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd', '#8c564b'])
    
    ax.set_xlabel('Benchmark', fontsize=12)
    ax.set_ylabel('Hit Rate (%)', fontsize=12)
    ax.set_title('Victim Cache Hit Rate by Benchmark', fontsize=14, fontweight='bold')
    ax.set_xticks(x)
    ax.set_xticklabels(benchmarks, rotation=45, ha='right')
    ax.grid(True, axis='y', alpha=0.3)
    
    # Add value labels on bars
    for bar in bars:
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:.1f}%',
                ha='center', va='bottom', fontsize=10)
    
    plt.tight_layout()
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"✓ Plot saved to {output_file}")
    plt.close()

def plot_checkpoint_comparison(cp1_data, cp2_data, output_file='results/checkpoint_comparison.png'):
    """Compare Checkpoint 1 vs Checkpoint 2 performance."""
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    
    benchmarks = list(cp1_data.keys())
    cp1_hit_rates = [cp1_data[b]['hit_rate'] for b in benchmarks]
    cp2_hit_rates = [cp2_data[b]['hit_rate'] for b in benchmarks]
    
    x = np.arange(len(benchmarks))
    width = 0.35
    
    # Hit rate comparison
    ax1.bar(x - width/2, cp1_hit_rates, width, label='Static (CP1)', color='#1f77b4')
    ax1.bar(x + width/2, cp2_hit_rates, width, label='Adaptive (CP2)', color='#ff7f0e')
    ax1.set_xlabel('Benchmark', fontsize=12)
    ax1.set_ylabel('Hit Rate (%)', fontsize=12)
    ax1.set_title('Hit Rate: Static vs Adaptive', fontsize=14, fontweight='bold')
    ax1.set_xticks(x)
    ax1.set_xticklabels(benchmarks, rotation=45, ha='right')
    ax1.legend()
    ax1.grid(True, axis='y', alpha=0.3)
    
    # Improvement percentage
    improvements = [(cp2 - cp1) for cp1, cp2 in zip(cp1_hit_rates, cp2_hit_rates)]
    colors = ['green' if imp > 0 else 'red' for imp in improvements]
    ax2.bar(x, improvements, color=colors, alpha=0.7)
    ax2.set_xlabel('Benchmark', fontsize=12)
    ax2.set_ylabel('Improvement (%)', fontsize=12)
    ax2.set_title('Performance Improvement', fontsize=14, fontweight='bold')
    ax2.set_xticks(x)
    ax2.set_xticklabels(benchmarks, rotation=45, ha='right')
    ax2.axhline(y=0, color='black', linestyle='-', linewidth=0.5)
    ax2.axhline(y=8, color='green', linestyle='--', alpha=0.5, label='Target (8%)')
    ax2.legend()
    ax2.grid(True, axis='y', alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"✓ Plot saved to {output_file}")
    plt.close()

def main():
    print("=== Visualization Tool ===\n")
    
    # Example usage
    if len(sys.argv) < 2:
        print("Usage: python3 visualize.py <adaptation_history.csv>")
        print("   or: python3 visualize.py --demo")
        sys.exit(1)
    
    if sys.argv[1] == '--demo':
        print("Generating demo plots...")
        
        # Demo data
        benchmarks = ['Sequential', 'Random', 'Repeated', 'Strided', 'Mixed', 'Phase']
        hit_rates = [2.1, 8.7, 15.3, 4.5, 11.2, 13.8]
        
        plot_hit_rate_comparison(benchmarks, hit_rates)
        print("\n✓ Demo visualization complete!")
    else:
        data = read_adaptation_history(sys.argv[1])
        if data:
            plot_adaptation_over_time(data)
            print("\n✓ Visualization complete!")

if __name__ == '__main__':
    main()

