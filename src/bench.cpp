#include <benchmark/benchmark.h>
#include "utilcpp.h"

extern "C" {
#include <stdio.h>
#include "buffet.h"
#include "util.h"
#include "log.h"
}

#define alphalen (1024*1024)
const char *alpha = NULL;

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
MEMCOPY_plainc (benchmark::State& state) 
{
    GETLEN

    for (auto _ : state) {
        char *buf = malloc(len+1);
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
        Buffet buf = buffet_memcopy(alpha, len);
        benchmark::DoNotOptimize(buf);
        // assert (!strncmp(buffet_data(&buf), alpha, len));
        buffet_free(&buf);
    }
}

//=============================================================================
static void
MEMVIEW_cppview (benchmark::State& state) 
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
        Buffet buf = buffet_memview(alpha, len);
        benchmark::DoNotOptimize(buf);
        // assert (!strncmp(buffet_data(&buf), alpha, len));
    }
}
 
//=============================================================================
#define APPEND_INIT \
    const size_t initlen = state.range(0);\
    const size_t appnlen = state.range(1);\
    assert (initlen+appnlen < alphalen);

static void 
APPEND_cppstr (benchmark::State& state) 
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
    Buffet dst = buffet_memview(alpha, initlen);
        benchmark::DoNotOptimize(dst);
        buffet_append (&dst, alpha+initlen, appnlen);
        // assert (!strncmp(buffet_data(&dst), alpha, initlen+appnlen));
    buffet_free(&dst);
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

        // assert(!strcmp(ret, SPLITME));
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

        // assert(!strcmp(ret, SPLITME));
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

        // assert(!strcmp(ret, SPLITME));
        buffet_free(&back);
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

// MEMVIEW (MEMVIEW_cppview, MEMVIEW_buffet);
// MEMCOPY (MEMCOPY_plainc, MEMCOPY_buffet);
APPEND (APPEND_cppstr, APPEND_buffet);
// BENCHMARK(SPLITJOIN_plainc);
// BENCHMARK(SPLITJOIN_cppview);
// BENCHMARK(SPLITJOIN_buffet);

int main(int argc, char** argv)
{
    alpha = repeat(ALPHA64, alphalen);

    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();

    // fgetc(stdin);
    free(alpha);
}