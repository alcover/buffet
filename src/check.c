#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>

#include "buf.h"
#include "util.c"
#include "teststr.h"

#define assert_int(a, b) { if ((int)(a) != (int)(b)) { \
fprintf(stderr, "%d: %s:%d != %s:%d\n", __LINE__, #a, (int)a, #b, (int)b); \
exit(EXIT_FAILURE);}}

#define assert_str(a, b) { if (strcmp(a,b)) {\
fprintf(stderr, "%d: %s:'%s' != %s:'%s'\n", __LINE__, #a, a, #b, b); \
exit(EXIT_FAILURE);}}

#define assert_strncmp(a, b, n) { if (strncmp(a,b,n)) {\
fprintf(stderr, "%d: %s:'%s' != %s:'%s'\n", __LINE__, #a, a, #b, b); \
exit(EXIT_FAILURE);}}

#define assert_buf(b, cap, len, str) \
    assert_int (buf_cap(b), cap);\
    assert_int (buf_len(b), len);\
    assert_str (buf_cstr(b), str);

const char* foo = "foo";
const size_t foolen = 3;
const char alpha[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
const size_t alphalen = strlen(alpha);

//==============================================================================
static void u_new (size_t cap) 
{
    Buf b; 
    buf_new (&b, cap);     
    assert_int (buf_len(&b), 0);
    assert_str (buf_data(&b), "");
    assert_str (buf_cstr(&b), "");
    buf_free(&b); 
}

static void new() 
{ 
    u_new (0);
    u_new (1);
    u_new (8);
    u_new (14);
    u_new (15);
    u_new (16);
    u_new (100);
    u_new (UINT32_MAX);
}

//==============================================================================
static void u_copy_str (const char* src, size_t srclen, const char* expstr)
{
    Buf b;
    buf_copy_str (&b, src, srclen);
    assert_int (buf_len(&b), srclen);
    assert_str (buf_data(&b), expstr);
    assert_str (buf_cstr(&b), expstr);
    buf_free(&b);
}

void copy_str() 
{
    u_copy_str ("", 0, ""); 
    u_copy_str (foo, 0, "");
    u_copy_str (foo, 1, "f"); 
    u_copy_str (foo, 2, "fo");
    u_copy_str (foo, foolen, foo);
    u_copy_str (w16, 13, w13);
    u_copy_str (w16, 14, w14);
    u_copy_str (w16, 15, w15);
    u_copy_str (w16, 16, w16);
    u_copy_str (w32, 32, w32);
}

//==============================================================================

static void u_view_str (const char* src, size_t srclen, const char* expstr)
{
    Buf b;
    buf_view_str (&b, src, srclen);
    assert_int (buf_len(&b), srclen); //buf_dbg(&b);
    char* out = buf_export(&b);
    assert_str (out, expstr);
    // assert_str (buf_cstr(&b), expstr);
    free(out);
    buf_free(&b);
}

void view_str() 
{
    u_view_str ("", 0, ""); 
    u_view_str (foo, 0, "");
    u_view_str (foo, 1, "f"); 
    u_view_str (foo, 2, "fo");
    u_view_str (foo, foolen, foo);
    u_view_str (w16, 13, w13);
    u_view_str (w16, 14, w14);
    u_view_str (w16, 15, w15);
    u_view_str (w16, 16, w16);
    u_view_str (w32, 32, w32);
}

//==============================================================================
static void 
u_slice (const char* src, size_t off, size_t len, const char* expstr)
{
    Buf b;
    buf_copy_str (&b, src, strlen(src));
    Buf v = buf_slice(&b, off, len);
    assert_int (buf_len(&v), len);
    assert_str (buf_cstr(&v), expstr);
    buf_free(&v);
    buf_free(&b);
}

void slice()
{
    u_slice ("abcd", 0, 0, "");
    u_slice ("abcd", 0, 2, "ab");
    u_slice ("abcd", 2, 2, "cd");
    u_slice ("abcd", 0, 4, "abcd");

    u_slice (alpha, 0, 0, "");
    u_slice (alpha, 0, 2, "ab");
    u_slice (alpha, 2, 2, "cd");
    u_slice (alpha, 0, alphalen, alpha);
}

//==============================================================================

static void 
u_view (size_t srclen, size_t off, size_t len)
{
    Buf b;
    buf_copy_str (&b, alpha, srclen);
    Buf v = buf_view (&b, off, len);
    assert_int (buf_len(&v), len);
    assert_strncmp (buf_data(&v), alpha+off, len);
    buf_free(&v);
    buf_free(&b);
}

static void 
u_view_ref (size_t off, size_t len)
{
    Buf b;
    buf_copy_str (&b, alpha, alphalen);
    Buf bv = buf_view (&b, 0, alphalen);
    Buf v = buf_view (&bv, off, len);
    assert_int (buf_len(&v), len);
    assert_strncmp (buf_data(&v), alpha+off, len);
    buf_free(&bv);
    buf_free(&v);
    buf_free(&b);
}

static void 
u_view_strref (size_t off, size_t len)
{
    Buf b;
    buf_view_str (&b, alpha, alphalen);
    Buf v = buf_view (&b, off, len);
    assert_int (buf_len(&v), len);
    assert_strncmp (buf_data(&v), alpha+off, len);
    buf_free(&v);
    buf_free(&b);
}

void view()
{
    // on SSO
    u_view (8, 0, 0);
    u_view (8, 0, 4);
    u_view (8, 0, 8);
    u_view (8, 2, 6);
    u_view (8, 4, 4);
    // on PTR
    u_view (40, 0, 0);
    u_view (40, 0, 8);
    u_view (40, 0, 38);
    u_view (40, 0, 40);
    u_view (40, 2, 38);
    // on REF
    u_view_ref (0,8);
    u_view_ref (0,40);
    u_view_ref (2,8);
    u_view_ref (2,40);
    // on STRVIEW
    u_view_strref (0, 8);
    u_view_strref (0, 40);
}

//==============================================================================
void u_append_new (size_t cap, size_t len)
{
    Buf b;
    buf_new (&b, cap);
    buf_append (&b, alpha, len);
    assert_int (buf_len(&b), len);
    assert_strncmp (buf_data(&b), alpha, len);
    buf_free(&b);    
}

void u_append_copystr (size_t initlen, size_t len)
{
    size_t totlen = initlen+len;
    Buf b;
    buf_copy_str (&b, alpha, initlen);
    buf_append (&b, alpha+initlen, len);
    assert_int (buf_len(&b), totlen);
    assert_strncmp (buf_data(&b), alpha, totlen);
    buf_free(&b);    
}

void u_append_viewstr (size_t initlen, size_t len)
{
    size_t totlen = initlen+len;
    Buf b;
    buf_view_str (&b, alpha, initlen);
    buf_append (&b, alpha+initlen, len);
    assert_int (buf_len(&b), totlen);
    assert_strncmp (buf_data(&b), alpha, totlen);
    buf_free(&b);    
}

void u_append_view (size_t initlen, size_t len)
{
    size_t totlen = initlen+len;
    Buf b;
    buf_copy_str (&b, alpha, initlen);
    Buf v = buf_view (&b, 0, initlen);
    buf_append (&v, alpha+initlen, len);
    assert_int (buf_len(&v), totlen);
    assert_strncmp (buf_data(&v), alpha, totlen);
    buf_free(&v); 
    buf_free(&b);    
}

void append()
{
    u_append_new (0, 0);
    u_append_new (0, 8);
    u_append_new (0, 40);
    u_append_new (8, 0);
    u_append_new (8, 5);
    u_append_new (8, 6);
    u_append_new (8, 7);
    u_append_new (8, 8);
    u_append_new (40, 0);    
    u_append_new (40, 8);
    u_append_new (40, 40);

    u_append_copystr (4, 4);
    u_append_copystr (8, 5);
    u_append_copystr (8, 6);
    u_append_copystr (8, 7);
    u_append_copystr (8, 8);
    u_append_copystr (8, 20);
    u_append_copystr (20, 20);

    u_append_viewstr (4, 4);
    u_append_viewstr (8, 5);
    u_append_viewstr (8, 6);
    u_append_viewstr (8, 7);
    u_append_viewstr (8, 8);
    u_append_viewstr (8, 20);
    u_append_viewstr (20, 20);

    u_append_view (8, 8);
}

//==============================================================================
void export()
{
    #define EXP_NEW(cap) {\
        Buf b;\
        buf_new(&b, cap);\
        char* out = buf_export(&b);\
        assert_str(out, "");\
        free(out);\
        buf_free(&b);\
    }
    EXP_NEW(10);
    EXP_NEW(100);

   #define EXP_STRCPY(len) {\
        Buf b;\
        buf_copy_str(&b, alpha, len);\
        char* out = buf_export(&b);\
        assert (!strncmp(out, alpha, len));\
        free(out);\
        buf_free(&b);\
    }
    EXP_STRCPY(10);
    EXP_STRCPY(alphalen);
}
//==============================================================================

#define run(name) { \
    printf("%s ", #name); fflush(stdout); \
    name(); \
    puts("OK"); \
}

int main()
{
    run (new);
    run (copy_str);
    run (view_str);
    run (slice);
    run (view);
    run (append);
    run (export);
    // run (dup);
    // run (join);
    // run (split);
    // run (append_fmt);
    // run (resize);
    // run (reset);
    // run (adjust);
    // run (trim);
    // run (equal);
    // run (story);

    printf ("unit tests OK\n");
    return 0;
}