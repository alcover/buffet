/*
Buffet - All-inclusive Buffer for C
Copyright (C) 2022 - Francois Alcover <francois [on] alcover [dot] fr>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef BUFFET_H
#define BUFFET_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define BFT_SIZE 16
#define BFT_SSO_CAP (BFT_SIZE-2)
#define BFT_TYPE_BITS 2

typedef union Buffet {
        
    struct {
        char*    data;
        uint32_t len;
        uint32_t aux:30, type:2;
    } ptr;

    struct {
        char     data[BFT_SIZE-1];
        uint8_t  len:6, type:2;
    } sso;

    char fill[BFT_SIZE];
 
} Buffet;


void    bft_new (Buffet *dst, size_t cap);
void    bft_strcopy (Buffet *dst, const char *src, size_t len);
void    bft_strview (Buffet *dst, const char *src, size_t len);
Buffet  bft_copy (const Buffet *src, ptrdiff_t off, size_t len);
Buffet  bft_view (const Buffet *src, ptrdiff_t off, size_t len);
size_t  bft_append (Buffet *dst, const char *src, size_t len);
void    bft_free (Buffet *buf);

size_t  bft_cap (const Buffet *buf);
size_t  bft_len (const Buffet *buf);
const char* bft_data (const Buffet *buf);
const char* bft_cstr (const Buffet *buf, bool *mustfree);
char*   bft_export (const Buffet *buf);

void    bft_print (const Buffet *buf);
void    bft_dbg (Buffet *buf);

#endif