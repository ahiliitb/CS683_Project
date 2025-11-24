# Makefile for Cache Simulation Tests
# Real performance measurements

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g
INCLUDES = -I. -I../src -I../benchmarks -I../config

# Source files
SIM_SRCS = cache_simulator.cpp run_simulation.cpp
PROJ_SRCS = ../src/cache/victim_cache.cpp \
            ../src/monitoring/phase_detector.cpp \
            ../src/adaptive/adaptive_controller.cpp

# Object files
BUILD_DIR = build
SIM_OBJS = $(BUILD_DIR)/cache_simulator.o $(BUILD_DIR)/run_simulation.o
PROJ_OBJS = $(BUILD_DIR)/victim_cache.o \
            $(BUILD_DIR)/phase_detector.o \
            $(BUILD_DIR)/adaptive_controller.o

ALL_OBJS = $(SIM_OBJS) $(PROJ_OBJS)

# Target executable
TARGET = simulations/cache_sim

.PHONY: all clean run dirs

all: dirs $(TARGET)

dirs:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p simulations
	@mkdir -p results

$(TARGET): $(ALL_OBJS)
	@echo "Linking simulator..."
	$(CXX) $(CXXFLAGS) -o $@ $(ALL_OBJS)
	@echo "[OK] Simulator built: $(TARGET)"

$(BUILD_DIR)/cache_simulator.o: cache_simulator.cpp cache_simulator.h
	@echo "Compiling cache_simulator.cpp..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/run_simulation.o: run_simulation.cpp cache_simulator.h
	@echo "Compiling run_simulation.cpp..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/victim_cache.o: ../src/cache/victim_cache.cpp ../src/cache/victim_cache.h
	@echo "Compiling victim_cache.cpp..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/phase_detector.o: ../src/monitoring/phase_detector.cpp ../src/monitoring/phase_detector.h
	@echo "Compiling phase_detector.cpp..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/adaptive_controller.o: ../src/adaptive/adaptive_controller.cpp ../src/adaptive/adaptive_controller.h
	@echo "Compiling adaptive_controller.cpp..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

run: $(TARGET)
	@echo ""
	@echo "============================================================"
	@echo "   Running Real Cache Simulations"
	@echo "============================================================"
	@echo ""
	./$(TARGET)

clean:
	@echo "Cleaning build files..."
	rm -rf $(BUILD_DIR)/*.o
	rm -rf $(TARGET)
	rm -rf results/*.txt
	@echo "Clean complete!"

help:
	@echo "Cache Simulation Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all     - Build the simulator (default)"
	@echo "  run     - Build and run all simulations"
	@echo "  clean   - Remove build artifacts"
	@echo "  help    - Display this help message"
	@echo ""
	@echo "Usage:"
	@echo "  make        # Build simulator"
	@echo "  make run    # Run all 3 configurations"

