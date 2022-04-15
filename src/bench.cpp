#include <benchmark/benchmark.h>
#include "utilcpp.h"
extern "C" {
	#include "buffet.h"
	#include "log.h"
	#include "util.h"
}

#define SPC " "
#define SEP SPC

const char *TEXT =
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

const char *sep = SEP;

static char* repeat(size_t n) {
    char *ret = malloc(n+1);
    memset(ret,'A',n); ret[n] = 0;
    return ret;
} 

//=============================================================================
static void 
SPLITJOIN_cppView (benchmark::State& state) 
{
    for (auto _ : state) {
        vector<string_view> parts = split_cppview(TEXT, sep);
        const char* ret = join_cppview(parts, sep);

        assert(!strcmp(ret, TEXT));
        free(ret);
    }
}

static void 
SPLITJOIN_plainC (benchmark::State& state) 
{
    for (auto _ : state) {
        int cnt = 0;
        char** parts = split(TEXT, sep, &cnt);
        const char* ret = join(parts, cnt, sep);

        assert(!strcmp(ret, TEXT));
        free(ret);
        for (int i = 0; i < cnt; ++i) free(parts[i]);
        free(parts);
    }
}

static void 
SPLITJOIN_buffet (benchmark::State& state) 
{
    for (auto _ : state) {
        int cnt = 0;
        Buffet *parts = buffet_splitstr(TEXT, sep, &cnt);
        Buffet back = buffet_join(parts, cnt, sep, strlen(sep));
        bool mustfree;
        const char *ret = buffet_cstr(&back, &mustfree);

        assert(!strcmp(ret, TEXT));
        if (mustfree) free(ret);
        buffet_free(&back);
        free(parts);
    }
}
 
//=============================================================================
#define APPEND_ITER 10000
#define APPEND_INIT  \
	const size_t srclen = state.range(0); \
	const char *src = repeat(srclen); \
	const int iter = APPEND_ITER; \
	const char *exp = repeat(iter*srclen);

static void 
APPEND_cpp (benchmark::State& state) 
{
	APPEND_INIT
    for (auto _ : state) {
    	string dst;
        for (int i=0; i<iter; ++i) dst += src;
        const char *ret = dst.data();

        assert(!strcmp(ret, exp));
    }
	
	free(exp);
	free(src);
}

static void 
APPEND_buffet (benchmark::State& state) 
{
	APPEND_INIT
    for (auto _ : state) {
    	Buffet dst = buffet_new(0);
        for (int i=0; i<iter; ++i) buffet_append(&dst, src, srclen);
        const char *ret = buffet_data(&dst);

        assert(!strcmp(ret, exp));
        buffet_free(&dst);
    }
	
	free(exp);
	free(src);
}

//=====================================================================

#define BEG 1
#define END 64
#define MUL 8

BENCHMARK(SPLITJOIN_cppView);
BENCHMARK(SPLITJOIN_plainC);
BENCHMARK(SPLITJOIN_buffet);
BENCHMARK(APPEND_cpp)->RangeMultiplier(MUL)->Range(BEG,END)->Unit(benchmark::kMicrosecond);
BENCHMARK(APPEND_buffet)->RangeMultiplier(MUL)->Range(BEG,END)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();