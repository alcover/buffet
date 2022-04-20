/*
Buffet - All-inclusive Buffer for C
Copyright (C) 2022 - Francois Alcover <francois [on] alcover [dot] fr>
*/

#ifndef BUFFET_H
#define BUFFET_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef BUFFET_STACK_MEM
#define BUFFET_STACK_MEM 1024
#endif

// Do not touch
#define BUFFET_SIZE 16
#define BUFFET_TAG 2 // bits
#define BUFFET_SSOMAX (BUFFET_SIZE-2)

typedef union Buffet {
        
    char fill[BUFFET_SIZE];

    struct {
        char*    data;
        uint32_t len;
        uint32_t aux:32-BUFFET_TAG, tag:BUFFET_TAG;
    } ptr;

    struct {
        char     data[BUFFET_SIZE-1];
        uint8_t  len:8-BUFFET_TAG, tag:BUFFET_TAG;
    } sso;
 
} Buffet;


#ifdef __cplusplus
extern "C" {
#endif

// Create
Buffet  buffet_new (size_t cap);
Buffet  buffet_memcopy (const char *src, size_t len);
Buffet  buffet_memview (const char *src, size_t len);
Buffet  buffet_copy (const Buffet *src, ptrdiff_t off, size_t len);
Buffet  buffet_view (Buffet *src, ptrdiff_t off, size_t len);
Buffet  buffet_clone (const Buffet *src);
Buffet  buffet_join (const Buffet *list, int cnt, 
                     const char* sep, size_t seplen);
Buffet* buffet_split (const char* src, size_t srclen,
                      const char* sep, size_t seplen, int *outcnt);
Buffet* buffet_splitstr (const char *src, const char *sep, int *outcnt);

// Modify
size_t  buffet_append (Buffet *dst, const char *src, size_t len);
bool    buffet_free (Buffet *buf);

// Access
const char* buffet_data (const Buffet *buf);
const char* buffet_cstr (const Buffet *buf, bool *mustfree);
char*   buffet_export (const Buffet *buf);
size_t  buffet_cap (const Buffet *buf);
size_t  buffet_len (const Buffet *buf);

// Util
void    buffet_print (const Buffet *buf);
void    buffet_debug (const Buffet *buf);

#ifdef __cplusplus
}
#endif

#endif //guard