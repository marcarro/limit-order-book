#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <chrono>
#include <iostream>

class Benchmark {
private:
  std::chrono::time_point<std::chrono::high_resolution_clock> start_point;
public:
  Benchmark(): start_point(std::chrono::high_resolution_clock::now()) {}
  ~Benchmark() { Stop(); }
  void Stop() {
    auto endtimepoint = std::chrono::high_resolution_clock::now();

    auto start = std::chrono::time_point_cast<std::chrono::nanoseconds>(start_point).time_since_epoch().count();
    auto end = std::chrono::time_point_cast<std::chrono::nanoseconds>(endtimepoint).time_since_epoch().count();
    
    auto duration = end - start;
    double us = duration * 0.001;
    double ms = us * 0.001;
    
    std::cout << duration << " ns (" << us << " us) - (" << ms << " ms)\n";
  }
};

#endif