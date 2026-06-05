#include <chrono>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#if defined(__linux__)
#include <malloc.h>
#include <unistd.h>
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#endif

#include "my_stl/containers/vector.h"

#if defined(_MSC_VER)
#define MYSTL_NOINLINE __declspec(noinline)
#else
#define MYSTL_NOINLINE __attribute__((noinline))
#endif

// ============================================================
// Test structs — 10 sizes → freelist buckets 8..128
// ============================================================

struct S8   { int v{}; char pad[4]{};  };
struct S16  { int v{}; char pad[12]{}; };
struct S24  { int v{}; char pad[20]{}; };
struct S32  { int v{}; char pad[28]{}; };
struct S48  { int v{}; char pad[44]{}; };
struct S64  { int v{}; char pad[60]{}; };
struct S80  { int v{}; char pad[76]{}; };
struct S96  { int v{}; char pad[92]{}; };
struct S112 { int v{}; char pad[108]{}; };
struct S128 { int v{}; char pad[124]{}; };

// ============================================================
// Compiler barrier — prevents dead-code elimination at -O2
// Without this the compiler can eliminate malloc/free pairs
// and dead stores, making the benchmark meaningless.
// ============================================================

static long g_sink = 0;  // volatile-like: write visible to external observers

template <typename T>
MYSTL_NOINLINE static void touch_vec(const T& v) {
    // Sum element v fields so the compiler must materialize the data.
    // noinline prevents the compiler from seeing into this call.
    for (std::size_t i = 0; i < v.size(); ++i)
        g_sink += static_cast<long>(v[i].v);
}

// ============================================================
// Timing-only benchmark (no instrumentation in hot loop)
// ============================================================

using Clock = std::chrono::high_resolution_clock;

struct Row { const char* name; std::size_t size; double t_mystl; double t_std; };

template <typename T>
static double run_my_stl(int batches, int per_batch) {
    auto t0 = Clock::now();
    for (int b = 0; b < batches; ++b) {
        MySTL::vector<T> v;
        for (int i = 0; i < per_batch; ++i)
            v.push_back(T{i});
        touch_vec(v);
    }
    return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
}

template <typename T>
static double run_std(int batches, int per_batch) {
    auto t0 = Clock::now();
    for (int b = 0; b < batches; ++b) {
        std::vector<T> v;
        for (int i = 0; i < per_batch; ++i)
            v.push_back(T{i});
        touch_vec(v);
    }
    return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
}

// ============================================================
// RSS from /proc/self/statm (sampled before/after each phase)
// ============================================================

static long rss_kb() {
#if defined(__linux__)
    std::ifstream f("/proc/self/statm");
    long rss = 0;
    f.ignore(32, ' ');      // skip "size" field
    f >> rss;
    return rss * static_cast<long>(sysconf(_SC_PAGESIZE)) / 1024;
#elif defined(_WIN32)
    PROCESS_MEMORY_COUNTERS pmc{};
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        return static_cast<long>(pmc.WorkingSetSize / 1024);
    return -1;
#else
    return -1;
#endif
}

// ============================================================
// Heap dump via malloc_info (glibc) — for manual inspection
// ============================================================

static void heap_dump(const char* label) {
#if defined(__linux__)
    char path[128];
    std::snprintf(path, sizeof(path), "/tmp/bench_heap_%s.xml", label);
    FILE* f = std::fopen(path, "w");
    if (f) { malloc_info(0, f); std::fclose(f); }
#else
    (void)label;
#endif
}

// ============================================================
// main
// ============================================================

int main() {
    constexpr int BATCHES   = 10'000;
    constexpr int PER_BATCH = 10;
    constexpr long TOTAL    = 10L * BATCHES * PER_BATCH;

    // ---- warmup (excluded from results) ----
    run_my_stl<S8>(1000, 10);
    run_std<S8>(1000, 10);

    // ========================================
    // Phase 1 — MySTL allocator (all 10 types)
    // ========================================
    long rss1_before = rss_kb();
    heap_dump("mystl_before");

    Row rows[] = {
        {"S8  (~8B)",   sizeof(S8),   run_my_stl<S8>  (BATCHES, PER_BATCH), 0},
        {"S16 (~16B)",  sizeof(S16),  run_my_stl<S16> (BATCHES, PER_BATCH), 0},
        {"S24 (~24B)",  sizeof(S24),  run_my_stl<S24> (BATCHES, PER_BATCH), 0},
        {"S32 (~32B)",  sizeof(S32),  run_my_stl<S32> (BATCHES, PER_BATCH), 0},
        {"S48 (~48B)",  sizeof(S48),  run_my_stl<S48> (BATCHES, PER_BATCH), 0},
        {"S64 (~64B)",  sizeof(S64),  run_my_stl<S64> (BATCHES, PER_BATCH), 0},
        {"S80 (~80B)",  sizeof(S80),  run_my_stl<S80> (BATCHES, PER_BATCH), 0},
        {"S96 (~96B)",  sizeof(S96),  run_my_stl<S96> (BATCHES, PER_BATCH), 0},
        {"S112(~112B)", sizeof(S112), run_my_stl<S112>(BATCHES, PER_BATCH), 0},
        {"S128(~128B)", sizeof(S128), run_my_stl<S128>(BATCHES, PER_BATCH), 0},
    };

    long rss1_after = rss_kb();
    long rss_mystl = rss1_after - rss1_before;
    heap_dump("mystl_after");

    // ========================================
    // Phase 2 — std::allocator (all 10 types)
    // ========================================
    long rss2_before = rss_kb();
    heap_dump("std_before");

    rows[0].t_std = run_std<S8>  (BATCHES, PER_BATCH);
    rows[1].t_std = run_std<S16> (BATCHES, PER_BATCH);
    rows[2].t_std = run_std<S24> (BATCHES, PER_BATCH);
    rows[3].t_std = run_std<S32> (BATCHES, PER_BATCH);
    rows[4].t_std = run_std<S48> (BATCHES, PER_BATCH);
    rows[5].t_std = run_std<S64> (BATCHES, PER_BATCH);
    rows[6].t_std = run_std<S80> (BATCHES, PER_BATCH);
    rows[7].t_std = run_std<S96> (BATCHES, PER_BATCH);
    rows[8].t_std = run_std<S112>(BATCHES, PER_BATCH);
    rows[9].t_std = run_std<S128>(BATCHES, PER_BATCH);

    long rss2_after = rss_kb();
    long rss_std = rss2_after - rss2_before;
    heap_dump("std_after");

    // ========================================
    // Output
    // ========================================
    std::cout << "=== MySTL allocator  vs  std::allocator (malloc) ===\n";
    std::cout << "Workload:  " << TOTAL
              << " push_back insertions\n";
    std::cout << "           " << "10 struct types  x  "
              << BATCHES << " vectors  x  " << PER_BATCH << " elems\n\n";

    std::cout << std::left
              << std::setw(15) << "Struct"
              << std::setw(7)  << "Size"
              << std::setw(14) << "MySTL(ms)"
              << std::setw(14) << "std(ms)"
              << std::setw(10) << "Speedup"
              << "\n";
    std::cout << std::string(60, '-') << "\n";

    double total_my = 0, total_std = 0;
    for (const auto& r : rows) {
        double sp = (r.t_std > 0.001) ? r.t_std / r.t_mystl : 0.0;
        total_my  += r.t_mystl;
        total_std += r.t_std;
        std::cout << std::left  << std::setw(15) << r.name
                  << std::setw(7)  << r.size
                  << std::setw(14) << std::fixed << std::setprecision(2) << r.t_mystl
                  << std::setw(14) << r.t_std
                  << std::setw(9)  << std::setprecision(2) << sp << "x"
                  << "\n";
    }

    double sp_total = (total_std > 0.001) ? total_std / total_my : 0.0;
    std::cout << std::string(60, '-') << "\n";
    std::cout << std::left  << std::setw(15) << "TOTAL"
              << std::setw(7)  << ""
              << std::setw(14) << std::fixed << std::setprecision(2) << total_my
              << std::setw(14) << total_std
              << std::setw(9)  << std::setprecision(2) << sp_total << "x"
              << "\n\n";

    // ---- RSS ----
    long rss_advantage = rss_std - rss_mystl;
    std::cout << "--- Memory (RSS per phase) ---\n";
    std::cout << "  MySTL phase delta:  " << rss_mystl << " KB\n";
    std::cout << "  std   phase delta:  " << rss_std   << " KB\n";
    std::cout << "  RSS saved:          " << rss_advantage << " KB";
    if (rss_advantage > 0)
        std::cout << "  (MySTL uses less)\n";
    else
        std::cout << "  (std uses less)\n";

    // Prove work was done (prevents whole-program elimination of g_sink)
    std::cout << "(checksum: " << g_sink << ")\n";

    // ---- Syscall hint ----
    std::cout << "\n--- System calls ---\n";
    std::cout << "  strace -c -e trace=memory  ./bench_allocator\n";
    std::cout << "  For heap analysis:         /tmp/bench_heap_*.xml\n";

    return 0;
}
