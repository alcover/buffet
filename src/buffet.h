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

#define BUFFET_SIZE 16
#define BUFFET_SSO (BUFFET_SIZE-2)
#define BUFFET_TAG 2

typedef union Buffet {
        
    struct {
        char*    data;
        uint32_t len;
        uint32_t aux:32-BUFFET_TAG, type:BUFFET_TAG;
    } ptr;

    struct {
        char     data[BUFFET_SIZE-1];
        uint8_t  len:8-BUFFET_TAG, type:BUFFET_TAG;
    } sso;

    char fill[BUFFET_SIZE];
 
} Buffet;


void    buffet_new (Buffet *dst, size_t cap);
void    buffet_strcopy (Buffet *dst, const char *src, size_t len);
void    buffet_strview (Buffet *dst, const char *src, size_t len);
Buffet  buffet_copy (const Buffet *src, ptrdiff_t off, size_t len);
Buffet  buffet_view (const Buffet *src, ptrdiff_t off, size_t len);
size_t  buffet_append (Buffet *dst, const char *src, size_t len);
void    buffet_free (Buffet *buf);

size_t  buffet_cap (const Buffet *buf);
size_t  buffet_len (const Buffet *buf);
const char* buffet_data (const Buffet *buf);
const char* buffet_cstr (const Buffet *buf, bool *mustfree);
char*   buffet_export (const Buffet *buf);

void    buffet_print (const Buffet *buf);
void    buffet_dbg (Buffet *buf);

#endif