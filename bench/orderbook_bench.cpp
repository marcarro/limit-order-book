#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <new>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

#include "orderbook/Orderbook.h"

namespace {

using Clock = std::chrono::steady_clock;
using trading::ClientId;
using trading::ExecutionBuffer;
using trading::Order;
using trading::OrderId;
using trading::OrderResult;
using trading::Orderbook;
using trading::OrderbookConfig;
using ::Price;
using trading::Quantity;
using trading::Side;

constexpr OrderbookConfig kConfig{1024, 256};
constexpr int kTrials = 7;
constexpr int kWarmupSamples = 1000;
constexpr int kMeasuredSamples = 5000;
constexpr int kCancelBatchSize = 128;
constexpr Quantity kVolume = 100;
constexpr std::int64_t kBestBid = 999'900;
constexpr std::int64_t kBestAsk = 1'000'100;
constexpr std::int64_t kTick = 100;

std::atomic<std::uint64_t> g_allocations{0};
std::atomic<std::uint64_t> g_deallocations{0};
thread_local bool g_count_allocations = false;
volatile std::uint64_t g_sink = 0;

struct AllocationGuard {
    AllocationGuard() { g_count_allocations = true; }
    ~AllocationGuard() { g_count_allocations = false; }
};

struct Sample {
    double ns_per_operation = 0.0;
    double allocations_per_operation = 0.0;
    double deallocations_per_operation = 0.0;
};

struct Trial {
    std::vector<Sample> samples;
};

struct Report {
    std::string name;
    std::string description;
    Trial representative;
    std::vector<double> trial_medians;
};

Order make_order(ClientId client_id, std::int64_t raw_price, OrderId order_id, Quantity quantity, Side side) {
    return Order{client_id, Price::fromRaw(raw_price), order_id, quantity, side};
}

template <typename Fn>
Sample measure(Fn&& operation, int operations_per_sample) {
    const std::uint64_t allocations_before = g_allocations.load(std::memory_order_relaxed);
    const std::uint64_t deallocations_before = g_deallocations.load(std::memory_order_relaxed);
    const auto started = Clock::now();
    {
        AllocationGuard guard;
        operation();
    }
    const auto stopped = Clock::now();
    const std::uint64_t allocations_after = g_allocations.load(std::memory_order_relaxed);
    const std::uint64_t deallocations_after = g_deallocations.load(std::memory_order_relaxed);
    const double divisor = static_cast<double>(operations_per_sample);
    return Sample{
        static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(stopped - started).count()) / divisor,
        static_cast<double>(allocations_after - allocations_before) / divisor,
        static_cast<double>(deallocations_after - deallocations_before) / divisor,
    };
}

double percentile(std::vector<double> values, double fraction) {
    std::sort(values.begin(), values.end());
    const double position = fraction * static_cast<double>(values.size() - 1);
    const std::size_t lower = static_cast<std::size_t>(position);
    const std::size_t upper = static_cast<std::size_t>(std::ceil(position));
    return lower == upper
        ? values[lower]
        : values[lower] + ((values[upper] - values[lower]) * (position - lower));
}

double median_latency(const Trial& trial) {
    std::vector<double> values;
    values.reserve(trial.samples.size());
    for (const Sample& sample : trial.samples) {
        values.push_back(sample.ns_per_operation);
    }
    return percentile(std::move(values), 0.50);
}

void require(OrderResult actual, OrderResult expected, const char* context) {
    if (actual != expected) {
        throw std::runtime_error(context);
    }
}

void seed(Orderbook& book, ExecutionBuffer& executions) {
    OrderId next_id = 1;
    for (int level = 0; level < 64; ++level) {
        for (int order = 0; order < 4; ++order) {
            require(book.place_order(make_order(1, kBestBid - (level * kTick), next_id++, kVolume, Side::BUY), executions), OrderResult::SUCCESS, "failed to seed bid");
        }
    }
    for (int level = 0; level < 64; ++level) {
        const int orders = level < 4 ? 1 : 4;
        for (int order = 0; order < orders; ++order) {
            require(book.place_order(make_order(2, kBestAsk + (level * kTick), next_id++, kVolume, Side::SELL), executions), OrderResult::SUCCESS, "failed to seed ask");
        }
    }
}

template <typename TrialFn>
Report run_report(const char* name, const char* description, TrialFn&& trial_fn) {
    std::vector<Trial> trials;
    trials.reserve(kTrials);
    std::vector<double> medians;
    medians.reserve(kTrials);
    for (int trial = 0; trial < kTrials; ++trial) {
        trials.push_back(trial_fn(trial));
        medians.push_back(median_latency(trials.back()));
    }
    std::vector<std::size_t> indices(kTrials);
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&](std::size_t lhs, std::size_t rhs) {
        return medians[lhs] < medians[rhs];
    });
    return Report{name, description, std::move(trials[indices[kTrials / 2]]), std::move(medians)};
}

Report benchmark_resting_add() {
    return run_report("add_resting", "Add non-crossing orders at an existing bid level in batches of 128; latency is batch time divided by 128.", [](int trial) {
        Orderbook book(kConfig);
        ExecutionBuffer executions{kConfig.max_orders};
        seed(book, executions);
        OrderId next_id = 1'000'000 + (static_cast<OrderId>(trial) * 100'000);
        const auto run = [&](bool record, std::vector<Sample>& samples) {
            for (int sample = 0; sample < (record ? kMeasuredSamples : kWarmupSamples); ++sample) {
                std::vector<Order> orders;
                orders.reserve(kCancelBatchSize);
                for (int index = 0; index < kCancelBatchSize; ++index) {
                    orders.push_back(make_order(3, kBestBid - (10 * kTick), next_id++, kVolume, Side::BUY));
                }
                Sample result = measure([&] {
                    for (const Order& order : orders) {
                        require(book.place_order(order, executions), OrderResult::SUCCESS, "resting add failed");
                    }
                }, kCancelBatchSize);
                const Sample overhead = measure([&] {
                    for (const Order& order : orders) {
                        g_sink += order.order_id;
                        std::atomic_signal_fence(std::memory_order_seq_cst);
                    }
                }, kCancelBatchSize);
                result.ns_per_operation = std::max(0.0, result.ns_per_operation - overhead.ns_per_operation);
                for (const Order& order : orders) {
                    require(book.cancel_order(order.order_id), OrderResult::SUCCESS, "resting add cleanup failed");
                }
                if (record) {
                    samples.push_back(result);
                }
            }
        };
        Trial result;
        result.samples.reserve(kMeasuredSamples);
        std::vector<Sample> ignored;
        run(false, ignored);
        run(true, result.samples);
        return result;
    });
}

Report benchmark_cancel_batch() {
    return run_report("cancel_existing", "Cancel pre-seeded orders in batches of 128; latency is total batch time divided by 128.", [](int trial) {
        OrderId next_id = 2'000'000 + (static_cast<OrderId>(trial) * 1'000'000);
        const auto run = [&](bool record, std::vector<Sample>& samples) {
            for (int sample = 0; sample < (record ? kMeasuredSamples : kWarmupSamples); ++sample) {
                Orderbook book(kConfig);
                ExecutionBuffer executions{kConfig.max_orders};
                seed(book, executions);
                std::vector<OrderId> ids;
                ids.reserve(kCancelBatchSize);
                for (int index = 0; index < kCancelBatchSize; ++index) {
                    const Order order = make_order(3, kBestBid - (12 * kTick), next_id++, kVolume, Side::BUY);
                    require(book.place_order(order, executions), OrderResult::SUCCESS, "cancel seed failed");
                    ids.push_back(order.order_id);
                }
                Sample result = measure([&] {
                    for (OrderId id : ids) {
                        require(book.cancel_order(id), OrderResult::SUCCESS, "cancel failed");
                    }
                }, kCancelBatchSize);
                const Sample overhead = measure([&] {
                    for (OrderId id : ids) {
                        g_sink += id;
                        std::atomic_signal_fence(std::memory_order_seq_cst);
                    }
                }, kCancelBatchSize);
                result.ns_per_operation = std::max(0.0, result.ns_per_operation - overhead.ns_per_operation);
                if (record) {
                    samples.push_back(result);
                }
            }
        };
        Trial result;
        result.samples.reserve(kMeasuredSamples);
        std::vector<Sample> ignored;
        run(false, ignored);
        run(true, result.samples);
        return result;
    });
}

Report benchmark_single_match() {
    return run_report("single_match", "Crossing buy fills exactly one best ask.", [](int trial) {
        Orderbook book(kConfig);
        ExecutionBuffer executions{kConfig.max_orders};
        seed(book, executions);
        OrderId next_id = 3'000'000 + (static_cast<OrderId>(trial) * 100'000);
        const auto run = [&](bool record, std::vector<Sample>& samples) {
            for (int sample = 0; sample < (record ? kMeasuredSamples : kWarmupSamples); ++sample) {
                const Order taker = make_order(3, kBestAsk, next_id++, kVolume, Side::BUY);
                const Sample result = measure([&] {
                    require(book.place_order(taker, executions), OrderResult::COMPLETE_FILL, "single match failed");
                }, 1);
                require(book.place_order(make_order(2, kBestAsk, 257, kVolume, Side::SELL), executions), OrderResult::SUCCESS, "single match restore failed");
                if (record) {
                    samples.push_back(result);
                }
            }
        };
        Trial result;
        result.samples.reserve(kMeasuredSamples);
        std::vector<Sample> ignored;
        run(false, ignored);
        run(true, result.samples);
        return result;
    });
}

Report benchmark_sweep() {
    return run_report("multi_level_sweep", "Crossing buy sweeps four ask levels.", [](int trial) {
        Orderbook book(kConfig);
        ExecutionBuffer executions{kConfig.max_orders};
        seed(book, executions);
        OrderId next_id = 4'000'000 + (static_cast<OrderId>(trial) * 100'000);
        const auto run = [&](bool record, std::vector<Sample>& samples) {
            for (int sample = 0; sample < (record ? kMeasuredSamples : kWarmupSamples); ++sample) {
                const Order taker = make_order(3, kBestAsk + (3 * kTick), next_id++, kVolume * 4, Side::BUY);
                const Sample result = measure([&] {
                    require(book.place_order(taker, executions), OrderResult::COMPLETE_FILL, "sweep failed");
                }, 1);
                for (int level = 0; level < 4; ++level) {
                    require(book.place_order(make_order(2, kBestAsk + (level * kTick), 257 + level, kVolume, Side::SELL), executions), OrderResult::SUCCESS, "sweep restore failed");
                }
                if (record) {
                    samples.push_back(result);
                }
            }
        };
        Trial result;
        result.samples.reserve(kMeasuredSamples);
        std::vector<Sample> ignored;
        run(false, ignored);
        run(true, result.samples);
        return result;
    });
}

Report benchmark_mixed() {
    return run_report("mixed_stream", "Synthetic stream: 55% add, 25% cancel, 15% single match, 5% four-level sweep.", [](int trial) {
        Orderbook book(kConfig);
        ExecutionBuffer executions{kConfig.max_orders};
        seed(book, executions);
        OrderId next_id = 5'000'000 + (static_cast<OrderId>(trial) * 100'000);
        const auto run = [&](bool record, std::vector<Sample>& samples) {
            for (int sample = 0; sample < (record ? kMeasuredSamples : kWarmupSamples); ++sample) {
                const int bucket = sample % 20;
                Sample result;
                if (bucket < 11) {
                    const Order order = make_order(3, kBestBid - (10 * kTick), next_id++, kVolume, Side::BUY);
                    result = measure([&] { require(book.place_order(order, executions), OrderResult::SUCCESS, "mixed add failed"); }, 1);
                    require(book.cancel_order(order.order_id), OrderResult::SUCCESS, "mixed add cleanup failed");
                } else if (bucket < 16) {
                    const Order order = make_order(3, kBestBid - (12 * kTick), next_id++, kVolume, Side::BUY);
                    require(book.place_order(order, executions), OrderResult::SUCCESS, "mixed cancel seed failed");
                    result = measure([&] { require(book.cancel_order(order.order_id), OrderResult::SUCCESS, "mixed cancel failed"); }, 1);
                } else if (bucket < 19) {
                    const Order order = make_order(3, kBestAsk, next_id++, kVolume, Side::BUY);
                    result = measure([&] { require(book.place_order(order, executions), OrderResult::COMPLETE_FILL, "mixed match failed"); }, 1);
                    require(book.place_order(make_order(2, kBestAsk, 257, kVolume, Side::SELL), executions), OrderResult::SUCCESS, "mixed match restore failed");
                } else {
                    const Order order = make_order(3, kBestAsk + (3 * kTick), next_id++, kVolume * 4, Side::BUY);
                    result = measure([&] { require(book.place_order(order, executions), OrderResult::COMPLETE_FILL, "mixed sweep failed"); }, 1);
                    for (int level = 0; level < 4; ++level) {
                        require(book.place_order(make_order(2, kBestAsk + (level * kTick), 257 + level, kVolume, Side::SELL), executions), OrderResult::SUCCESS, "mixed sweep restore failed");
                    }
                }
                if (record) {
                    samples.push_back(result);
                }
            }
        };
        Trial result;
        result.samples.reserve(kMeasuredSamples);
        std::vector<Sample> ignored;
        run(false, ignored);
        run(true, result.samples);
        return result;
    });
}

void print_report(const Report& report) {
    std::vector<double> latencies;
    std::vector<double> allocations;
    std::vector<double> deallocations;
    latencies.reserve(report.representative.samples.size());
    allocations.reserve(report.representative.samples.size());
    deallocations.reserve(report.representative.samples.size());
    for (const Sample& sample : report.representative.samples) {
        latencies.push_back(sample.ns_per_operation);
        allocations.push_back(sample.allocations_per_operation);
        deallocations.push_back(sample.deallocations_per_operation);
    }
    const auto [min_trial, max_trial] = std::minmax_element(report.trial_medians.begin(), report.trial_medians.end());
    const double median = percentile(latencies, 0.50);
    std::cout << report.name << '\n'
              << "  " << report.description << '\n'
              << "  median " << std::fixed << std::setprecision(1) << median << " ns, p99 " << percentile(latencies, 0.99)
              << " ns, p99.9 " << percentile(latencies, 0.999) << " ns\n"
              << "  throughput " << std::setprecision(0) << (1'000'000'000.0 / median) << " ops/sec\n"
              << "  allocations/op median " << percentile(allocations, 0.50) << ", max " << *std::max_element(allocations.begin(), allocations.end()) << '\n'
              << "  deallocations/op median " << percentile(deallocations, 0.50) << ", max " << *std::max_element(deallocations.begin(), deallocations.end()) << '\n'
              << "  trial median spread " << std::setprecision(1) << *min_trial << " .. " << *max_trial << " ns\n\n";
}

} // namespace

void* operator new(std::size_t size) {
    if (void* pointer = std::malloc(size)) {
        if (g_count_allocations) {
            g_allocations.fetch_add(1, std::memory_order_relaxed);
        }
        return pointer;
    }
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* pointer) noexcept {
    if (g_count_allocations && pointer) {
        g_deallocations.fetch_add(1, std::memory_order_relaxed);
    }
    std::free(pointer);
}
void operator delete[](void* pointer) noexcept { ::operator delete(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { ::operator delete(pointer); }
void operator delete[](void* pointer, std::size_t) noexcept { ::operator delete[](pointer); }

int main() {
    try {
        std::cout << "Order book benchmark\n"
                  << "7 trials; 1,000 warmup samples; 5,000 measured samples; 500-order warm book.\n"
                  << "All input orders, pools, price indexes, and execution buffers are allocated before timing.\n\n";
        print_report(benchmark_resting_add());
        print_report(benchmark_cancel_batch());
        print_report(benchmark_single_match());
        print_report(benchmark_sweep());
        print_report(benchmark_mixed());
    } catch (const std::exception& error) {
        std::cerr << "benchmark failed: " << error.what() << '\n';
        return 1;
    }
}
