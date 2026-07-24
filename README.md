# C++ Limit Order Book

A single-threaded in-memory limit order book for studying matching-engine fundamentals. It maintains price-time priority with fixed-point prices, an order-ID hash map for direct cancellation, ordered price-level lookup, intrusive FIFO order queues, and block-allocated `Order` / `PriceLevel` storage.

## Design

- **Price-time priority:** the best executable price is matched first; orders at the same price are matched FIFO.
- **Fixed-point prices:** `Price` stores four decimal places as an integer, avoiding floating-point comparison drift in matching.
- **Order lookup:** `std::unordered_map<int, Order*>` finds an order by ID for cancellation and modification.
- **Price levels:** `std::map<Price, PriceLevel*>` supports price lookup, while intrusive `prev_price` / `next_price` links keep best-to-worst traversal cheap.
- **FIFO queues:** each `PriceLevel` owns an intrusive linked list of its resting orders, so removing a known order does not scan the level.
- **Block allocator:** `MemoryPool` allocates `Order` and `PriceLevel` objects in fixed blocks. It can grow when capacity is exceeded; the benchmark reserves capacity before timing.

## Benchmarks

Measured on an Apple M1 Pro with Apple clang 17.0.0 in Release mode (`-O3 -march=native`), using a warmed 500-order book and seven trials.

| Scenario | Median | p99 | Throughput | Heap allocations/op |
| --- | ---: | ---: | ---: | ---: |
| Single-level match | 458 ns | 542 ns | 2.19M ops/s | 5 |
| Four-level sweep | 1,125 ns | 1,250 ns | 0.87M ops/s | 17 |
| Add resting order | 83 ns | 125 ns | 13.40M ops/s | 2 |
| Cancel resting order | 42 ns* | 42 ns | 28.54M ops/s | 0 |

`*` The cancel median is at the local timer quantum (about 41 ns), so it is resolution-limited rather than a precise latency claim. Full methodology, variability, allocation counts, and limitations are in [BENCHMARKS.md](BENCHMARKS.md).

## Build and run

Requires CMake 3.14+ and a C++17 compiler.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
ctest --test-dir build --output-on-failure
./build/orderbook
./build/orderbook_bench
```

## Scope and limitations

This is a learning-oriented, single-threaded engine. It does not model network I/O, persistence, market-data feeds, risk controls, synchronization, exchange protocols, self-trade prevention, or production durability. The benchmark is synthetic and local; it is not a claim about end-to-end exchange throughput.
