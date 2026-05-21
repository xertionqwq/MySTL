#!/bin/bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
BENCH_BIN="$BUILD_DIR/bench/bench_allocator"
COUNTER_SO="$PROJECT_DIR/bench/libmalloc_counter.so"

echo "=== Building ==="
cd "$BUILD_DIR"
cmake .. > /dev/null 2>&1
cmake --build . --target bench_allocator -j"$(nproc)" 2>&1 | tail -3

# Build malloc counter if needed
if [ ! -f "$COUNTER_SO" ]; then
    gcc -shared -fPIC -o "$COUNTER_SO" "$PROJECT_DIR/bench/malloc_counter.c" -ldl
fi

echo ""
echo "============================================="
echo "  1/3  Timing + RSS (in-program)"
echo "============================================="
echo ""
"$BENCH_BIN"

echo ""
echo "============================================="
echo "  2/3  Peak RSS via /usr/bin/time -v"
echo "============================================="
/usr/bin/time -v "$BENCH_BIN" 2>&1 | grep -E "Maximum resident|Minor page|Major page|System time|Elapsed"

echo ""
echo "============================================="
echo "  3/3  malloc/free count via LD_PRELOAD"
echo "============================================="
LD_PRELOAD="$COUNTER_SO" "$BENCH_BIN" 2>&1 | grep -E "\[malloc_counter\]"

echo ""
echo "Heap XML dumps: /tmp/bench_heap_*.xml"
