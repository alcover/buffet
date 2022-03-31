#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "buffet.h"
#include "log.h"

#define SSOMAX (BUFFET_SIZE-2) // max inline length

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

// put a slice of alpha into tmp
static char* take (size_t off, size_t len) {
    memcpy(tmp, alpha+off, len);
    tmp[len] = 0;
    return tmp;
}

//=============================================================================

void check_cstr (Buffet *b, size_t off, size_t len, bool expfree)
{
    bool mustfree;
    const char *cstr = buffet_cstr(b, &mustfree);
    const char *expstr = take(off,len);

    assert_str (cstr, expstr);
    
    if (mustfree!=expfree) {
        LOG("expfree:%d != mustfree:%d off:%zu len:%zu", 
            expfree, mustfree, off, len);
        buffet_debug(b); fflush(stdout);
    }
    assert (mustfree == expfree);

    if (expfree) free((char*)cstr);
}

void check_export (Buffet *b, size_t off, size_t len)
{
    char *export = buffet_export(b);
    const char *expstr = take(off,len);
    assert_str (export, expstr);
    free(export);
}

//=============================================================================
void u_new (size_t cap) 
{
    Buffet b; 
    buffet_new (&b, cap); 

    assert (buffet_cap(&b) >= cap);    
    assert_int (buffet_len(&b), 0);
    assert_str (buffet_data(&b), "");
    check_cstr (&b, 0, 0, false);
    check_export (&b, 0, 0);
    
    buffet_free(&b); 
}

#define u_new_around(n) \
u_new((n)-1); \
u_new((n)); \
u_new((n)+1);

void new() 
{ 
    u_new (0);
    u_new (1);
    u_new (8);
    u_new_around (SSOMAX);
    u_new_around (BUFFET_SIZE);
    u_new_around (32);
    u_new_around (64);
    u_new_around (1024);
    u_new_around (4096);
    // u_new (UINT32_MAX);
}

//=============================================================================
void u_strcopy (size_t off, size_t len)
{
    Buffet b;
    buffet_memcopy (&b, alpha+off, len);

    assert_int (buffet_len(&b), len);
    assert_strn (buffet_data(&b), alpha+off, len);
    check_cstr (&b, off, len, false);
    check_export (&b, off, len);
    
    buffet_free(&b);
}

void strcopy() 
{
    u_strcopy (0, 0); 
    u_strcopy (0, 1); 
    u_strcopy (0, 8);
    u_strcopy (0, SSOMAX-1);
    u_strcopy (0, SSOMAX);
    u_strcopy (0, SSOMAX+1);
    u_strcopy (0, 20);
    u_strcopy (0, 40);

    u_strcopy (8, 0); 
    u_strcopy (8, 1); 
    u_strcopy (8, 8);
    u_strcopy (8, SSOMAX-1);
    u_strcopy (8, SSOMAX);
    u_strcopy (8, SSOMAX+1);
    u_strcopy (8, 20);
    u_strcopy (8, 40);
}

//=============================================================================
void u_strview (size_t off, size_t len)
{
    Buffet b;
    buffet_memview (&b, alpha+off, len);

    assert_int (buffet_len(&b), len);
    assert_strn (buffet_data(&b), alpha+off, len);
    check_cstr (&b, off, len, true);
    check_export (&b, off, len);
    
    buffet_free(&b);
}

void strview() 
{
    u_strview (0, 0); 
    u_strview (0, 1); 
    u_strview (0, 8);
    u_strview (0, SSOMAX-1);
    u_strview (0, SSOMAX);
    u_strview (0, SSOMAX+1);
    u_strview (0, 20);
    u_strview (0, 40);

    u_strview (8, 0); 
    u_strview (8, 1); 
    u_strview (8, 8);
    u_strview (8, SSOMAX-1);
    u_strview (8, SSOMAX);
    u_strview (8, SSOMAX+1);
    u_strview (8, 20);
    u_strview (8, 40);
}

//=============================================================================
void u_copy (size_t off, size_t len)
{
    Buffet src;
    buffet_memcopy (&src, alpha, alphalen);
    Buffet b = buffet_copy(&src, off, len);

    assert_int (buffet_len(&b), len);
    assert_strn (buffet_data(&b), alpha+off, len);
    check_cstr (&b, off, len, false);
    check_export (&b, off, len);
    
    buffet_free(&b);
    buffet_free(&src);
}

void copy()
{
    u_copy (0, 0); 
    u_copy (0, 1); 
    u_copy (0, 8);
    u_copy (0, SSOMAX-1);
    u_copy (0, SSOMAX);
    u_copy (0, SSOMAX+1);
    u_copy (0, 20);
    u_copy (0, 40);

    u_copy (8, 0); 
    u_copy (8, 1); 
    u_copy (8, 8);
    u_copy (8, SSOMAX-1);
    u_copy (8, SSOMAX);
    u_copy (8, SSOMAX+1);
    u_copy (8, 20);
    u_copy (8, 40);
    u_copy (0, alphalen);
}

//=============================================================================

void u_view (size_t srclen, size_t off, size_t len, bool mustfree)
{
    Buffet src;
    buffet_memcopy (&src, alpha, srclen);
    Buffet view = buffet_view (&src, off, len);

    assert_int (buffet_len(&view), len);
    assert_strn (buffet_data(&view), alpha+off, len);
    check_cstr (&view, off, len, mustfree);
    check_export (&view, off, len);
    
    buffet_free(&view);
    buffet_free(&src);
}

void u_view_ref (size_t srclen, size_t off, size_t len, bool mustfree)
{
    Buffet b;
    buffet_memcopy (&b, alpha, alphalen);
    Buffet src = buffet_view (&b, 0, srclen);
    Buffet view = buffet_view (&src, off, len);

    assert_int (buffet_len(&view), len);
    assert_strn (buffet_data(&view), alpha+off, len);
    check_cstr (&view, off, len, mustfree);
    check_export (&view, off, len); 
    
    buffet_free(&view);
    buffet_free(&src);
    buffet_free(&b);
}

void u_view_vue (size_t off, size_t len)
{
    Buffet vue;
    buffet_memview (&vue, alpha, alphalen);
    Buffet view = buffet_view (&vue, off, len);

    assert_int (buffet_len(&view), len);
    assert_strn (buffet_data(&view), alpha+off, len);
    check_cstr (&view, off, len, true);
    check_export (&view, off, len);

    buffet_free(&view);
    buffet_free(&vue);
}

// TODO fill-up refcnt
void view()
{
    // on SSO
    u_view (8, 0, 0, true);
    u_view (8, 0, 4, true);
    u_view (8, 0, 8, false); //whole
    u_view (8, 2, 4, true);
    u_view (8, 2, 6, false); //till end
    // on OWN
    u_view (40, 0, 0, true);
    u_view (40, 0, 20, true);
    u_view (40, 0, 40, false);
    u_view (40, 20, 10, true);
    u_view (40, 20, 20, false);
    // on REF
    u_view_ref (40, 0, 0, true);
    u_view_ref (40, 0, 20, true);
    u_view_ref (40, 0, 40, true);
    u_view_ref (40, 20, 10, true);
    u_view_ref (40, 20, 20, true);
    // on VUE
    u_view_vue (0, 8);
    u_view_vue (0, 40);
}

//=============================================================================
// Confusing and certainly not covering all paths..

void u_append_new (size_t cap, size_t len)
{
    Buffet b;
    buffet_new (&b, cap);
    buffet_append (&b, alpha, len);

    assert_int (buffet_len(&b), len);
    assert_strn (buffet_data(&b), alpha, len);
    check_cstr (&b, 0, len, false);
    check_export (&b, 0, len);

    buffet_free(&b);    
}

void u_append_strcopy (size_t initlen, size_t len)
{
    size_t totlen = initlen+len;
    Buffet b;
    buffet_memcopy (&b, alpha, initlen); 
    buffet_append (&b, alpha+initlen, len); 

    assert_int (buffet_len(&b), totlen);
    assert_strn (buffet_data(&b), alpha, totlen);
    check_cstr (&b, 0, totlen, false);
    check_export (&b, 0, totlen);

    buffet_free(&b);    
}

void u_append_strview (size_t initlen, size_t len)
{
    size_t totlen = initlen+len;
    Buffet b;
    buffet_memview (&b, alpha, initlen);
    buffet_append (&b, alpha+initlen, len);

    assert_int (buffet_len(&b), totlen);
    assert_strn (buffet_data(&b), alpha, totlen);
    check_cstr (&b, 0, totlen, false);
    check_export (&b, 0, totlen);
    
    buffet_free(&b);    
}

void u_append_view (size_t initlen, size_t len)
{
    size_t totlen = initlen+len;
    Buffet src;
    buffet_memcopy (&src, alpha, initlen);
    Buffet ref = buffet_view (&src, 0, initlen);
    buffet_append (&ref, alpha+initlen, len);

    assert_int (buffet_len(&ref), totlen);
    assert_strn (buffet_data(&ref), alpha, totlen);
    check_cstr (&ref, 0, totlen, false);
    check_export (&ref, 0, totlen);
    
    buffet_free(&ref); 
    buffet_free(&src);    
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

    u_append_view (8, 4);
    u_append_view (8, 20);
    u_append_view (20, 20);
}

//==============================================================================

#define usplitjoin(src, sep) \
{ \
    size_t srclen = strlen(src);\
    size_t seplen = strlen(sep);\
    int cnt; \
    Buffet* parts = buffet_split (src, srclen, sep, seplen, &cnt); \
    Buffet joined = buffet_join (parts, cnt, sep, seplen); \
    assert_str (buffet_data(&joined), src); \
    assert_int (buffet_len(&joined), srclen); \
    buffet_list_free(parts, cnt);\
    buffet_free(&joined);\
}

void splitjoin() 
{ 
    usplitjoin ("", "|");
    usplitjoin ("|", "|");
    usplitjoin ("a", "|");
    usplitjoin ("a|", "|"); 
    usplitjoin ("a|b", "|"); 
    usplitjoin ("a|b|", "|"); 
    usplitjoin ("|a", "|");
    usplitjoin ("|a|", "|"); 
    usplitjoin ("|a|b", "|"); 
    usplitjoin ("|a|b|", "|"); 
    usplitjoin ("a||", "|"); 
    usplitjoin ("a||b", "|"); 
    usplitjoin ("a||b||", "|"); 
    usplitjoin ("||a", "|");
    usplitjoin ("||a||", "|"); 
    usplitjoin ("||a||b", "|"); 
    usplitjoin ("||a||b||", "|"); 
    
    usplitjoin ("abc", "|"); 
    usplitjoin ("abc|", "|"); 
    usplitjoin ("abc|def", "|"); 
    usplitjoin ("abc|def|", "|"); 
    usplitjoin ("|abc", "|"); 
    usplitjoin ("|abc|", "|"); 
    usplitjoin ("|abc|def", "|"); 
    usplitjoin ("|abc|def|", "|"); 
    usplitjoin ("abc||", "|"); 
    usplitjoin ("abc||def", "|"); 
    usplitjoin ("abc||def||", "|"); 
    usplitjoin ("||abc", "|"); 
    usplitjoin ("||abc||", "|"); 
    usplitjoin ("||abc||def", "|"); 
    usplitjoin ("||abc||def||", "|"); 
}

//=============================================================================

void check_free (Buffet *b, bool expfree)
{
    bool rc = buffet_free(b);

    assert_int (rc, expfree);
    assert_int (buffet_len(b), 0);
    // assert_int (buffet_cap(b), SSOMAX);
    assert_str (buffet_data(b), "");

    bool mustfree;
    assert_str (buffet_cstr(b,&mustfree), "");
    assert(!mustfree);
}

void free_()
{
    {
        Buffet b; 
        buffet_new (&b, 10); 
        check_free(&b, true);
    }
    {
        Buffet b; 
        buffet_memcopy (&b, alpha, 10); 
        check_free(&b, true);
    }
    {
        Buffet b; 
        buffet_memcopy (&b, alpha, 40); 
        check_free(&b, true);
    }
    {
        Buffet b; 
        buffet_memview (&b, alpha, 10); 
        check_free(&b, true);
    }
    {
        Buffet own;
        buffet_memcopy (&own, alpha, 40);
        Buffet ref = buffet_view (&own, 10, 4); 
        check_free(&ref, true);
        buffet_free(&own);
    }
    {
        Buffet own;
        buffet_memcopy (&own, alpha, 40);
        Buffet cpy = buffet_copy (&own, 10, 4); 
        check_free(&cpy, true);
        buffet_free(&own);
    }
    {   // double free
        Buffet b; 
        buffet_memcopy (&b, alpha, 40); 
        check_free(&b, true);
        check_free(&b, true);
    }
    {   // aliasing
        Buffet b; 
        buffet_memcopy (&b, alpha, 40); 
        Buffet alias = b;
        buffet_free(&b);
        assert(buffet_free(&alias) == false);
    }
}

//=============================================================================

int main()
{
    LOG("unit tests... ");    
    new();
    strcopy();
    strview();
    copy();
    view();
    append();
    splitjoin();
    free_();
    LOG("OK");
    return 0;
}