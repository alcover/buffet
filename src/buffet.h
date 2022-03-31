/*
Buffet - All-inclusive Buffer for C
Copyright (C) 2022 - Francois Alcover <francois [on] alcover [dot] fr>
*/

#ifndef BUFFET_H
#define BUFFET_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define BUFFET_SIZE 16
#define BUFFET_TAG 2 // bits
#define BUFFET_STACK_MEM 1024

typedef union Buffet {
        
    struct {
        char*    data;
        uint32_t len;
        uint32_t aux : 32-BUFFET_TAG,
                 tag : BUFFET_TAG;
    } ptr;

    struct {
        char     data[BUFFET_SIZE-1];
        uint8_t  len : 8-BUFFET_TAG,
                 tag : BUFFET_TAG;
    } sso;

    char fill[BUFFET_SIZE];
 
} Buffet;


#ifdef __cplusplus
extern "C" {
#endif

void    buffet_new (Buffet *dst, size_t cap);
void    buffet_memcopy (Buffet *dst, const char *src, size_t len);
void    buffet_memview (Buffet *dst, const char *src, size_t len);
Buffet  buffet_copy (const Buffet *src, ptrdiff_t off, size_t len);
Buffet  buffet_view (const Buffet *src, ptrdiff_t off, size_t len);
size_t  buffet_append (Buffet *dst, const char *src, size_t len);
Buffet* buffet_split (const char* src, size_t srclen,
                      const char* sep, size_t seplen, int *outcnt);
Buffet* buffet_splitstr (const char *src, const char *sep, int *outcnt);
Buffet  buffet_join (Buffet *list, int cnt, const char* sep, size_t seplen);
bool    buffet_free (Buffet *buf);
void    buffet_list_free (Buffet *list, int cnt);

size_t  buffet_cap (const Buffet *buf);
size_t  buffet_len (const Buffet *buf);
const char* buffet_data (const Buffet *buf);
const char* buffet_cstr (const Buffet *buf, bool *mustfree);
char*   buffet_export (const Buffet *buf);

void    buffet_print (const Buffet *buf);
void    buffet_debug (const Buffet *buf);

#ifdef __cplusplus
}
#endif

#endif //guard