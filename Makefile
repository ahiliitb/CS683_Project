# Makefile for Adaptive Victim Cache Project
# CS683 Final Project

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g
INCLUDES = -I. -Isrc -Ibenchmarks

# Directories
SRC_DIR = src
BENCH_DIR = benchmarks
BUILD_DIR = build
BIN_DIR = bin

# Source files
CACHE_SRCS = $(SRC_DIR)/cache/victim_cache.cpp
MONITOR_SRCS = $(SRC_DIR)/monitoring/phase_detector.cpp
ADAPTIVE_SRCS = $(SRC_DIR)/adaptive/adaptive_controller.cpp
BENCH_SRCS = $(BENCH_DIR)/synthetic_benchmark.cpp
MAIN_SRC = $(SRC_DIR)/main.cpp

# Object files
CACHE_OBJS = $(BUILD_DIR)/victim_cache.o
MONITOR_OBJS = $(BUILD_DIR)/phase_detector.o
ADAPTIVE_OBJS = $(BUILD_DIR)/adaptive_controller.o
BENCH_OBJS = $(BUILD_DIR)/synthetic_benchmark.o
MAIN_OBJ = $(BUILD_DIR)/main.o

ALL_OBJS = $(CACHE_OBJS) $(MONITOR_OBJS) $(ADAPTIVE_OBJS) $(BENCH_OBJS) $(MAIN_OBJ)

# Target executable
TARGET = $(BIN_DIR)/victim_cache_sim

.PHONY: all clean dirs checkpoint1 checkpoint2 help

all: dirs $(TARGET)

dirs:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)
	@mkdir -p results

$(TARGET): $(ALL_OBJS)
	@echo "Linking $@..."
	$(CXX) $(CXXFLAGS) -o $@ $(ALL_OBJS)
	@echo "Build complete!"

$(BUILD_DIR)/victim_cache.o: $(SRC_DIR)/cache/victim_cache.cpp $(SRC_DIR)/cache/victim_cache.h
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/phase_detector.o: $(SRC_DIR)/monitoring/phase_detector.cpp $(SRC_DIR)/monitoring/phase_detector.h
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/adaptive_controller.o: $(SRC_DIR)/adaptive/adaptive_controller.cpp $(SRC_DIR)/adaptive/adaptive_controller.h
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/synthetic_benchmark.o: $(BENCH_DIR)/synthetic_benchmark.cpp $(BENCH_DIR)/synthetic_benchmark.h
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/main.o: $(SRC_DIR)/main.cpp
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

checkpoint1: $(TARGET)
	@echo "\n=== Running Checkpoint 1 Tests ==="
	./$(TARGET) --checkpoint1 | tee results/checkpoint1_results.txt

checkpoint2: $(TARGET)
	@echo "\n=== Running Checkpoint 2 Tests ==="
	./$(TARGET) --checkpoint2 | tee results/checkpoint2_results.txt

test: $(TARGET)
	@echo "\n=== Running All Tests ==="
	./$(TARGET) --all | tee results/all_results.txt

clean:
	@echo "Cleaning build files..."
	rm -rf $(BUILD_DIR)/*.o
	rm -rf $(BIN_DIR)/*
	rm -rf results/*.txt results/*.csv
	@echo "Clean complete!"

help:
	@echo "Adaptive Victim Cache Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all         - Build the simulator (default)"
	@echo "  checkpoint1 - Build and run Checkpoint 1 tests"
	@echo "  checkpoint2 - Build and run Checkpoint 2 tests"
	@echo "  test        - Build and run all tests"
	@echo "  clean       - Remove build artifacts"
	@echo "  help        - Display this help message"
	@echo ""
	@echo "Usage examples:"
	@echo "  make                    # Build simulator"
	@echo "  make checkpoint1        # Run Checkpoint 1"
	@echo "  make checkpoint2        # Run Checkpoint 2"
	@echo "  make clean              # Clean build"

