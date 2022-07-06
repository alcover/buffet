#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>

#include "buffet.h"
#include "log.h"

#define NTHREADS 8
int thidx[] = {0,1,2,3,4,5,6,7};
const char *srcs[] = {
    "00000000",
    "11111111",
    "22222222",
    "33333333",
    "44444444",
    "55555555",
    "66666666",
    "77777777",
};
const int SRCLEN = sizeof(srcs[0]);
Buffet bufs[NTHREADS] = {0};
Buffet buf;

//============================================================================
void* new(void *args)
{
    int idx = *(int*)args;
    buf = bft_new(idx*10);
    return NULL;
}

void* memcopy(void *args)
{
    int idx = *(int*)args;
    buf = bft_memcopy(srcs[idx], idx);
    return NULL;
}

void* append(void *args)
{
    int idx = *(int*)args;
    for (int i = 0; i < SRCLEN; ++i)
        bft_cat(&buf, &buf, srcs[idx], 1);
    return NULL;
}

void* view(void *args)
{
    int idx = *(int*)args;
    bufs[idx] = bft_view(&buf, 0, SRCLEN);
    return NULL;
}

int main(void) 
{
    // buf = bft_new(20);
    pthread_t threads[NTHREADS];

    #define RUN(task) \
    for (int i = 0; i < NTHREADS; i++) \
        pthread_create (&threads[i], NULL, task, &thidx[i]); \
    for (int i = 0; i < NTHREADS; i++) \
        pthread_join (threads[i], NULL); \
    LOG (#task " done"); \
    bft_dbg(&buf)

    // rem: almost no fail when run these first !?
    // RUN(new);
    // RUN(memcopy);

    buf = bft_new(0);
    RUN(append);
    int len = bft_len(&buf);
    int exp = NTHREADS*SRCLEN;
    if (len!=exp) {
        ERR("append fail : len=%d exp=%d\n", len, exp);
    }
    assert(len==exp);


    // RUN(view);
    // for (int i = 0; i < NTHREADS; i++) {
    //     Buffet *b = &bufs[i];
    //     // bft_dbg(b);
    //     assert(bft_len(b)==SRCLEN);
    //     bft_free(b);
    // }

    bft_free(&buf);
    
    return 0;
}