#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <thread>
#include <vector>

#include "my_stl/containers/skip_list.h"

using Clock = std::chrono::high_resolution_clock;
using ns    = std::chrono::nanoseconds;

// ============================================================
// Latency collector: samples up to N_SAMPLES, then stops
// ============================================================

constexpr size_t MAX_SAMPLES = 1'000'000;

struct LatCollector {
    std::vector<int64_t> data;  // raw nanoseconds
    size_t dropped = 0;

    LatCollector() { data.reserve(MAX_SAMPLES); }

    void add(int64_t ns_val) {
        if (data.size() < MAX_SAMPLES)
            data.push_back(ns_val);
        else
            ++dropped;
    }

    void merge(const LatCollector& o) {
        data.insert(data.end(), o.data.begin(), o.data.end());
        dropped += o.dropped;
    }
};

// Percentile from already-sorted vector (returns nanoseconds)
static double pct(const std::vector<int64_t>& sorted, double p) {
    if (sorted.empty()) return 0;
    size_t i = static_cast<size_t>(sorted.size() * p / 100.0);
    if (i >= sorted.size()) i = sorted.size() - 1;
    return static_cast<double>(sorted[i]);
}

static std::string ns_str(double ns_val) {
    if (ns_val < 1000.0)
        return std::to_string(static_cast<int>(ns_val)) + "ns";
    if (ns_val < 1'000'000.0)
        return std::to_string(static_cast<int>(ns_val / 1000)) + "us";
    return std::to_string(static_cast<int>(ns_val / 1'000'000)) + "ms";
}

// ============================================================
// DB adapters — uniform interface: find(k,v), upsert(k,v)
// ============================================================

struct SkipListDB {
    MySTL::skip_list<int, int> sl;

    bool find(int k, int& v) const {
        const int* p = sl.find(k);
        if (p) { v = *p; return true; }
        return false;
    }
    void upsert(int k, int v)   { sl.upsert(k, v); }
    size_t size() const         { return sl.size(); }
};

struct MapMutexDB {
    std::map<int, int> map;
    mutable std::mutex  mtx;

    bool find(int k, int& v) const {
        std::lock_guard<std::mutex> lk(mtx);
        auto it = map.find(k);
        if (it != map.end()) { v = it->second; return true; }
        return false;
    }
    void upsert(int k, int v) {
        std::lock_guard<std::mutex> lk(mtx);
        map.insert_or_assign(k, v);
    }
    size_t size() const {
        std::lock_guard<std::mutex> lk(mtx);
        return map.size();
    }
};

struct MapSharedDB {
    std::map<int, int>        map;
    mutable std::shared_mutex mtx;

    bool find(int k, int& v) const {
        std::shared_lock<std::shared_mutex> lk(mtx);
        auto it = map.find(k);
        if (it != map.end()) { v = it->second; return true; }
        return false;
    }
    void upsert(int k, int v) {
        std::unique_lock<std::shared_mutex> lk(mtx);
        map.insert_or_assign(k, v);
    }
    size_t size() const {
        std::shared_lock<std::shared_mutex> lk(mtx);
        return map.size();
    }
};

// ============================================================
// Pre-generated key pool (faster than mt19937 in hot loop)
// ============================================================

static std::vector<int> make_key_pool(int n_keys, int seed) {
    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> dist(0, 99'999);   // hit pre-populated keys
    std::vector<int> pool(n_keys);
    for (auto& k : pool) k = dist(gen);
    return pool;
}

// ============================================================
// Benchmark runner
// ============================================================

template <typename DB>
struct BenchResult {
    double total_ops_s;
    double read_ops_s;
    double write_ops_s;
    double read_p50_ns, read_p95_ns, read_p99_ns, read_p999_ns;
};

template <typename DB>
static BenchResult<DB> bench(DB& db,
                              int duration_s,
                              int n_readers, int n_writers,
                              bool pre_populate)
{
    // ---------- Pre-populate ----------
    if (pre_populate) {
        for (int i = 0; i < 100'000; ++i)
            db.upsert(i, i);
    }

    std::atomic<bool>     stop{false};
    std::atomic<long>     read_cnt{0}, write_cnt{0};
    std::vector<LatCollector> read_lats(n_readers);
    std::vector<std::thread>  ths;

    // ---------- Reader threads ----------
    for (int t = 0; t < n_readers; ++t) {
        ths.emplace_back([&, t]() {
            auto pool = make_key_pool(MAX_SAMPLES, t + 17);
            LatCollector lc;
            size_t ki = 0;
            int dummy = 0;

            while (!stop.load(std::memory_order_relaxed)) {
                int k = pool[ki]; ki = (ki + 1) % pool.size();
                auto t0 = Clock::now();
                db.find(k, dummy);
                auto t1 = Clock::now();
                lc.add(std::chrono::duration_cast<ns>(t1 - t0).count());
                read_cnt.fetch_add(1, std::memory_order_relaxed);
            }
            read_lats[t] = std::move(lc);
        });
    }

    // ---------- Writer threads ----------
    std::atomic<int> next_key{100'000};
    for (int t = 0; t < n_writers; ++t) {
        ths.emplace_back([&]() {
            int val = 0;
            while (!stop.load(std::memory_order_relaxed)) {
                int k = next_key.fetch_add(1, std::memory_order_relaxed);
                db.upsert(k, val++);
                write_cnt.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    // ---------- Run ----------
    std::this_thread::sleep_for(std::chrono::seconds(duration_s));
    stop.store(true, std::memory_order_relaxed);
    for (auto& th : ths) th.join();

    // ---------- Aggregate latencies ----------
    LatCollector merged;
    for (auto& lc : read_lats) merged.merge(lc);
    std::sort(merged.data.begin(), merged.data.end());

    BenchResult<DB> r;
    r.total_ops_s  = (read_cnt + write_cnt) / static_cast<double>(duration_s);
    r.read_ops_s   = read_cnt / static_cast<double>(duration_s);
    r.write_ops_s  = write_cnt / static_cast<double>(duration_s);
    r.read_p50_ns  = pct(merged.data, 50);
    r.read_p95_ns  = pct(merged.data, 95);
    r.read_p99_ns  = pct(merged.data, 99);
    r.read_p999_ns = pct(merged.data, 99.9);
    return r;
}

// ============================================================
// Output helpers
// ============================================================

static void print_row(const char* label,
                       double total_ops, double read_ops, double write_ops,
                       double p50, double p95, double p99, double p999)
{
    std::cout << std::left
              << std::setw(18) << label
              << std::setw(14) << std::fixed << std::setprecision(0)
              << total_ops
              << std::setw(14) << read_ops
              << std::setw(14) << write_ops
              << std::setw(12) << ns_str(p50)
              << std::setw(12) << ns_str(p95)
              << std::setw(12) << ns_str(p99)
              << std::setw(12) << ns_str(p999)
              << "\n";
}

static void print_header() {
    std::cout << std::left
              << std::setw(18) << ""
              << std::setw(14) << "Total ops/s"
              << std::setw(14) << "Read ops/s"
              << std::setw(14) << "Write ops/s"
              << std::setw(12) << "P50"
              << std::setw(12) << "P95"
              << std::setw(12) << "P99"
              << std::setw(12) << "P99.9"
              << "\n";
    std::cout << std::string(110, '-') << "\n";
}

// ============================================================
// main
// ============================================================

int main() {
    std::cout << "=== Skip List vs std::map  Concurrent Benchmark ===\n";
    std::cout << "Pre-populated: 100K keys  |  Each scenario: fresh DB instance\n";
    std::cout << "Writers insert unique keys (no key conflict with readers)\n\n";

    // ============================
    // Scenario A: Read-heavy
    // ============================
    std::cout << "--- Scenario A: Read-heavy (4 readers + 1 writer, 3s) ---\n";
    print_header();

    {
        SkipListDB db;
        auto r = bench(db, 3, 4, 1, true);
        print_row("skip_list", r.total_ops_s, r.read_ops_s, r.write_ops_s,
                  r.read_p50_ns, r.read_p95_ns, r.read_p99_ns, r.read_p999_ns);
    }
    {
        MapSharedDB db;
        auto r = bench(db, 3, 4, 1, true);
        print_row("map+shared_mutex", r.total_ops_s, r.read_ops_s, r.write_ops_s,
                  r.read_p50_ns, r.read_p95_ns, r.read_p99_ns, r.read_p999_ns);
    }
    {
        MapMutexDB db;
        auto r = bench(db, 3, 4, 1, true);
        print_row("map+mutex", r.total_ops_s, r.read_ops_s, r.write_ops_s,
                  r.read_p50_ns, r.read_p95_ns, r.read_p99_ns, r.read_p999_ns);
    }
    std::cout << "\n";

    // ============================
    // Scenario B: Mixed read/write
    // ============================
    std::cout << "--- Scenario B: Mixed (4 readers + 2 writers, 3s) ---\n";
    print_header();

    {
        SkipListDB db;
        auto r = bench(db, 3, 4, 2, true);
        print_row("skip_list", r.total_ops_s, r.read_ops_s, r.write_ops_s,
                  r.read_p50_ns, r.read_p95_ns, r.read_p99_ns, r.read_p999_ns);
    }
    {
        MapSharedDB db;
        auto r = bench(db, 3, 4, 2, true);
        print_row("map+shared_mutex", r.total_ops_s, r.read_ops_s, r.write_ops_s,
                  r.read_p50_ns, r.read_p95_ns, r.read_p99_ns, r.read_p999_ns);
    }
    {
        MapMutexDB db;
        auto r = bench(db, 3, 4, 2, true);
        print_row("map+mutex", r.total_ops_s, r.read_ops_s, r.write_ops_s,
                  r.read_p50_ns, r.read_p95_ns, r.read_p99_ns, r.read_p999_ns);
    }
    std::cout << "\n";

    // ============================
    // Scenario C: Write scalability
    // ============================
    std::cout << "--- Scenario C: Write scalability (pre-populated 100K, 1s) ---\n";
    std::cout << std::left
              << std::setw(18) << ""
              << std::setw(16) << "1 writer"
              << std::setw(16) << "2 writers"
              << std::setw(16) << "4 writers"
              << "\n";
    std::cout << std::string(66, '-') << "\n";

    auto write_test = [](const char* label, auto make_db) {
        std::cout << std::left << std::setw(18) << label;
        for (int nw : {1, 2, 4}) {
            auto db = make_db();
            for (int i = 0; i < 100'000; ++i) db.upsert(i, i);
            auto r = bench(db, 1, 0, nw, false);  // no pre-pop (already done)
            std::cout << std::setw(16) << std::fixed << std::setprecision(0)
                      << r.write_ops_s;
        }
        std::cout << "\n";
    };

    write_test("skip_list",       []() { return SkipListDB{}; });
    write_test("map+shared_mutex",[]() { return MapSharedDB{}; });
    write_test("map+mutex",       []() { return MapMutexDB{}; });

    std::cout << "\nDone.\n";
    return 0;
}
