#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "buffet.h"
#include "log.h"

#define assert_int(a, b) { if ((int)(a) != (int)(b)) { \
fprintf(stderr, "%d: %s:%d != %s:%d\n", __LINE__, #a, (int)a, #b, (int)b); \
exit(EXIT_FAILURE);}}

#define assert_str(a, b) { if (strcmp(a,b)) {\
fprintf(stderr, "%d: %s:'%s' != %s:'%s'\n", __LINE__, #a, a, #b, b); \
exit(EXIT_FAILURE);}}

#define assert_strn(a, b, n) { if (strncmp(a,b,n)) {\
fprintf(stderr, \
    "%d: %zu bytes %s:'%s' != %s:'%s'\n", __LINE__, n, #a, a, #b, b); \
exit(EXIT_FAILURE);}}

const char alpha[] = 
"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+=";
const size_t alphalen = 64;
char tmp[100];

//==============================================================================
static void 
check_free (Buffet *b)
{
    bft_free(b);

    assert_int (bft_len(b), 0);
    assert_int (bft_cap(b), BFT_SSO_CAP);
    assert_str (bft_data(b), "");
    bool mustfree;
    assert_str (bft_cstr(b,&mustfree), "");
    assert(!mustfree);
}

static char* 
take(size_t off, size_t len) {
    memcpy(tmp, alpha+off, len);
    tmp[len] = 0;
    return tmp;
}

static void 
check_cstr (Buffet *b, size_t off, size_t len, bool expfree)
{
    bool mustfree;
    const char *cstr = bft_cstr(b, &mustfree);
    const char *expstr = take(off,len);

    assert_str (cstr, expstr);
    assert (mustfree == expfree);

    if (expfree) free((char*)cstr);
}

static void 
check_export (Buffet *b, size_t off, size_t len)
{
    char *export = bft_export(b);
    const char *expstr = take(off,len);
    assert_str (export, expstr);
    free(export);
}

//==============================================================================
static void 
u_new (size_t cap) 
{
    Buffet b; 
    bft_new (&b, cap); 

    assert (bft_cap(&b) >= cap);    
    assert_int (bft_len(&b), 0);
    assert_str (bft_data(&b), "");
    check_cstr (&b, 0, 0, false);
    check_export (&b, 0, 0);
    
    bft_free(&b); 
}

#define u_new_around(n) \
u_new((n)-1); \
u_new((n)); \
u_new((n)+1);

static void new() 
{ 
    u_new (0);
    u_new (1);
    u_new (8);
    u_new_around (BFT_SSO_CAP);
    u_new_around (BFT_SIZE);
    u_new_around (32);
    u_new_around (64);
    u_new_around (1024);
    u_new_around (4096);
    // u_new (UINT32_MAX);
}

//==============================================================================
static void 
u_strcopy (size_t off, size_t len)
{
    Buffet b;
    bft_strcopy (&b, alpha+off, len);

    assert_int (bft_len(&b), len);
    assert_strn (bft_data(&b), alpha+off, len);
    check_cstr (&b, off, len, false);
    check_export (&b, off, len);
    
    bft_free(&b);
}

static void strcopy() 
{
    u_strcopy (0, 0); 
    u_strcopy (0, 1); 
    u_strcopy (0, 8);
    u_strcopy (0, BFT_SSO_CAP-1);
    u_strcopy (0, BFT_SSO_CAP);
    u_strcopy (0, BFT_SSO_CAP+1);
    u_strcopy (0, 20);
    u_strcopy (0, 40);

    u_strcopy (8, 0); 
    u_strcopy (8, 1); 
    u_strcopy (8, 8);
    u_strcopy (8, BFT_SSO_CAP-1);
    u_strcopy (8, BFT_SSO_CAP);
    u_strcopy (8, BFT_SSO_CAP+1);
    u_strcopy (8, 20);
    u_strcopy (8, 40);
}

//==============================================================================
static void u_strview (size_t off, size_t len)
{
    Buffet b;
    bft_strview (&b, alpha+off, len);

    assert_int (bft_len(&b), len);
    assert_strn (bft_data(&b), alpha+off, len);
    check_cstr (&b, off, len, true);
    check_export (&b, off, len);
    
    bft_free(&b);
}

static void strview() 
{
    u_strview (0, 0); 
    u_strview (0, 1); 
    u_strview (0, 8);
    u_strview (0, BFT_SSO_CAP-1);
    u_strview (0, BFT_SSO_CAP);
    u_strview (0, BFT_SSO_CAP+1);
    u_strview (0, 20);
    u_strview (0, 40);

    u_strview (8, 0); 
    u_strview (8, 1); 
    u_strview (8, 8);
    u_strview (8, BFT_SSO_CAP-1);
    u_strview (8, BFT_SSO_CAP);
    u_strview (8, BFT_SSO_CAP+1);
    u_strview (8, 20);
    u_strview (8, 40);
}

//==============================================================================
static void 
u_copy (size_t off, size_t len)
{
    Buffet src;
    bft_strcopy (&src, alpha, alphalen);
    Buffet b = bft_copy(&src, off, len);

    assert_int (bft_len(&b), len);
    assert_strn (bft_data(&b), alpha+off, len);
    check_cstr (&b, off, len, false);
    check_export (&b, off, len);
    
    bft_free(&b);
    bft_free(&src);
}

static void copy()
{
    u_copy (0, 0); 
    u_copy (0, 1); 
    u_copy (0, 8);
    u_copy (0, BFT_SSO_CAP-1);
    u_copy (0, BFT_SSO_CAP);
    u_copy (0, BFT_SSO_CAP+1);
    u_copy (0, 20);
    u_copy (0, 40);

    u_copy (8, 0); 
    u_copy (8, 1); 
    u_copy (8, 8);
    u_copy (8, BFT_SSO_CAP-1);
    u_copy (8, BFT_SSO_CAP);
    u_copy (8, BFT_SSO_CAP+1);
    u_copy (8, 20);
    u_copy (8, 40);
    u_copy (0, alphalen);
}

//==============================================================================

static void 
u_view (size_t srclen, size_t off, size_t len)
{
    Buffet src;
    bft_strcopy (&src, alpha, srclen);
    Buffet view = bft_view (&src, off, len);

    assert_int (bft_len(&view), len);
    assert_strn (bft_data(&view), alpha+off, len);
    check_cstr (&view, off, len, true);
    check_export (&view, off, len);
    
    bft_free(&view);
    bft_free(&src);
}

static void 
u_view_ref (size_t off, size_t len)
{
    Buffet b;
    bft_strcopy (&b, alpha, alphalen);
    Buffet src = bft_view (&b, 0, alphalen);
    Buffet view = bft_view (&src, off, len);

    assert_int (bft_len(&view), len);
    assert_strn (bft_data(&view), alpha+off, len);
    check_cstr (&view, off, len, true);
    check_export (&view, off, len); 
    
    bft_free(&view);
    bft_free(&src);
    bft_free(&b);
}

static void 
u_view_vue (size_t off, size_t len)
{
    Buffet src;
    bft_strview (&src, alpha, alphalen);
    Buffet view = bft_view (&src, off, len);

    assert_int (bft_len(&view), len);
    assert_strn (bft_data(&view), alpha+off, len);
    check_cstr (&view, off, len, true);
    check_export (&view, off, len);

    bft_free(&view);
    bft_free(&src);
}

static void view()
{
    // on SSO
    u_view (8, 0, 0);
    u_view (8, 0, 4);
    u_view (8, 0, 8);
    u_view (8, 2, 6);
    u_view (8, 4, 4);
    // on OWN
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
    // on VUE
    u_view_vue (0, 8);
    u_view_vue (0, 40);
}

//==============================================================================
static void 
u_append_new (size_t cap, size_t len)
{
    Buffet b;
    bft_new (&b, cap);
    bft_append (&b, alpha, len);

    assert_int (bft_len(&b), len);
    assert_strn (bft_data(&b), alpha, len);
    check_cstr (&b, 0, len, false);
    check_export (&b, 0, len);

    bft_free(&b);    
}

static void 
u_append_strcopy (size_t initlen, size_t len)
{
    size_t totlen = initlen+len;
    Buffet b;
    bft_strcopy (&b, alpha, initlen); 
    bft_append (&b, alpha+initlen, len); 

    assert_int (bft_len(&b), totlen);
    assert_strn (bft_data(&b), alpha, totlen);
    check_cstr (&b, 0, totlen, false);
    check_export (&b, 0, totlen);

    bft_free(&b);    
}

static void 
u_append_strview (size_t initlen, size_t len)
{
    size_t totlen = initlen+len;
    Buffet b;
    bft_strview (&b, alpha, initlen);
    bft_append (&b, alpha+initlen, len);

    assert_int (bft_len(&b), totlen);
    assert_strn (bft_data(&b), alpha, totlen);
    check_cstr (&b, 0, totlen, false);
    check_export (&b, 0, totlen);
    
    bft_free(&b);    
}

static void 
u_append_view (size_t initlen, size_t len)
{
    size_t totlen = initlen+len;
    Buffet src;
    bft_strcopy (&src, alpha, initlen);
    Buffet b = bft_view (&src, 0, initlen);
    bft_append (&b, alpha+initlen, len);

    assert_int (bft_len(&b), totlen);
    assert_strn (bft_data(&b), alpha, totlen);
    check_cstr (&b, 0, totlen, false);
    check_export (&b, 0, totlen);
    
    bft_free(&b); 
    bft_free(&b);    
}

static void append()
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

    u_append_strcopy (4, 4);
    u_append_strcopy (8, 5);
    u_append_strcopy (8, 6);
    u_append_strcopy (8, 7);
    u_append_strcopy (8, 8);
    u_append_strcopy (8, 20);
    u_append_strcopy (20, 20);

    u_append_strview (4, 4);
    u_append_strview (8, 5);
    u_append_strview (8, 6);
    u_append_strview (8, 7);
    u_append_strview (8, 8);
    u_append_strview (8, 20);
    u_append_strview (20, 20);

    u_append_view (8, 8);
}

//==============================================================================
static void free_()
{
    {
        Buffet b; 
        bft_new (&b, 10); 
        check_free(&b);
    }
    {
        Buffet b; 
        bft_strcopy (&b, alpha, 10); 
        check_free(&b);
    }
    {
        Buffet b; 
        bft_strcopy (&b, alpha, 40); 
        check_free(&b);
    }
    {
        Buffet b; 
        bft_strview (&b, alpha, 10); 
        check_free(&b);
    }
    {
        Buffet own;
        bft_strcopy (&own, alpha, 40);
        Buffet ref = bft_view (&own, 10, 4); 
        check_free(&ref);
        bft_free(&own);
    }
    {
        Buffet own;
        bft_strcopy (&own, alpha, 40);
        Buffet cpy = bft_copy (&own, 10, 4); 
        check_free(&cpy);
        bft_free(&own);
    }

    {   // double free
        Buffet b; 
        bft_strcopy (&b, alpha, 40); 
        check_free(&b);
        check_free(&b);
    }
}

//==============================================================================

int main()
{
    printf ("unit tests... ");    

    new();
    strcopy();
    strview();
    copy();
    view();
    append();
    free_();

    printf ("OK\n");
    return 0;
}