#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "buffet.h"
#include "log.h"

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

#define assert_int(a, b) { if ((int)(a) != (int)(b)) { \
fprintf(stderr, "%d: %s:%d != %s:%d\n", __LINE__, #a, (int)a, #b, (int)b); \
exit(EXIT_FAILURE);}}

#define assert_str(a, b) { if (strcmp(a,b)) {\
fprintf(stderr, "%d: %s:'%s' != %s:'%s'\n", __LINE__, #a, a, #b, b); \
exit(EXIT_FAILURE);}}

#define assert_strn(a, b, n) { if (strncmp(a,b,n)) {\
fprintf(stderr, \
    "%d: %zu bytes %s:'%s' != %s:'%s'\n", __LINE__, (size_t)n, #a, a, #b, b); \
exit(EXIT_FAILURE);}}

#define check_cstr(b, off, len, expfree) \
    bool mustfree; \
    const char *cstr = buffet_cstr(b, &mustfree); \
    const char *expstr = take(off,len); \
    assert_str (cstr, expstr); \
    assert_int (mustfree, expfree); \
    if (expfree) free((char*)cstr)

void check_export (Buffet *b, size_t off, size_t len) {
    char *export = buffet_export(b);
    const char *expstr = take(off,len);
    assert_str (export, expstr);
    free(export);
}

#define check(b, len, off, expfree) \
    assert_int (buffet_len(&b), (int)(len)); \
    assert_strn (buffet_data(&b), alpha+off, len); \
    check_cstr (&b, off, len, expfree); \
    check_export (&b, off, len)

//=============================================================================
void u_new (size_t cap) 
{
    Buffet b; 
    buffet_new (&b, cap); 
    check(b, 0, 0, false);
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
void u_memcopy (size_t off, size_t len)
{
    Buffet b;
    buffet_memcopy (&b, alpha+off, len);
    check(b, len, off, false);    
    buffet_free(&b);
}

void memcopy() 
{
    u_memcopy (0, 0); 
    u_memcopy (0, 1); 
    u_memcopy (0, 8);
    u_memcopy (0, SSOMAX-1);
    u_memcopy (0, SSOMAX);
    u_memcopy (0, SSOMAX+1);
    u_memcopy (0, 20);
    u_memcopy (0, 40);

    u_memcopy (8, 0); 
    u_memcopy (8, 1); 
    u_memcopy (8, 8);
    u_memcopy (8, SSOMAX-1);
    u_memcopy (8, SSOMAX);
    u_memcopy (8, SSOMAX+1);
    u_memcopy (8, 20);
    u_memcopy (8, 40);
}

//=============================================================================
void u_memview (size_t off, size_t len)
{
    Buffet b;
    buffet_memview (&b, alpha+off, len);
    check(b, len, off, true);
    buffet_free(&b);
}

void memview() 
{
    u_memview (0, 0); 
    u_memview (0, 1); 
    u_memview (0, 8);
    u_memview (0, SSOMAX-1);
    u_memview (0, SSOMAX);
    u_memview (0, SSOMAX+1);
    u_memview (0, 20);
    u_memview (0, 40);

    u_memview (8, 0); 
    u_memview (8, 1); 
    u_memview (8, 8);
    u_memview (8, SSOMAX-1);
    u_memview (8, SSOMAX);
    u_memview (8, SSOMAX+1);
    u_memview (8, 20);
    u_memview (8, 40);
}

//=============================================================================
void u_copy (size_t off, size_t len)
{
    Buffet src;
    buffet_memcopy (&src, alpha, alphalen);
    Buffet b = buffet_copy(&src, off, len);

    check(b, len, off, false);
    
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

void u_view (size_t srclen, size_t off, size_t len, bool expmustfree)
{
    Buffet src;
    buffet_memcopy (&src, alpha, srclen);
    Buffet view = buffet_view (&src, off, len);

    check(view, len, off, expmustfree);
    
    buffet_free(&view);
    buffet_free(&src);
}

void u_view_ref (size_t srclen, size_t off, size_t len, bool expmustfree)
{
    Buffet b;
    buffet_memcopy (&b, alpha, alphalen);
    Buffet src = buffet_view (&b, 0, srclen);
    Buffet view = buffet_view (&src, off, len);

    check(view, len, off, expmustfree);
    
    buffet_free(&view);
    buffet_free(&src);
    buffet_free(&b);
}

void u_view_vue (size_t off, size_t len)
{
    Buffet vue;
    buffet_memview (&vue, alpha, alphalen);
    Buffet view = buffet_view (&vue, off, len);

    check(view, len, off, true);

    buffet_free(&view);
    buffet_free(&vue);
}

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

    check(b, len, 0, false);

    buffet_free(&b);    
}

void u_append_memcopy (size_t initlen, size_t len)
{
    size_t totlen = initlen+len;
    Buffet b;
    buffet_memcopy (&b, alpha, initlen); 
    buffet_append (&b, alpha+initlen, len); 

    check(b, totlen, 0, false);

    buffet_free(&b);    
}

void u_append_memview (size_t initlen, size_t len)
{
    size_t totlen = initlen+len;
    Buffet b;
    buffet_memview (&b, alpha, initlen);
    buffet_append (&b, alpha+initlen, len);

    check(b, totlen, 0, false);
    
    buffet_free(&b);    
}

void u_append_view (size_t initlen, size_t len)
{
    Buffet src;
    buffet_memcopy (&src, alpha, initlen);
    Buffet ref = buffet_view (&src, 0, initlen);
    buffet_append (&ref, alpha+initlen, len);

    check(ref, initlen+len, 0, false);
    
    buffet_free(&ref); 
    buffet_free(&src);    
}

void u_append_view_after_reloc (size_t initlen, size_t len)
{
    Buffet src;
    buffet_memcopy (&src, alpha, initlen);
    const char *loc = buffet_data(&src);
    Buffet ref = buffet_view (&src, 0, initlen);
    buffet_append (&src, alpha, alphalen);
    const char *reloc = buffet_data(&src);
    
    if (reloc == loc) {
        LOG("u_append_view_after_reloc : not relocated, skipping case");
        goto fin;
    }
    
    buffet_append (&ref, alpha+initlen, len);

    check(ref, initlen+len, 0, false);
    
    fin:
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

    u_append_memcopy (4, 4);
    u_append_memcopy (8, 5);
    u_append_memcopy (8, 6);
    u_append_memcopy (8, 7);
    u_append_memcopy (8, 8);
    u_append_memcopy (8, 20);
    u_append_memcopy (20, 20);

    u_append_memview (4, 4);
    u_append_memview (8, 5);
    u_append_memview (8, 6);
    u_append_memview (8, 7);
    u_append_memview (8, 8);
    u_append_memview (8, 20);
    u_append_memview (20, 20);

    u_append_view (8, 4);
    u_append_view (8, 20);
    u_append_view (20, 20);

    u_append_view_after_reloc(8,4);
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
    free(parts);\
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

#define check_free(b, expfree) {\
    bool rc = buffet_free(b); \
    assert_int (rc, expfree); \
    assert_int (buffet_len(b), 0); \
    assert_str (buffet_data(b), ""); \
    bool mustfree; \
    assert_str (buffet_cstr(b,&mustfree), ""); \
    assert(!mustfree); \
}
#define free_new(len) { \
    Buffet b; \
    buffet_new (&b, len); \
    check_free(&b, true); \
}
#define free_memcopy(len) { \
    Buffet b; \
    buffet_memcopy (&b, alpha, len); \
    check_free(&b, true); \
}
#define free_memview(len) { \
    Buffet b; \
    buffet_memview (&b, alpha, len); \
    check_free(&b, true); \
}
#define free_view(len) { \
    Buffet own; \
    buffet_memcopy (&own, alpha, 40); \
    Buffet ref = buffet_view (&own, 0, len); \
    check_free(&ref, true); \
    check_free(&own, true); \
}
#define free_copy(len) { \
    Buffet own; \
    buffet_memcopy (&own, alpha, 40); \
    Buffet cpy = buffet_copy (&own, 0, len); \
    check_free(&cpy, true); \
    check_free(&own, true); \
}

void free_()
{
    free_new(0)
    free_new(8)
    free_new(40)
    free_memcopy(0)
    free_memcopy(8)
    free_memcopy(40)
    free_memview(0)
    free_memview(8)
    free_memview(40)
    free_copy(0)
    free_copy(8)
    free_copy(40)
    free_view(0)
    free_view(8)
    free_view(40)
}

//=============================================================================
#define free_own_before_view(len) { \
    Buffet own; \
    buffet_memcopy (&own, alpha, 40); \
    Buffet ref = buffet_view (&own, 0, len); \
    check_free(&own, true); \
    check_free(&ref, true); \
}
#define double_free_own() { \
    Buffet own; \
    buffet_memcopy (&own, alpha, 40); \
    check_free(&own, true); \
    check_free(&own, true); \
}
#define double_free_ref(len) { \
    Buffet own; \
    buffet_memcopy (&own, alpha, 40); \
    Buffet ref = buffet_view (&own, 0, len); \
    check_free(&ref, true); \
    check_free(&ref, true); \
    check_free(&own, true); \
}
#define free_own_alias() { \
    Buffet own; \
    buffet_memcopy (&own, alpha, 40); \
    Buffet alias = own; \
    check_free(&own, true); \
    check_free(&alias, false); \
}
#define free_ref_alias(len) { \
    Buffet own; \
    buffet_memcopy (&own, alpha, 40); \
    Buffet ref = buffet_view (&own, 0, len); \
    Buffet alias = ref; \
    check_free(&ref, true); \
    check_free(&alias, true); \
    check_free(&own, false); \
    buffet_append(&own, alpha, 10); \
}
// check_free(&alias, false); 
void faults()
{
    free_own_before_view(0)
    free_own_before_view(8)
    free_own_before_view(40)
    double_free_own()
    double_free_ref(8)
    double_free_ref(20)
    free_own_alias()
    free_ref_alias(0)
    free_ref_alias(8)
    free_ref_alias(40)    
}
//=============================================================================

int main()
{
    LOG("unit tests... ");

    new();
    memcopy();
    memview();
    copy();
    view();
    append();
    splitjoin();
    free_();
    faults();
    
    LOG("unit tests OK");
    return 0;
}