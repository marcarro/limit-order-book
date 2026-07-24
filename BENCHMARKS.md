# Benchmark Results

## Run environment

| Property | Value |
| --- | --- |
| Date | 2026-07-24 |
| CPU | Apple M1 Pro (arm64) |
| OS | macOS 26.2 (25C56) |
| Compiler | Apple clang 17.0.0 (clang-1700.0.13.5) |
| Build | CMake Release |
| Project compile flags | `-O3 -march=native -Wall -Wextra -Wpedantic` |

Run with:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
./build/orderbook_bench
```

## Methodology

The benchmark target is separate from the demo executable. It creates the order streams before timing starts: no random-number generation, input construction, or harness vector growth runs inside a measured operation.

Each trial starts from a freshly built steady-state book with 64 bid levels, 64 ask levels, and 500 resting orders. The book is warmed for 5,000 operations, then 25,000 operations are measured. Each scenario runs seven independent trials; this report uses the trial whose median latency is the median of the seven trial medians and includes the observed trial spread.

The benchmark measures an empty timing region separately and subtracts its median overhead. On this machine the smallest non-zero `steady_clock` delta was about 41 ns. Results at or below that value are explicitly marked resolution-limited.

The benchmark overrides global `new` and `delete` in the benchmark binary and counts calls only while an operation is timed. It pre-reserves the order and price-level pools and the trade vector. Client IDs deliberately exceed small-string optimization capacity, so the measurement includes the engine's real `std::string` copies as well as allocations from its `std::unordered_map<int, Order*>` order index.

## Results

| Scenario | Operation | Median | p99 | p99.9 | Throughput | Trial median spread | Heap allocs/op | Heap deallocs/op |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Add resting | Non-crossing buy at an existing bid level; cancelled after timing | 83 ns | 125 ns | 208 ns | 13.40M/s | 83-83 ns | 2 | 0 |
| Cancel existing | Cancel a resting buy appended to an existing bid level | 42 ns* | 42 ns | 125 ns | 28.54M/s | 42-42 ns | 0 | 1 |
| Single match | Crossing buy fills one best-ask order | 458 ns | 542 ns | 1,959 ns | 2.19M/s | 458-458 ns | 5 | 6 |
| Four-level sweep | Crossing buy fills one order at each of four ask levels | 1,125 ns | 1,250 ns | 5,333 ns | 0.87M/s | 1,125-1,125 ns | 17 | 21 |
| Mixed stream | 55% add, 25% cancel, 15% single match, 5% four-level sweep | 83 ns | 1,458 ns | 2,542 ns | 4.95M/s | 83-83 ns | 2 median, 17 p99, 17 max | 0 median, 21 max |

`*` The measured median is at the local timer quantum, so it should be read as “at most roughly 42 ns under this harness,” not as a precise per-operation latency.

Clock-overhead samples had a 0 ns median, 42 ns p99, and an approximately 41 ns minimum non-zero quantum. The benchmark subtracts the 0 ns median; it does not pretend this removes quantization noise from sub-quantum operations.

## Allocation findings

The accurate claim is that `Order` and `PriceLevel` storage comes from a block pool, not that the entire hot path is heap-allocation free:

- Adding a resting order made **two heap allocation calls per operation**: one for the copied long client ID in the stored `Order`, and one for the `order_map_` node.
- A fully filled single-level match made **five heap allocation calls**: one for the incoming stored `Order` client ID, plus two long client IDs copied into the stack `TradeInfo` and again into the pre-reserved trade vector.
- A four-level sweep made **17 heap allocation calls**: one incoming `Order` client copy and four long-string allocations for each of four emitted trades.
- Cancelling a resting order made **one heap deallocation call per operation** when the map node was erased.
- `MemoryPool` has a growth path that creates another block if all existing blocks are full. The benchmark pre-reserves capacity, so that path did not run. Production code must size or bound pools appropriately if allocation-free behavior is required.

## What this does not measure

- Multi-threaded contention, synchronization, or lock-free behavior.
- Network, exchange protocol parsing, persistence, recovery, risk checks, or market-data publication.
- Real market order flow, adverse selection, self-trade prevention, or exchange-specific rules.
- Tail latency under CPU contention, thermal throttling, power management, or heterogeneous-core scheduling. The M1 Pro can vary under those conditions.
- Exact latency below the roughly 41 ns local timer quantum.

These figures are useful local microbenchmark evidence for the in-memory matching engine, not an end-to-end production exchange capacity claim.
