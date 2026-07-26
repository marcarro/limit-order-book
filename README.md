# C++ Limit Order Book

A fixed-capacity, single-threaded in-memory matching engine with price-time priority. The matching core accepts numeric orders, writes numeric executions into a caller-provided preallocated buffer, and performs no heap allocation after construction.

## Core design

- **Numeric boundary:** `Order` and `Execution` carry `ClientId` / `OrderId`, never client strings. String-to-ID resolution belongs in an API adapter before an order reaches the book.
- **Fixed capacity:** `OrderbookConfig` fixes maximum live orders and price levels at construction. Capacity failures are explicit results, never dynamic growth on the matching path.
- **Preallocated execution output:** callers supply `ExecutionBuffer` with capacity at least `book.max_orders()`. This makes a multi-fill order atomic without allocating or risking partial output.
- **Order lookup:** a fixed-capacity open-addressing hash table supports direct cancellation without node allocations or rehashing.
- **Price levels:** each side uses a fixed price-to-level hash table plus a preallocated binary heap for best-price lookup. Orders at a price live in an intrusive FIFO queue.
- **Exact arithmetic:** prices use four-decimal fixed-point values.

## Benchmarks

Measured on an Apple M1 Pro with Apple clang 17.0.0 in Release mode (`-O3 -march=native`). The warm book holds 500 resting orders across 64 bid and 64 ask levels; each result is the representative run from seven trials.

| Scenario | Median | p99 | Heap allocations/op |
| --- | ---: | ---: | ---: |
| Resting add, batch-normalized | 14.0 ns | 644.9 ns | 0 |
| Cancel, batch-normalized | 99.0 ns | 791.0 ns | 0 |
| One-level match | 375.0 ns | 750.0 ns | 0 |
| Four-level sweep | 1,500.0 ns | 1,875.0 ns | 0 |

The add and cancel benchmarks time batches of 128 operations, subtract a matching empty-loop control, then divide by 128. Full methodology and limitations are in [BENCHMARKS.md](BENCHMARKS.md).

## Build and run

Requires CMake 3.14+ and a C++17 compiler.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
ctest --test-dir build --output-on-failure
./build/orderbook
./build/orderbook_bench
```

## Scope

This is a bounded, single-threaded matching core. It does not provide client-ID registration, networking, persistence, risk controls, market-data publication, concurrency, exchange protocol handling, or an instrument-specific direct price ladder. Those belong outside this core or require an explicit bounded tick-range contract.
