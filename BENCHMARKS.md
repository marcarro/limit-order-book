# Benchmark Results

## Environment

| Property | Value |
| --- | --- |
| Date | 2026-07-24 |
| CPU | Apple M1 Pro (arm64) |
| OS | macOS 26.2 (25C56) |
| Compiler | Apple clang 17.0.0 (clang-1700.0.13.5) |
| Build | CMake Release, `-O3 -march=native` |

## Methodology

The benchmark is a separate target from the demo. It starts each trial from a freshly built 500-order book: 64 bid levels, 64 ask levels, four resting orders per normal level, and one order at each of the four best ask levels used by sweep scenarios.

Every scenario runs seven trials, discards 1,000 warmup samples, and records 5,000 measured samples. Input orders, order/price pools, order and price indexes, and `ExecutionBuffer` are created before a timed operation. The benchmark overrides global `new` / `delete` and counts only calls made while an operation is timed.

Resting adds and cancels are measured in 128-operation batches. Each batch is compared with an equal empty loop containing a compiler fence; the reported value is the adjusted batch time divided by 128. This avoids presenting the local roughly 41 ns single-call timer quantum as a cancellation result.

## Results

| Scenario | Median | p99 | p99.9 | Throughput | Trial median spread | Heap allocs/op | Heap deallocs/op |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Add resting, batch-normalized | 14.0 ns | 644.9 ns | 834.3 ns | 71.43M/s | 14.0-14.3 ns | 0 | 0 |
| Cancel existing, batch-normalized | 99.0 ns | 791.0 ns | 917.7 ns | 10.10M/s | 99.0-99.3 ns | 0 | 0 |
| Single match | 375.0 ns | 750.0 ns | 833.0 ns | 2.67M/s | 375.0-416.0 ns | 0 | 0 |
| Four-level sweep | 1,500.0 ns | 1,875.0 ns | 2,555.1 ns | 0.67M/s | 1,500.0-1,541.0 ns | 0 | 0 |
| Mixed stream: 55% add, 25% cancel, 15% single match, 5% sweep | 42.0 ns* | 1,542.0 ns | 1,875.0 ns | 23.81M/s | 42.0-42.0 ns | 0 | 0 |

`*` The mixed stream is intentionally measured one event at a time, so its median remains at the local clock floor. Use the batch-normalized add and cancel figures for those operation-specific numbers.

## What this proves

The measured engine path performs no heap allocation or deallocation for resting adds, cancellations, one-level matches, or four-level sweeps. This includes emitting executions because `ExecutionBuffer` is preallocated and carries numeric IDs instead of owning strings.

## What this does not measure

- Network, persistence, protocol parsing, risk checks, or market-data publishing.
- Concurrent matching, lock contention, CPU affinity, thermal variance, or heterogeneous-core scheduling.
- An instrument-specific direct price ladder. This generic engine uses a fixed hash table and fixed binary heap, so new price levels cost `O(log L)` heap work.
- End-to-end exchange latency or throughput under production traffic.
