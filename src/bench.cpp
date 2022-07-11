#include <benchmark/benchmark.h>
#include "utilcpp.h"

extern "C" {
#include <stdio.h>
#include "buffet.h"
#include "util.h"
}

#define alphalen (1024*1024)
char alpha[alphalen+1] = {0};

#define SPC " "
#define SEP SPC 
const char *sep = SEP;

const char *SPLITME =
"a" SEP "b" SEP "c" SEP "d" SEP "e" SEP "fun" SEP "g" SEP "h" SEP \
"a" SEP "b" SEP "c" SEP "d" SEP "e" SEP "fun" SEP "g" SEP "h" SEP \
"a" SEP "b" SEP "c" SEP "d" SEP "e" SEP "fun" SEP "g" SEP "h" SEP \
"a" SEP "b" SEP "c" SEP "d" SEP "e" SEP "fun" SEP "g" SEP "h" SEP \
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

//=============================================================================
#define GETLEN \
    const size_t len = state.range(0);\
    assert (len < alphalen);

static void
MEMCOPY_c (benchmark::State& state) 
{
    GETLEN

    for (auto _ : state) {
        char *buf = (char*)malloc(len+1);
        benchmark::DoNotOptimize(buf);
        memcpy(buf, alpha, len);
        benchmark::DoNotOptimize(buf[len] = 0);
        // assert (!strncmp(buf, alpha, len));
        free(buf);
    }
}

static void
MEMCOPY_buffet (benchmark::State& state) 
{
    GETLEN

    for (auto _ : state) {
        Buffet buf = bft_memcopy(alpha, len);
        benchmark::DoNotOptimize(buf);
        // assert (!strncmp(bft_data(&buf), alpha, len));
        bft_free(&buf);
    }
}

//=============================================================================
static void
MEMVIEW_cpp (benchmark::State& state) 
{
    GETLEN

    for (auto _ : state) {
        auto buf = string_view(alpha, len);
        benchmark::DoNotOptimize(buf);
        // assert (!strncmp(buf.data(), alpha, len));
    }
}

static void
MEMVIEW_buffet (benchmark::State& state) 
{
    GETLEN

    for (auto _ : state) {
        Buffet buf = bft_memview(alpha, len);
        benchmark::DoNotOptimize(buf);
        // assert (!strncmp(bft_data(&buf), alpha, len));
    }
}
 
//=============================================================================
#define APPEND_INIT \
    const size_t initlen = state.range(0);\
    const size_t appnlen = state.range(1);\
    assert (initlen+appnlen < alphalen);

static void 
APPEND_cpp (benchmark::State& state) 
{
    APPEND_INIT

    for (auto _ : state) {
    auto dst = string(alpha, initlen);
        string_view more = string_view(alpha+initlen, appnlen);
        dst += more;
        benchmark::DoNotOptimize(dst);
        // assert (!strncmp(dst.data(), alpha, initlen+appnlen));
    }   
}

static void 
APPEND_buffet (benchmark::State& state) 
{
    APPEND_INIT

    for (auto _ : state) {
    Buffet dst = bft_memview(alpha, initlen);
        benchmark::DoNotOptimize(dst);
        bft_cat (&dst, &dst, alpha+initlen, appnlen);
        // assert (!strncmp(bft_data(&dst), alpha, initlen+appnlen));
    bft_free(&dst);
    }
}

//=============================================================================
static void 
SPLITJOIN_c (benchmark::State& state) 
{
    for (auto _ : state) {
        int cnt = 0;
        char** parts = split(SPLITME, sep, &cnt);
        const char* ret = join(parts, cnt, sep);

        // assert(!strcmp(ret, SPLITME));
        free((void*)ret);
        for (int i = 0; i < cnt; ++i) free(parts[i]);
        free(parts);
    }
}

static void 
SPLITJOIN_cpp (benchmark::State& state) 
{
    for (auto _ : state) {
        vector<string_view> parts = split_cppview(SPLITME, sep);
        const char* ret = join_cppview(parts, sep);

        // assert(!strcmp(ret, SPLITME));
        free((void*)ret);
    }
}

static void 
SPLITJOIN_buffet (benchmark::State& state) 
{
    for (auto _ : state) {
        int cnt = 0;
        Buffet *parts = bft_splitstr(SPLITME, sep, &cnt);
        Buffet back = bft_join(parts, cnt, sep, strlen(sep));
        const char *ret = bft_data(&back);

        // assert(!strcmp(ret, SPLITME));
        bft_free(&back);
        free(parts);
    }
}


//=====================================================================
#define MEMCOPY(one, two) \
BENCHMARK(one)->Arg(8); \
BENCHMARK(two)->Arg(8); \
BENCHMARK(one)->Arg(32); \
BENCHMARK(two)->Arg(32); \
BENCHMARK(one)->Arg(128); \
BENCHMARK(two)->Arg(128); \
BENCHMARK(one)->Arg(512); \
BENCHMARK(two)->Arg(512); \
BENCHMARK(one)->Arg(2048); \
BENCHMARK(two)->Arg(2048); \
BENCHMARK(one)->Arg(8192); \
BENCHMARK(two)->Arg(8192); \

#define MEMVIEW(one, two) \
BENCHMARK(one)->Arg(8); \
BENCHMARK(two)->Arg(8); \

#define APPEND(one, two) \
BENCHMARK(one)->Args({8,4});\
BENCHMARK(two)->Args({8,4});\
BENCHMARK(one)->Args({8,16});\
BENCHMARK(two)->Args({8,16});\
BENCHMARK(one)->Args({24,4});\
BENCHMARK(two)->Args({24,4});\
BENCHMARK(one)->Args({24,32});\
BENCHMARK(two)->Args({24,32});\

MEMVIEW (MEMVIEW_cpp, MEMVIEW_buffet);
MEMCOPY (MEMCOPY_c, MEMCOPY_buffet);
APPEND (APPEND_cpp, APPEND_buffet);
BENCHMARK(SPLITJOIN_c);
BENCHMARK(SPLITJOIN_cpp);
BENCHMARK(SPLITJOIN_buffet);

int main(int argc, char** argv)
{
    repeatat(alpha, alphalen, ALPHA64);

    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();

    // fgetc(stdin);
}