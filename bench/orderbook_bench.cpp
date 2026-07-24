#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <new>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../include/common/FixedPoint.h"
#include "../include/orderbook/Order.h"
#include "../include/orderbook/Orderbook.h"

namespace {

std::atomic<std::uint64_t> g_alloc_calls{0};
std::atomic<std::uint64_t> g_dealloc_calls{0};
thread_local bool g_count_allocations = false;
volatile std::uint64_t g_benchmark_sink = 0;

void* aligned_malloc(std::size_t alignment, std::size_t size) {
    void* ptr = nullptr;
    if (posix_memalign(&ptr, alignment, size) != 0) {
        throw std::bad_alloc();
    }
    return ptr;
}

struct AllocationCountingGuard {
    AllocationCountingGuard() { g_count_allocations = true; }
    ~AllocationCountingGuard() { g_count_allocations = false; }
};

struct AllocationSnapshot {
    std::uint64_t alloc_calls;
    std::uint64_t dealloc_calls;
};

AllocationSnapshot snapshot_allocations() {
    return AllocationSnapshot{
        g_alloc_calls.load(std::memory_order_relaxed),
        g_dealloc_calls.load(std::memory_order_relaxed),
    };
}

struct TimedSample {
    std::uint64_t raw_ns;
    std::uint64_t alloc_calls;
    std::uint64_t dealloc_calls;
};

using Clock = std::chrono::steady_clock;

template <typename Fn>
TimedSample measure_sample(Fn&& fn) {
    const AllocationSnapshot before = snapshot_allocations();
    const auto start = Clock::now();
    {
        AllocationCountingGuard guard;
        fn();
    }
    const auto end = Clock::now();
    const AllocationSnapshot after = snapshot_allocations();

    return TimedSample{
        static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()
        ),
        after.alloc_calls - before.alloc_calls,
        after.dealloc_calls - before.dealloc_calls,
    };
}

struct ScenarioConfig {
    static constexpr int kBidLevels = 64;
    static constexpr int kAskLevels = 64;
    static constexpr int kOrdersPerBidLevel = 4;
    static constexpr int kOrdersPerAskLevel = 4;
    static constexpr int kTopAskLevelsSingleOrder = 4;
    static constexpr int kSweepLevels = 4;
    static constexpr int kWarmupIterations = 5'000;
    static constexpr int kMeasuredIterations = 25'000;
    static constexpr int kTrials = 7;
    static constexpr int kRestingOrderVolume = 100;
    static constexpr int kSweepOrderVolume = kRestingOrderVolume * kSweepLevels;
    static constexpr int kInitialRestingOrders =
        (kBidLevels * kOrdersPerBidLevel) +
        (kTopAskLevelsSingleOrder * 1) +
        ((kAskLevels - kTopAskLevelsSingleOrder) * kOrdersPerAskLevel);
    static constexpr int kInitialPriceLevels = kBidLevels + kAskLevels;
    static constexpr std::int64_t kTickRaw = 100;      // $0.01
    static constexpr std::int64_t kBestBidRaw = 999'900;  // 99.99
    static constexpr std::int64_t kBestAskRaw = 1'000'100; // 100.01
};

enum class MixedOperationKind {
    AddResting,
    CancelExisting,
    SingleMatch,
    MultiLevelSweep,
};

struct SeededBook {
    trading::Orderbook book;
    std::array<trading::Order, ScenarioConfig::kSweepLevels> sweep_restore_orders;
    trading::Order single_match_restore_order;
    Price best_ask_price;
    Price resting_price;
    Price cancel_price;
};

struct TrialMetrics {
    std::vector<std::uint64_t> adjusted_ns;
    std::vector<std::uint64_t> alloc_calls;
    std::vector<std::uint64_t> dealloc_calls;
    std::uint64_t clock_overhead_ns = 0;
    std::uint64_t clock_p99_ns = 0;
    std::uint64_t clock_p999_ns = 0;
    std::uint64_t clock_quantum_ns = 0;
    double throughput_ops_per_sec = 0.0;
    double median_ns = 0.0;
    double p99_ns = 0.0;
    double p999_ns = 0.0;
    double median_allocs = 0.0;
    double p99_allocs = 0.0;
    std::uint64_t max_allocs = 0;
    double median_deallocs = 0.0;
    std::uint64_t max_deallocs = 0;
};

struct ScenarioReport {
    std::string name;
    std::string description;
    TrialMetrics reported_trial;
    std::vector<double> trial_medians_ns;
    std::vector<double> trial_throughputs;
    std::size_t reported_trial_index = 0;
};

double percentile(std::vector<std::uint64_t> values, double q) {
    if (values.empty()) {
        return 0.0;
    }

    std::sort(values.begin(), values.end());
    const double scaled = q * static_cast<double>(values.size() - 1);
    const auto lower = static_cast<std::size_t>(scaled);
    const auto upper = static_cast<std::size_t>(std::ceil(scaled));
    if (lower == upper) {
        return static_cast<double>(values[lower]);
    }

    const double fraction = scaled - static_cast<double>(lower);
    return static_cast<double>(values[lower]) +
        (static_cast<double>(values[upper]) - static_cast<double>(values[lower])) * fraction;
}

std::string format_ns(double ns) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << ns << " ns";
    return out.str();
}

std::string format_ops_per_sec(double ops_per_sec) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(0) << ops_per_sec;
    return out.str();
}

std::string format_trial_range(const std::vector<double>& values) {
    if (values.empty()) {
        return "n/a";
    }

    const auto [min_it, max_it] = std::minmax_element(values.begin(), values.end());
    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << *min_it << " .. " << *max_it;
    return out.str();
}

Price make_price(std::int64_t raw) {
    return Price::fromRaw(raw);
}

trading::Order make_order(
    const std::string& client,
    const Price& price,
    int order_id,
    int volume,
    trading::Side side
) {
    return trading::Order(
        client,
        price,
        order_id,
        volume,
        side,
        std::chrono::system_clock::time_point{}
    );
}

SeededBook build_seeded_book() {
    SeededBook seeded{
        trading::Orderbook(),
        std::array<trading::Order, ScenarioConfig::kSweepLevels>{},
        trading::Order(),
        make_price(ScenarioConfig::kBestAskRaw),
        make_price(ScenarioConfig::kBestBidRaw - (10 * ScenarioConfig::kTickRaw)),
        make_price(ScenarioConfig::kBestBidRaw - (12 * ScenarioConfig::kTickRaw)),
    };

    seeded.book.reserve_orders(ScenarioConfig::kInitialRestingOrders + 512);
    seeded.book.reserve_price_levels(ScenarioConfig::kInitialPriceLevels + 16);

    std::vector<trading::TradeInfo> trades;
    trades.reserve(ScenarioConfig::kSweepLevels + 4);

    int next_order_id = 1;

    for (int level = 0; level < ScenarioConfig::kBidLevels; ++level) {
        const Price price = make_price(
            ScenarioConfig::kBestBidRaw - (static_cast<std::int64_t>(level) * ScenarioConfig::kTickRaw)
        );
        for (int slot = 0; slot < ScenarioConfig::kOrdersPerBidLevel; ++slot) {
            trading::Order order = make_order("benchmark-market-making-participant", price, next_order_id++, ScenarioConfig::kRestingOrderVolume, trading::Side::BUY);
            trades.clear();
            const auto result = seeded.book.place_order(order, trades);
            if (result != trading::OrderResult::SUCCESS) {
                throw std::runtime_error("failed to seed bid side");
            }
        }
    }

    for (int level = 0; level < ScenarioConfig::kAskLevels; ++level) {
        const Price price = make_price(
            ScenarioConfig::kBestAskRaw + (static_cast<std::int64_t>(level) * ScenarioConfig::kTickRaw)
        );
        const int orders_this_level = level < ScenarioConfig::kTopAskLevelsSingleOrder
            ? 1
            : ScenarioConfig::kOrdersPerAskLevel;
        for (int slot = 0; slot < orders_this_level; ++slot) {
            trading::Order order = make_order("benchmark-market-making-participant", price, next_order_id++, ScenarioConfig::kRestingOrderVolume, trading::Side::SELL);
            if (level == 0 && slot == 0) {
                seeded.single_match_restore_order = order;
            }
            if (level < ScenarioConfig::kSweepLevels && slot == 0) {
                seeded.sweep_restore_orders[static_cast<std::size_t>(level)] = order;
            }
            trades.clear();
            const auto result = seeded.book.place_order(order, trades);
            if (result != trading::OrderResult::SUCCESS) {
                throw std::runtime_error("failed to seed ask side");
            }
        }
    }

    if (seeded.book.order_count() != ScenarioConfig::kInitialRestingOrders) {
        throw std::runtime_error("seeded order count mismatch");
    }
    if (seeded.book.price_level_count() != ScenarioConfig::kInitialPriceLevels) {
        throw std::runtime_error("seeded price level count mismatch");
    }

    return seeded;
}

std::vector<trading::Order> make_resting_add_orders(int total_iterations, const Price& price) {
    std::vector<trading::Order> orders;
    orders.reserve(total_iterations);
    for (int i = 0; i < total_iterations; ++i) {
        orders.push_back(make_order("benchmark-resting-buyer", price, 100'000 + i, ScenarioConfig::kRestingOrderVolume, trading::Side::BUY));
    }
    return orders;
}

std::vector<trading::Order> make_cancel_orders(int total_iterations, const Price& price) {
    std::vector<trading::Order> orders;
    orders.reserve(total_iterations);
    for (int i = 0; i < total_iterations; ++i) {
        orders.push_back(make_order("benchmark-cancel-buyer", price, 200'000 + i, ScenarioConfig::kRestingOrderVolume, trading::Side::BUY));
    }
    return orders;
}

std::vector<trading::Order> make_single_match_orders(int total_iterations, const Price& price) {
    std::vector<trading::Order> orders;
    orders.reserve(total_iterations);
    for (int i = 0; i < total_iterations; ++i) {
        orders.push_back(make_order("benchmark-single-match-buyer", price, 300'000 + i, ScenarioConfig::kRestingOrderVolume, trading::Side::BUY));
    }
    return orders;
}

std::vector<trading::Order> make_sweep_orders(int total_iterations, const Price& price) {
    std::vector<trading::Order> orders;
    orders.reserve(total_iterations);
    for (int i = 0; i < total_iterations; ++i) {
        orders.push_back(make_order("benchmark-sweep-match-buyer", price, 400'000 + i, ScenarioConfig::kSweepOrderVolume, trading::Side::BUY));
    }
    return orders;
}

std::vector<MixedOperationKind> make_mixed_schedule(int total_iterations) {
    std::vector<MixedOperationKind> schedule;
    schedule.reserve(total_iterations);

    for (int i = 0; i < total_iterations; ++i) {
        const int bucket = i % 20;
        if (bucket < 11) {
            schedule.push_back(MixedOperationKind::AddResting);
        } else if (bucket < 16) {
            schedule.push_back(MixedOperationKind::CancelExisting);
        } else if (bucket < 19) {
            schedule.push_back(MixedOperationKind::SingleMatch);
        } else {
            schedule.push_back(MixedOperationKind::MultiLevelSweep);
        }
    }

    return schedule;
}

std::vector<TimedSample> measure_clock_overhead(int measured_iterations) {
    std::vector<TimedSample> samples;
    samples.reserve(measured_iterations);
    for (int i = 0; i < measured_iterations; ++i) {
        samples.push_back(measure_sample([i]() {
            g_benchmark_sink += static_cast<std::uint64_t>(i);
            std::atomic_signal_fence(std::memory_order_seq_cst);
        }));
    }
    return samples;
}

template <typename WarmupFn, typename MeasureFn>
TrialMetrics run_trial(WarmupFn&& warmup_fn, MeasureFn&& measure_fn) {
    const auto overhead_samples = measure_clock_overhead(ScenarioConfig::kMeasuredIterations);

    std::vector<std::uint64_t> overhead_raw_ns;
    overhead_raw_ns.reserve(overhead_samples.size());
    for (const TimedSample& sample : overhead_samples) {
        overhead_raw_ns.push_back(sample.raw_ns);
    }
    const std::uint64_t clock_median_ns = static_cast<std::uint64_t>(percentile(overhead_raw_ns, 0.50));
    const std::uint64_t clock_p99_ns = static_cast<std::uint64_t>(percentile(overhead_raw_ns, 0.99));
    const std::uint64_t clock_p999_ns = static_cast<std::uint64_t>(percentile(overhead_raw_ns, 0.999));
    std::uint64_t clock_quantum_ns = 0;
    for (const std::uint64_t sample_ns : overhead_raw_ns) {
        if (sample_ns != 0 && (clock_quantum_ns == 0 || sample_ns < clock_quantum_ns)) {
            clock_quantum_ns = sample_ns;
        }
    }

    for (int i = 0; i < ScenarioConfig::kWarmupIterations; ++i) {
        warmup_fn(i);
    }

    TrialMetrics metrics;
    metrics.adjusted_ns.reserve(ScenarioConfig::kMeasuredIterations);
    metrics.alloc_calls.reserve(ScenarioConfig::kMeasuredIterations);
    metrics.dealloc_calls.reserve(ScenarioConfig::kMeasuredIterations);
    metrics.clock_overhead_ns = clock_median_ns;
    metrics.clock_p99_ns = clock_p99_ns;
    metrics.clock_p999_ns = clock_p999_ns;
    metrics.clock_quantum_ns = clock_quantum_ns;

    std::uint64_t total_adjusted_ns = 0;

    for (int i = 0; i < ScenarioConfig::kMeasuredIterations; ++i) {
        const TimedSample sample = measure_fn(i);
        const std::uint64_t adjusted_ns = sample.raw_ns > clock_median_ns
            ? sample.raw_ns - clock_median_ns
            : 0;

        metrics.adjusted_ns.push_back(adjusted_ns);
        metrics.alloc_calls.push_back(sample.alloc_calls);
        metrics.dealloc_calls.push_back(sample.dealloc_calls);
        total_adjusted_ns += adjusted_ns;
    }

    metrics.median_ns = percentile(metrics.adjusted_ns, 0.50);
    metrics.p99_ns = percentile(metrics.adjusted_ns, 0.99);
    metrics.p999_ns = percentile(metrics.adjusted_ns, 0.999);
    metrics.median_allocs = percentile(metrics.alloc_calls, 0.50);
    metrics.p99_allocs = percentile(metrics.alloc_calls, 0.99);
    metrics.max_allocs = *std::max_element(metrics.alloc_calls.begin(), metrics.alloc_calls.end());
    metrics.median_deallocs = percentile(metrics.dealloc_calls, 0.50);
    metrics.max_deallocs = *std::max_element(metrics.dealloc_calls.begin(), metrics.dealloc_calls.end());
    metrics.throughput_ops_per_sec = total_adjusted_ns == 0
        ? 0.0
        : (static_cast<double>(ScenarioConfig::kMeasuredIterations) * 1'000'000'000.0) /
            static_cast<double>(total_adjusted_ns);

    return metrics;
}

template <typename TrialRunner>
ScenarioReport run_scenario(const std::string& name, const std::string& description, TrialRunner&& trial_runner) {
    std::vector<TrialMetrics> trials;
    trials.reserve(ScenarioConfig::kTrials);

    for (int trial = 0; trial < ScenarioConfig::kTrials; ++trial) {
        trials.push_back(trial_runner());
    }

    std::vector<double> medians;
    std::vector<double> throughputs;
    medians.reserve(trials.size());
    throughputs.reserve(trials.size());
    for (const TrialMetrics& trial : trials) {
        medians.push_back(trial.median_ns);
        throughputs.push_back(trial.throughput_ops_per_sec);
    }

    std::vector<std::size_t> indices(trials.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&](std::size_t lhs, std::size_t rhs) {
        return medians[lhs] < medians[rhs];
    });
    const std::size_t reported_index = indices[indices.size() / 2];

    ScenarioReport report;
    report.name = name;
    report.description = description;
    report.reported_trial = trials[reported_index];
    report.trial_medians_ns = std::move(medians);
    report.trial_throughputs = std::move(throughputs);
    report.reported_trial_index = reported_index;
    return report;
}

void place_order_or_throw(trading::Orderbook& book, const trading::Order& order, std::vector<trading::TradeInfo>& trades) {
    trades.clear();
    const auto result = book.place_order(order, trades);
    if (result == trading::OrderResult::INVALID_ORDER || result == trading::OrderResult::DUPLICATE_ORDER_ID) {
        throw std::runtime_error("unexpected failure while placing order");
    }
}

void cancel_order_or_throw(trading::Orderbook& book, int order_id) {
    const auto result = book.cancel_order(order_id);
    if (result != trading::OrderResult::SUCCESS) {
        throw std::runtime_error("unexpected failure while cancelling order");
    }
}

ScenarioReport benchmark_add_resting(const std::vector<trading::Order>& orders) {
    return run_scenario(
        "add_resting",
        "Add a non-crossing buy order at an existing bid level, then cancel it outside the timed region.",
        [&]() {
            SeededBook seeded = build_seeded_book();
            std::vector<trading::TradeInfo> trades;
            trades.reserve(4);

            return run_trial(
                [&](int i) {
                    const trading::Order& order = orders[static_cast<std::size_t>(i)];
                    place_order_or_throw(seeded.book, order, trades);
                    cancel_order_or_throw(seeded.book, order.get_order_id());
                },
                [&](int i) {
                    const trading::Order& order = orders[static_cast<std::size_t>(ScenarioConfig::kWarmupIterations + i)];
                    trades.clear();
                    const TimedSample sample = measure_sample([&]() {
                        const auto result = seeded.book.place_order(order, trades);
                        if (result != trading::OrderResult::SUCCESS) {
                            throw std::runtime_error("add resting scenario did not rest");
                        }
                    });
                    cancel_order_or_throw(seeded.book, order.get_order_id());
                    return sample;
                }
            );
        }
    );
}

ScenarioReport benchmark_cancel_existing(const std::vector<trading::Order>& orders) {
    return run_scenario(
        "cancel_existing",
        "Cancel an already-resting order appended to an existing bid level.",
        [&]() {
            SeededBook seeded = build_seeded_book();
            std::vector<trading::TradeInfo> trades;
            trades.reserve(4);

            return run_trial(
                [&](int i) {
                    const trading::Order& order = orders[static_cast<std::size_t>(i)];
                    place_order_or_throw(seeded.book, order, trades);
                    cancel_order_or_throw(seeded.book, order.get_order_id());
                },
                [&](int i) {
                    const trading::Order& order = orders[static_cast<std::size_t>(ScenarioConfig::kWarmupIterations + i)];
                    place_order_or_throw(seeded.book, order, trades);
                    return measure_sample([&]() {
                        const auto result = seeded.book.cancel_order(order.get_order_id());
                        if (result != trading::OrderResult::SUCCESS) {
                            throw std::runtime_error("cancel scenario failed");
                        }
                    });
                }
            );
        }
    );
}

ScenarioReport benchmark_single_match(const std::vector<trading::Order>& aggressive_orders) {
    return run_scenario(
        "single_match",
        "Add a crossing buy order that fills exactly one resting ask at the best price.",
        [&]() {
            SeededBook seeded = build_seeded_book();
            std::vector<trading::TradeInfo> trades;
            trades.reserve(4);

            return run_trial(
                [&](int i) {
                    const trading::Order& aggressive = aggressive_orders[static_cast<std::size_t>(i)];
                    trades.clear();
                    const auto result = seeded.book.place_order(aggressive, trades);
                    if (result != trading::OrderResult::COMPLETE_FILL) {
                        throw std::runtime_error("single match warmup did not fully fill");
                    }
                    place_order_or_throw(seeded.book, seeded.single_match_restore_order, trades);
                },
                [&](int i) {
                    const trading::Order& aggressive = aggressive_orders[static_cast<std::size_t>(ScenarioConfig::kWarmupIterations + i)];
                    trades.clear();
                    const TimedSample sample = measure_sample([&]() {
                        const auto result = seeded.book.place_order(aggressive, trades);
                        if (result != trading::OrderResult::COMPLETE_FILL) {
                            throw std::runtime_error("single match scenario did not fully fill");
                        }
                    });
                    place_order_or_throw(seeded.book, seeded.single_match_restore_order, trades);
                    return sample;
                }
            );
        }
    );
}

ScenarioReport benchmark_multi_level_sweep(const std::vector<trading::Order>& aggressive_orders) {
    return run_scenario(
        "multi_level_sweep",
        "Add a crossing buy order that sweeps four ask levels.",
        [&]() {
            SeededBook seeded = build_seeded_book();
            std::vector<trading::TradeInfo> trades;
            trades.reserve(ScenarioConfig::kSweepLevels + 4);

            auto restore_swept_levels = [&]() {
                for (const trading::Order& order : seeded.sweep_restore_orders) {
                    place_order_or_throw(seeded.book, order, trades);
                }
            };

            return run_trial(
                [&](int i) {
                    const trading::Order& aggressive = aggressive_orders[static_cast<std::size_t>(i)];
                    trades.clear();
                    const auto result = seeded.book.place_order(aggressive, trades);
                    if (result != trading::OrderResult::COMPLETE_FILL) {
                        throw std::runtime_error("multi-level sweep warmup did not fully fill");
                    }
                    restore_swept_levels();
                },
                [&](int i) {
                    const trading::Order& aggressive = aggressive_orders[static_cast<std::size_t>(ScenarioConfig::kWarmupIterations + i)];
                    trades.clear();
                    const TimedSample sample = measure_sample([&]() {
                        const auto result = seeded.book.place_order(aggressive, trades);
                        if (result != trading::OrderResult::COMPLETE_FILL) {
                            throw std::runtime_error("multi-level sweep scenario did not fully fill");
                        }
                    });
                    restore_swept_levels();
                    return sample;
                }
            );
        }
    );
}

ScenarioReport benchmark_mixed_stream(
    const std::vector<MixedOperationKind>& schedule,
    const std::vector<trading::Order>& add_orders,
    const std::vector<trading::Order>& cancel_orders,
    const std::vector<trading::Order>& single_match_orders,
    const std::vector<trading::Order>& sweep_orders
) {
    return run_scenario(
        "mixed_stream",
        "Synthetic mixed stream: 55% add resting, 25% cancel existing, 15% single-level match, 5% four-level sweep.",
        [&]() {
            SeededBook seeded = build_seeded_book();
            std::vector<trading::TradeInfo> trades;
            trades.reserve(ScenarioConfig::kSweepLevels + 4);

            auto run_operation = [&](MixedOperationKind kind, int index) -> TimedSample {
                switch (kind) {
                    case MixedOperationKind::AddResting: {
                        const trading::Order& order = add_orders[static_cast<std::size_t>(index)];
                        trades.clear();
                        const TimedSample sample = measure_sample([&]() {
                            const auto result = seeded.book.place_order(order, trades);
                            if (result != trading::OrderResult::SUCCESS) {
                                throw std::runtime_error("mixed add scenario failed");
                            }
                        });
                        cancel_order_or_throw(seeded.book, order.get_order_id());
                        return sample;
                    }
                    case MixedOperationKind::CancelExisting: {
                        const trading::Order& order = cancel_orders[static_cast<std::size_t>(index)];
                        place_order_or_throw(seeded.book, order, trades);
                        return measure_sample([&]() {
                            const auto result = seeded.book.cancel_order(order.get_order_id());
                            if (result != trading::OrderResult::SUCCESS) {
                                throw std::runtime_error("mixed cancel scenario failed");
                            }
                        });
                    }
                    case MixedOperationKind::SingleMatch: {
                        const trading::Order& aggressive = single_match_orders[static_cast<std::size_t>(index)];
                        trades.clear();
                        const TimedSample sample = measure_sample([&]() {
                            const auto result = seeded.book.place_order(aggressive, trades);
                            if (result != trading::OrderResult::COMPLETE_FILL) {
                                throw std::runtime_error("mixed single-match scenario failed");
                            }
                        });
                        place_order_or_throw(seeded.book, seeded.single_match_restore_order, trades);
                        return sample;
                    }
                    case MixedOperationKind::MultiLevelSweep: {
                        const trading::Order& aggressive = sweep_orders[static_cast<std::size_t>(index)];
                        trades.clear();
                        const TimedSample sample = measure_sample([&]() {
                            const auto result = seeded.book.place_order(aggressive, trades);
                            if (result != trading::OrderResult::COMPLETE_FILL) {
                                throw std::runtime_error("mixed sweep scenario failed");
                            }
                        });
                        for (const trading::Order& order : seeded.sweep_restore_orders) {
                            place_order_or_throw(seeded.book, order, trades);
                        }
                        return sample;
                    }
                }

                throw std::runtime_error("unknown mixed operation");
            };

            return run_trial(
                [&](int i) {
                    (void)run_operation(schedule[static_cast<std::size_t>(i)], i);
                },
                [&](int i) {
                    const int op_index = ScenarioConfig::kWarmupIterations + i;
                    return run_operation(schedule[static_cast<std::size_t>(op_index)], op_index);
                }
            );
        }
    );
}

void print_report(const SeededBook& seeded_template, const std::vector<ScenarioReport>& reports) {
    std::cout << "Order book benchmark\n";
    std::cout << "====================\n";
    std::cout << "Steady-state warm book:\n";
    std::cout << "  bid levels: " << ScenarioConfig::kBidLevels << "\n";
    std::cout << "  ask levels: " << ScenarioConfig::kAskLevels << "\n";
    std::cout << "  total resting orders: " << ScenarioConfig::kInitialRestingOrders << "\n";
    std::cout << "  top ask levels reserved for sweep/single-match reproducibility: "
              << ScenarioConfig::kTopAskLevelsSingleOrder << " (one order each)\n";
    std::cout << "  resting add price: " << seeded_template.resting_price.to_string() << "\n";
    std::cout << "  cancel price: " << seeded_template.cancel_price.to_string() << "\n";
    std::cout << "  best ask: " << seeded_template.best_ask_price.to_string() << "\n";
    std::cout << "  trials: " << ScenarioConfig::kTrials
              << ", warmup iterations: " << ScenarioConfig::kWarmupIterations
              << ", measured iterations: " << ScenarioConfig::kMeasuredIterations << "\n";
    std::cout << "\n";

    for (const ScenarioReport& report : reports) {
        const TrialMetrics& trial = report.reported_trial;
        std::cout << report.name << "\n";
        std::cout << "  " << report.description << "\n";
        std::cout << "  reported trial: " << (report.reported_trial_index + 1) << "/" << ScenarioConfig::kTrials
                  << " (median of trial medians)\n";
        std::cout << "  latency: median " << format_ns(trial.median_ns)
                  << ", p99 " << format_ns(trial.p99_ns)
                  << ", p99.9 " << format_ns(trial.p999_ns);
        if (trial.clock_quantum_ns != 0 && trial.median_ns <= static_cast<double>(trial.clock_quantum_ns)) {
            std::cout << " (at or below local timer quantum)";
        }
        std::cout << "\n";
        std::cout << "  throughput: " << format_ops_per_sec(trial.throughput_ops_per_sec) << " ops/sec\n";
        std::cout << "  alloc calls/op: median " << trial.median_allocs
                  << ", p99 " << trial.p99_allocs
                  << ", max " << trial.max_allocs << "\n";
        std::cout << "  dealloc calls/op: median " << trial.median_deallocs
                  << ", max " << trial.max_deallocs << "\n";
        std::cout << "  clock overhead: median " << trial.clock_overhead_ns
                  << " ns, p99 " << trial.clock_p99_ns
                  << " ns, p99.9 " << trial.clock_p999_ns
                  << " ns, timer quantum ~" << trial.clock_quantum_ns << " ns\n";
        std::cout << "  trial median spread: " << format_trial_range(report.trial_medians_ns) << " ns\n";
        std::cout << "  trial throughput spread: " << format_trial_range(report.trial_throughputs) << " ops/sec\n";
        std::cout << "\n";
    }
}

} // namespace

void* operator new(std::size_t size) {
    if (void* ptr = std::malloc(size)) {
        if (g_count_allocations) {
            g_alloc_calls.fetch_add(1, std::memory_order_relaxed);
        }
        return ptr;
    }
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) {
    return ::operator new(size);
}

void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    try {
        return ::operator new(size);
    } catch (...) {
        return nullptr;
    }
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    try {
        return ::operator new[](size);
    } catch (...) {
        return nullptr;
    }
}

void* operator new(std::size_t size, std::align_val_t alignment) {
    void* ptr = aligned_malloc(static_cast<std::size_t>(alignment), size);
    if (g_count_allocations) {
        g_alloc_calls.fetch_add(1, std::memory_order_relaxed);
    }
    return ptr;
}

void* operator new[](std::size_t size, std::align_val_t alignment) {
    return ::operator new(size, alignment);
}

void operator delete(void* ptr) noexcept {
    if (!ptr) {
        return;
    }
    if (g_count_allocations) {
        g_dealloc_calls.fetch_add(1, std::memory_order_relaxed);
    }
    std::free(ptr);
}

void operator delete[](void* ptr) noexcept {
    ::operator delete(ptr);
}

void operator delete(void* ptr, std::size_t) noexcept {
    ::operator delete(ptr);
}

void operator delete[](void* ptr, std::size_t) noexcept {
    ::operator delete[](ptr);
}

void operator delete(void* ptr, std::align_val_t) noexcept {
    ::operator delete(ptr);
}

void operator delete[](void* ptr, std::align_val_t) noexcept {
    ::operator delete[](ptr);
}

void operator delete(void* ptr, std::size_t, std::align_val_t) noexcept {
    ::operator delete(ptr);
}

void operator delete[](void* ptr, std::size_t, std::align_val_t) noexcept {
    ::operator delete[](ptr);
}

int main() {
    try {
        const int total_iterations = ScenarioConfig::kWarmupIterations + ScenarioConfig::kMeasuredIterations;
        SeededBook seeded_template = build_seeded_book();

        const auto add_orders = make_resting_add_orders(total_iterations, seeded_template.resting_price);
        const auto cancel_orders = make_cancel_orders(total_iterations, seeded_template.cancel_price);
        const auto single_match_orders = make_single_match_orders(total_iterations, seeded_template.best_ask_price);
        const auto sweep_orders = make_sweep_orders(
            total_iterations,
            make_price(ScenarioConfig::kBestAskRaw + ((ScenarioConfig::kSweepLevels - 1) * ScenarioConfig::kTickRaw))
        );
        const auto mixed_schedule = make_mixed_schedule(total_iterations);

        std::vector<ScenarioReport> reports;
        reports.reserve(5);
        reports.push_back(benchmark_add_resting(add_orders));
        reports.push_back(benchmark_cancel_existing(cancel_orders));
        reports.push_back(benchmark_single_match(single_match_orders));
        reports.push_back(benchmark_multi_level_sweep(sweep_orders));
        reports.push_back(benchmark_mixed_stream(
            mixed_schedule,
            add_orders,
            cancel_orders,
            single_match_orders,
            sweep_orders
        ));

        print_report(seeded_template, reports);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "benchmark failed: " << error.what() << "\n";
        return 1;
    }
}
