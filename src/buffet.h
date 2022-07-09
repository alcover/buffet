/*
Buffet - All-inclusive Buffer for C
Copyright (C) 2022 - Francois Alcover <francois [on] alcover [dot] fr>
*/

#ifndef BUFFET_H
#define BUFFET_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

// max stack allocation for split()
#ifndef BUFFET_STACK_MEM
#define BUFFET_STACK_MEM 1024
#endif

#define TAGBITS 2

// tag=OWN : share of heap data
// tag=SSV : small string view
// tag=VUE : view of any data
typedef struct {
    char*   data;
    size_t  len;
    size_t  off:8*sizeof(size_t)-TAGBITS, tag:TAGBITS;
} BuffetPtr;

// tag=SSO : small string embedding
typedef struct {
    char    data[sizeof(BuffetPtr)-2];
    uint8_t refcnt;
    uint8_t len:8-TAGBITS, tag:TAGBITS;
} BuffetSSO;

typedef union Buffet {
    BuffetPtr ptr;
    BuffetSSO sso; 
    char fill[sizeof(BuffetPtr)];
} Buffet;

static_assert (sizeof(BuffetPtr) == sizeof(char*) + 2*sizeof(size_t), 
    "BuffetPtr size");
static_assert (sizeof(Buffet) == sizeof(BuffetPtr), 
    "Buffet size");

#undef TAGBITS

#define BUFFET_ZERO ((Buffet){.fill={0}})
#define BUFFET_SSOMAX (sizeof(((BuffetSSO){0}).data)-1)

#ifdef __cplusplus
extern "C" {
#endif

Buffet  bft_new (size_t cap);
Buffet  bft_memcopy (const char *src, size_t len);
Buffet  bft_memview (const char *src, size_t len);
Buffet  bft_dup (const Buffet *src);
Buffet  bft_copy (const Buffet *src, size_t off, size_t len);
Buffet  bft_copyall (const Buffet *src);
Buffet  bft_view (Buffet *src, size_t off, size_t len);
size_t  bft_cat (Buffet *dst, const Buffet *buf, const char *src, size_t len);
size_t  bft_append (Buffet *buf, const char *src, size_t len);
void    bft_free (Buffet *buf);

Buffet  bft_join (const Buffet *list, int cnt, 
                  const char* sep, size_t seplen);
Buffet* bft_split (const char* src, size_t srclen,
                   const char* sep, size_t seplen, int *outcnt);
Buffet* bft_splitstr (const char *src, const char *sep, int *outcnt);

bool    bft_equal (const Buffet *a, const Buffet *b);
size_t  bft_cap (const Buffet *buf);
size_t  bft_len (const Buffet *buf);
const char* bft_data (const Buffet *buf);
const char* bft_cstr (const Buffet *buf, bool *mustfree);
char*       bft_export (const Buffet *buf);

void    bft_print (const Buffet *buf);
void    bft_dbg (const Buffet *buf);

#ifdef __cplusplus
}
#endif

#endif //guard