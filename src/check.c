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

const char alpha[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
const size_t alphalen = strlen(alpha);

//==============================================================================
static void u_new (size_t cap) 
{
    Buffet b; 
    bft_new (&b, cap); 

    assert (bft_cap(&b) >= cap);    
    assert_int (bft_len(&b), 0);
    assert_str (bft_data(&b), "");
    // assert_str (bft_cstr(&b), "");
    
    bft_free(&b); 
}

static void new() 
{ 
    u_new (0);
    u_new (1);
    u_new (8);
    u_new (BFT_SSO_CAP-1);
    u_new (BFT_SSO_CAP);
    u_new (BFT_SSO_CAP+1);
    u_new (BFT_SIZE-1);
    u_new (BFT_SIZE);
    u_new (BFT_SIZE+1);
    u_new (100);
    u_new (UINT32_MAX);
}

//==============================================================================
static void u_strcopy (size_t off, size_t len)
{
    Buffet b;
    bft_strcopy (&b, alpha+off, len);

    assert_int (bft_len(&b), len);
    assert_strn (bft_data(&b), alpha+off, len);
    // assert_str (bft_cstr(&b), expstr);
    
    bft_free(&b);
}

void strcopy() 
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
    // assert_str (bft_cstr(&b), expstr);
    
    bft_free(&b);
}

void strview() 
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
u_slice (const char* src, size_t off, size_t len, const char* expstr)
{
    Buffet b;
    bft_strcopy (&b, src, strlen(src));
    Buffet v = bft_slice(&b, off, len);
    assert_int (bft_len(&v), len);
    assert_str (bft_cstr(&v), expstr);
    bft_free(&v);
    bft_free(&b);
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
    Buffet b;
    bft_strcopy (&b, alpha, srclen);
    Buffet v = bft_view (&b, off, len);
    assert_int (bft_len(&v), len);
    assert_strn (bft_data(&v), alpha+off, len);
    bft_free(&v);
    bft_free(&b);
}

static void 
u_view_ref (size_t off, size_t len)
{
    Buffet b;
    bft_strcopy (&b, alpha, alphalen);
    Buffet bv = bft_view (&b, 0, alphalen);
    Buffet v = bft_view (&bv, off, len);
    assert_int (bft_len(&v), len);
    assert_strn (bft_data(&v), alpha+off, len);
    bft_free(&bv);
    bft_free(&v);
    bft_free(&b);
}

static void 
u_strviewref (size_t off, size_t len)
{
    Buffet b;
    bft_strview (&b, alpha, alphalen);
    Buffet v = bft_view (&b, off, len);
    assert_int (bft_len(&v), len);
    assert_strn (bft_data(&v), alpha+off, len);
    bft_free(&v);
    bft_free(&b);
}

Buffet create(size_t len){
    Buffet a;
    bft_strcopy(&a, alpha, len);
    Buffet b = bft_view(&a, 4, 4);
    return b;
}

void view()
{
    // Buffet b = create(8);
    // bft_dbg(&b);
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
    u_strviewref (0, 8);
    u_strviewref (0, 40);
}

//==============================================================================
void u_append_new (size_t cap, size_t len)
{
    Buffet b;
    bft_new (&b, cap);
    bft_append (&b, alpha, len);
    assert_int (bft_len(&b), len);
    assert_strn (bft_data(&b), alpha, len);
    bft_free(&b);    
}

void u_append_strcopy (size_t initlen, size_t len)
{
    size_t totlen = initlen+len;
    Buffet b;
    bft_strcopy (&b, alpha, initlen); 
    //bft_dbg(&b);
    bft_append (&b, alpha+initlen, len); 
    // bft_dbg(&b);
    assert_int (bft_len(&b), totlen);
    assert_strn (bft_data(&b), alpha, totlen);
    bft_free(&b);    
}

void u_append_strview (size_t initlen, size_t len)
{
    // LOGVI(initlen); LOGVI(len);
    size_t totlen = initlen+len;
    Buffet b;
    bft_strview (&b, alpha, initlen);
    bft_append (&b, alpha+initlen, len);
    assert_int (bft_len(&b), totlen);
    assert_strn (bft_data(&b), alpha, totlen);
    bft_free(&b);    
}

void u_append_view (size_t initlen, size_t len)
{
    size_t totlen = initlen+len;
    Buffet b;
    bft_strcopy (&b, alpha, initlen);
    Buffet v = bft_view (&b, 0, initlen);
    bft_append (&v, alpha+initlen, len);
    assert_int (bft_len(&v), totlen);
    assert_strn (bft_data(&v), alpha, totlen);
    bft_free(&v); 
    bft_free(&b);    
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
void export()
{
    #define EXP_NEW(cap) {\
        Buffet b;\
        bft_new(&b, cap);\
        char* out = bft_export(&b);\
        assert_str(out, "");\
        free(out);\
        bft_free(&b);\
    }
    EXP_NEW(10);
    EXP_NEW(100);

   #define EXP_STRCPY(len) {\
        Buffet b;\
        bft_strcopy(&b, alpha, len);\
        char* out = bft_export(&b);\
        assert (!strncmp(out, alpha, len));\
        free(out);\
        bft_free(&b);\
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
    run (strcopy);
    run (strview);
    run (slice);
    run (view);
    run (append);
    run (export);

    printf ("unit tests OK\n");
    return 0;
}