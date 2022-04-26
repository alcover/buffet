#include <benchmark/benchmark.h>
#include "utilcpp.h"
extern "C" {
	#include "buffet.h"
	#include "util.h"
	#include "log.h"
}
//=============================================================================
const char alpha[] = 
"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+="
"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+=";
const size_t alphalen = sizeof(alpha);

#define SPC " "
#define SEP SPC 
const char *sep = SEP;

const char *SPLITME =
"a" SEP "b" SEP "c" SEP "d" SEP "e" SEP "f" SEP "g" SEP "h" SEP \
"a" SEP "b" SEP "c" SEP "d" SEP "e" SEP "f" SEP "g" SEP "h" SEP \
"a" SEP "b" SEP "c" SEP "d" SEP "e" SEP "f" SEP "g" SEP "h" SEP \
"a" SEP "b" SEP "c" SEP "d" SEP "e" SEP "f" SEP "g" SEP "h" SEP \
"aa" SEP "bb" SEP "cc" SEP "dd" SEP "ee" SEP "ff" SEP "gg" SEP "hh" SEP \
"aa" SEP "bb" SEP "cc" SEP "dd" SEP "ee" SEP "ff" SEP "gg" SEP "hh" SEP \
"aaaa" SEP "bbbb" SEP "cccc" SEP "dddd" SEP \
"aaaa" SEP "bbbb" SEP "cccc" SEP "dddd" SEP \
"aaaaaaaa" SEP "bbbbbbbb" SEP "cccccccc" SEP "dddddddd" SEP \
"aaaaaaaa" SEP "bbbbbbbb" SEP "cccccccc" SEP "dddddddd" SEP \
"aaaaaaaaaaaaaaaa" SEP "bbbbbbbbbbbbbbbb" SEP \
"aaaaaaaaaaaaaaaa" SEP "bbbbbbbbbbbbbbbb" SEP \
"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" SEP "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb" SEP \
"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" SEP "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb" SEP;

static char* repeat(size_t n) {
    char *ret = malloc(n+1);
    memset(ret,'A',n); ret[n] = 0;
    return ret;
} 

//=============================================================================
static void
MEMCOPY_plainc (benchmark::State& state) 
{
    size_t len = state.range(0);
    for (auto _ : state) {
        char* buf = malloc(len+1);
        // benchmark::DoNotOptimize(buf);
        memcpy(buf, alpha, len);
        benchmark::DoNotOptimize(buf[len] = 0);
        free(buf);
    }
}
static void
MEMCOPY_buffet (benchmark::State& state) 
{
    size_t len = state.range(0);
    for (auto _ : state) {
        Buffet buf = buffet_memcopy(alpha, len);
        // benchmark::DoNotOptimize(buf);
        buffet_free(&buf);
    }
}
static void
MEMVIEW_cppview (benchmark::State& state) 
{
    size_t len = state.range(0);
    for (auto _ : state) {
        auto buf = string_view(alpha, len);
        benchmark::DoNotOptimize(buf);
        assert(buf[len-1]==alpha[len-1]);
    }
}
static void
MEMVIEW_buffet (benchmark::State& state) 
{
    size_t len = state.range(0);
    for (auto _ : state) {
        Buffet buf = buffet_memview(alpha, len);
        benchmark::DoNotOptimize(buf);
        assert(buffet_data(&buf)[len-1]==alpha[len-1]);
        // buffet_free(&buf);
    }
}
//=============================================================================
static void 
SPLITJOIN_plainc (benchmark::State& state) 
{
    for (auto _ : state) {
        int cnt = 0;
        char** parts = split(SPLITME, sep, &cnt);
        const char* ret = join(parts, cnt, sep);

        assert(!strcmp(ret, SPLITME));
        free(ret);
        for (int i = 0; i < cnt; ++i) free(parts[i]);
        free(parts);
    }
}

static void 
SPLITJOIN_cppview (benchmark::State& state) 
{
    for (auto _ : state) {
        vector<string_view> parts = split_cppview(SPLITME, sep);
        const char* ret = join_cppview(parts, sep);

        assert(!strcmp(ret, SPLITME));
        free(ret);
    }
}

static void 
SPLITJOIN_buffet (benchmark::State& state) 
{
    for (auto _ : state) {
        int cnt = 0;
        Buffet *parts = buffet_splitstr(SPLITME, sep, &cnt);
        Buffet back = buffet_join(parts, cnt, sep, strlen(sep));
        const char *ret = buffet_data(&back);

        assert(!strcmp(ret, SPLITME));
        buffet_free(&back);
        free(parts);
    }
}
 
//=============================================================================
#define APPEND_ITER 100000
#define APPEND_INIT  \
	const size_t srclen = state.range(0); \
	const char *src = repeat(srclen); \
	const int iter = APPEND_ITER; \
	const char *exp = repeat(iter*srclen);

static void 
APPEND_cpp (benchmark::State& state) 
{
	APPEND_INIT
    // faster and fairer than plain string (Justas Masiulis)
    string_view srcv = string_view(src, srclen);
    
    for (auto _ : state) {
    	string dst;
        for (int i=0; i<iter; ++i) dst += srcv;
        
        const char *ret = dst.data();
        benchmark::DoNotOptimize(ret); // useless ?
        assert(!strcmp(ret, exp));
    }
	
	free(src);
	free(exp);
}

static void 
APPEND_buffet (benchmark::State& state) 
{
	APPEND_INIT
    
    for (auto _ : state) {
    	Buffet dst = buffet_new(0);
        for (int i=0; i<iter; ++i) buffet_append (&dst, src, srclen);
        
        const char *ret = buffet_data(&dst);
        benchmark::DoNotOptimize(ret);
        assert(!strcmp(ret, exp));
        buffet_free(&dst);
    }
	
	free(src);
	free(exp);
}

//=====================================================================

#define BEG 1
#define END 64
#define MUL 8

// BENCHMARK(MEMCOPY_plainc)->RangeMultiplier(4)->Range(1,64);
// BENCHMARK(MEMCOPY_buffet)->RangeMultiplier(4)->Range(1,64);

BENCHMARK(MEMVIEW_cppview)->RangeMultiplier(4)->Range(1,16);
BENCHMARK(MEMVIEW_buffet)->RangeMultiplier(4)->Range(1,16);

// BENCHMARK(SPLITJOIN_plainc);
// BENCHMARK(SPLITJOIN_cppview);
// BENCHMARK(SPLITJOIN_buffet);

// BENCHMARK(APPEND_cpp)->RangeMultiplier(MUL)->Range(BEG,END)->Unit(benchmark::kMicrosecond);
// BENCHMARK(APPEND_buffet)->RangeMultiplier(MUL)->Range(BEG,END)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();