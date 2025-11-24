#include "cache_simulator.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>

struct SimulationResult {
    std::string config_name;
    double l2_hit_rate;
    double llc_hit_rate;
    double llc_miss_rate;
    double mpki;
    double victim_hit_rate;
};

void run_workload(CacheHierarchySimulator& sim, const std::vector<uint64_t>& addresses, const std::string& workload_name) {
    std::cout << "\n  Running " << workload_name << " (" << addresses.size() << " accesses)..." << std::endl;
    
    for (uint64_t addr : addresses) {
        sim.access_memory(addr);
        sim.simulate_instruction();
    }
}

SimulationResult simulate_configuration(const std::string& config, const std::vector<uint64_t>& addresses, const std::string& workload) {
    bool use_victim = (config != "baseline");
    bool use_adaptive = (config == "adaptive");
    
    CacheHierarchySimulator sim(use_victim, use_adaptive);
    run_workload(sim, addresses, workload);
    
    SimulationResult result;
    result.config_name = config;
    result.l2_hit_rate = sim.get_l2_hit_rate();
    result.llc_hit_rate = sim.get_llc_hit_rate();
    result.llc_miss_rate = 1.0 - sim.get_llc_hit_rate();
    result.mpki = sim.get_memory_access_rate() * 1000.0;
    
    sim.print_summary();
    sim.export_results("test/results/" + config + "_" + workload + ".txt");
    
    return result;
}

void compare_results(const std::vector<SimulationResult>& results, const std::string& workload) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║              PERFORMANCE COMPARISON - " << std::setw(20) << workload << "   ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;
    
    SimulationResult baseline, static_vc, adaptive_vc;
    for (const auto& r : results) {
        if (r.config_name == "baseline") baseline = r;
        else if (r.config_name == "static") static_vc = r;
        else if (r.config_name == "adaptive") adaptive_vc = r;
    }
    
    std::cout << "\n┌────────────────┬───────────┬───────────┬───────────┐" << std::endl;
    std::cout << "│ Metric         │ Baseline  │ Static VC │ Adaptive  │" << std::endl;
    std::cout << "├────────────────┼───────────┼───────────┼───────────┤" << std::endl;
    
    std::cout << "│ L2 Hit Rate    │ " << std::setw(8) << std::fixed << std::setprecision(2) 
              << (baseline.l2_hit_rate * 100) << "% │ " 
              << std::setw(8) << (static_vc.l2_hit_rate * 100) << "% │ "
              << std::setw(8) << (adaptive_vc.l2_hit_rate * 100) << "% │" << std::endl;
    
    std::cout << "│ LLC Miss Rate  │ " << std::setw(8) << (baseline.llc_miss_rate * 100) << "% │ " 
              << std::setw(8) << (static_vc.llc_miss_rate * 100) << "% │ "
              << std::setw(8) << (adaptive_vc.llc_miss_rate * 100) << "% │" << std::endl;
    
    std::cout << "│ MPKI           │ " << std::setw(9) << baseline.mpki << " │ " 
              << std::setw(9) << static_vc.mpki << " │ "
              << std::setw(9) << adaptive_vc.mpki << " │" << std::endl;
    
    std::cout << "└────────────────┴───────────┴───────────┴───────────┘" << std::endl;
    
    double static_improvement = ((static_vc.llc_hit_rate - baseline.llc_hit_rate) / baseline.llc_hit_rate) * 100;
    double adaptive_improvement = ((adaptive_vc.llc_hit_rate - baseline.llc_hit_rate) / baseline.llc_hit_rate) * 100;
    
    double llc_miss_reduction_static = (baseline.llc_miss_rate - static_vc.llc_miss_rate) * 100;
    double llc_miss_reduction_adaptive = (baseline.llc_miss_rate - adaptive_vc.llc_miss_rate) * 100;
    
    std::cout << "\n📊 IMPROVEMENTS:" << std::endl;
    std::cout << "  Static Victim Cache:" << std::endl;
    std::cout << "    LLC Hit Rate:        " << std::showpos << std::fixed << std::setprecision(2) 
              << static_improvement << "%" << std::endl;
    std::cout << "    LLC Miss Reduction:  " << std::noshowpos << llc_miss_reduction_static 
              << " percentage points" << std::endl;
    
    std::cout << "\n  Adaptive Victim Cache:" << std::endl;
    std::cout << "    LLC Hit Rate:        " << std::showpos << adaptive_improvement << "%" << std::endl;
    std::cout << "    LLC Miss Reduction:  " << std::noshowpos << llc_miss_reduction_adaptive 
              << " percentage points" << std::endl;
    
    std::cout << "\n  Adaptive vs Static:" << std::endl;
    double adaptive_over_static = ((adaptive_vc.llc_hit_rate - static_vc.llc_hit_rate) / static_vc.llc_hit_rate) * 100;
    std::cout << "    Additional Improvement: " << std::showpos << adaptive_over_static << "%" << std::endl;
}

void save_comparison_report(const std::map<std::string, std::vector<SimulationResult>>& all_results) {
    std::ofstream report("test/results/REAL_RESULTS_COMPARISON.txt");
    
    report << "═══════════════════════════════════════════════════════════════\n";
    report << "     ACTUAL SIMULATION RESULTS - REAL MEASUREMENTS\n";
    report << "═══════════════════════════════════════════════════════════════\n\n";
    
    report << "Date: " << __DATE__ << " " << __TIME__ << "\n\n";
    
    report << "Configuration:\n";
    report << "  L2 Cache:  256 entries, 8-way set associative\n";
    report << "  LLC:       2048 entries, 16-way set associative\n";
    report << "  Victim:    64 entries (static), 32-128 entries (adaptive)\n\n";
    
    for (const auto& [workload, results] : all_results) {
        report << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        report << "WORKLOAD: " << workload << "\n";
        report << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
        
        SimulationResult baseline, static_vc, adaptive_vc;
        for (const auto& r : results) {
            if (r.config_name == "baseline") baseline = r;
            else if (r.config_name == "static") static_vc = r;
            else if (r.config_name == "adaptive") adaptive_vc = r;
        }
        
        report << "Baseline (No Victim Cache):\n";
        report << "  L2 Hit Rate:    " << (baseline.l2_hit_rate * 100) << "%\n";
        report << "  LLC Hit Rate:   " << (baseline.llc_hit_rate * 100) << "%\n";
        report << "  LLC Miss Rate:  " << (baseline.llc_miss_rate * 100) << "%\n";
        report << "  MPKI:           " << baseline.mpki << "\n\n";
        
        report << "Static Victim Cache (64 entries):\n";
        report << "  L2 Hit Rate:    " << (static_vc.l2_hit_rate * 100) << "%\n";
        report << "  LLC Hit Rate:   " << (static_vc.llc_hit_rate * 100) << "%\n";
        report << "  LLC Miss Rate:  " << (static_vc.llc_miss_rate * 100) << "%\n";
        report << "  MPKI:           " << static_vc.mpki << "\n\n";
        
        double static_improvement = ((static_vc.llc_hit_rate - baseline.llc_hit_rate) / baseline.llc_hit_rate) * 100;
        double llc_miss_reduction_static = (baseline.llc_miss_rate - static_vc.llc_miss_rate) * 100;
        
        report << "  Improvement over Baseline:\n";
        report << "    LLC Hit Rate:       " << std::showpos << static_improvement << "%\n";
        report << "    LLC Miss Reduction: " << std::noshowpos << llc_miss_reduction_static << " pp\n\n";
        
        report << "Adaptive Victim Cache (32-128 entries):\n";
        report << "  L2 Hit Rate:    " << (adaptive_vc.l2_hit_rate * 100) << "%\n";
        report << "  LLC Hit Rate:   " << (adaptive_vc.llc_hit_rate * 100) << "%\n";
        report << "  LLC Miss Rate:  " << (adaptive_vc.llc_miss_rate * 100) << "%\n";
        report << "  MPKI:           " << adaptive_vc.mpki << "\n\n";
        
        double adaptive_improvement = ((adaptive_vc.llc_hit_rate - baseline.llc_hit_rate) / baseline.llc_hit_rate) * 100;
        double llc_miss_reduction_adaptive = (baseline.llc_miss_rate - adaptive_vc.llc_miss_rate) * 100;
        double adaptive_over_static = ((adaptive_vc.llc_hit_rate - static_vc.llc_hit_rate) / static_vc.llc_hit_rate) * 100;
        
        report << "  Improvement over Baseline:\n";
        report << "    LLC Hit Rate:       " << std::showpos << adaptive_improvement << "%\n";
        report << "    LLC Miss Reduction: " << std::noshowpos << llc_miss_reduction_adaptive << " pp\n\n";
        
        report << "  Improvement over Static:\n";
        report << "    Additional Gain:    " << std::showpos << adaptive_over_static << "%\n\n";
    }
    
    report << "═══════════════════════════════════════════════════════════════\n";
    report << "                    END OF REPORT\n";
    report << "═══════════════════════════════════════════════════════════════\n";
    
    report.close();
    std::cout << "\n✅ Detailed report saved to: test/results/REAL_RESULTS_COMPARISON.txt\n";
}

int main() {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║     CACHE HIERARCHY SIMULATION - REAL PERFORMANCE TEST    ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;
    
    std::map<std::string, std::vector<SimulationResult>> all_results;
    
    {
        std::cout << "\n\n■■■ WORKLOAD 1: MEMORY-INTENSIVE (500K instructions) ■■■\n";
        auto workload = WorkloadGenerator::generate_memory_intensive(500000);
        
        std::vector<SimulationResult> results;
        std::cout << "\n▶ Configuration 1: BASELINE (No Victim Cache)";
        results.push_back(simulate_configuration("baseline", workload, "memory_intensive"));
        
        std::cout << "\n\n▶ Configuration 2: STATIC Victim Cache (128 entries FIXED)";
        results.push_back(simulate_configuration("static", workload, "memory_intensive"));
        
        std::cout << "\n\n▶ Configuration 3: ADAPTIVE Victim Cache (96-256 entries)";
        results.push_back(simulate_configuration("adaptive", workload, "memory_intensive"));
        
        compare_results(results, "Memory-Intensive");
        all_results["Memory-Intensive"] = results;
    }
    
    {
        std::cout << "\n\n■■■ WORKLOAD 2: COMPUTE-INTENSIVE (500K instructions) ■■■\n";
        auto workload = WorkloadGenerator::generate_compute_intensive(500000);
        
        std::vector<SimulationResult> results;
        std::cout << "\n▶ Configuration 1: BASELINE (No Victim Cache)";
        results.push_back(simulate_configuration("baseline", workload, "compute_intensive"));
        
        std::cout << "\n\n▶ Configuration 2: STATIC Victim Cache (128 entries FIXED)";
        results.push_back(simulate_configuration("static", workload, "compute_intensive"));
        
        std::cout << "\n\n▶ Configuration 3: ADAPTIVE Victim Cache (96-256 entries)";
        results.push_back(simulate_configuration("adaptive", workload, "compute_intensive"));
        
        compare_results(results, "Compute-Intensive");
        all_results["Compute-Intensive"] = results;
    }
    
    {
        std::cout << "\n\n■■■ WORKLOAD 3: MIXED WORKLOAD (500K instructions, 6 phases) ■■■\n";
        auto workload = WorkloadGenerator::generate_mixed(500000);
        
        std::vector<SimulationResult> results;
        std::cout << "\n▶ Configuration 1: BASELINE (No Victim Cache)";
        results.push_back(simulate_configuration("baseline", workload, "mixed"));
        
        std::cout << "\n\n▶ Configuration 2: STATIC Victim Cache (128 entries FIXED)";
        results.push_back(simulate_configuration("static", workload, "mixed"));
        
        std::cout << "\n\n▶ Configuration 3: ADAPTIVE Victim Cache (96-256 entries)";
        results.push_back(simulate_configuration("adaptive", workload, "mixed"));
        
        compare_results(results, "Mixed");
        all_results["Mixed"] = results;
    }
    
    save_comparison_report(all_results);
    
    std::cout << "\n\n╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║              SIMULATION COMPLETE!                         ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "\n📁 Results saved in: test/results/\n";
    std::cout << "📊 Main report: test/results/REAL_RESULTS_COMPARISON.txt\n\n";
    
    return 0;
}

