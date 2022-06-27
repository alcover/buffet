#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "buffet.h"
#include "log.h"
#include "util.h"

#ifdef NDEBUG
#undef NDEBUG
#endif

#define alphalen (128)
const char *alpha;
char tmp[alphalen+1];

// put a slice of alpha into tmp
static char* take (size_t off, size_t len) {
    assert(off+len <= alphalen);
    memcpy(tmp, alpha+off, len);
    tmp[len] = 0;
    return tmp;
}

#define assert_int(val, exp) { if ((int)(val) != (int)(exp)) { \
fprintf(stderr, "%d: %s:%d != %d\n", __LINE__, #val, (int)(val), (int)(exp)); \
exit(EXIT_FAILURE);}}

#define assert_str(val, exp) { if (strcmp((val), (exp))) {\
fprintf(stderr, "%d: %s:'%s' != '%s'\n", __LINE__, #val, (val), (exp)); \
exit(EXIT_FAILURE);}}

#define assert_stn(val, exp, n) { if (strncmp((val), (exp), (n))) {\
fprintf(stderr, \
    "%d: %d bytes %s:'%s' != '%s'\n", __LINE__, (int)(n), #val, (val), (exp)); \
exit(EXIT_FAILURE);}}


#define check_props(buf, off, len) { \
    /* len */ \
    assert_int (buffet_len(buf), len); \
    /* data */ \
    const char *exp = take(off,len); \
    assert_stn (buffet_data(buf), exp, len); \
    /* cstr */ \
    bool mustfree; \
    const char *cstr = buffet_cstr(buf, &mustfree); \
    assert_str (cstr, exp); \
    if (mustfree) free((char*)cstr); \
    /* export */ \
    char *export = buffet_export(buf); \
    assert_str (export, exp); \
    free(export); \
}

#define check_zero(buf) { \
    assert_int (buffet_len(buf), 0); \
    assert_str (buffet_data(buf), ""); \
    bool mustfree; \
    const char *cstr = buffet_cstr(buf, &mustfree); \
    assert_str (cstr, ""); \
    assert(!mustfree); \
}

#define around(fun, n) fun((n)-1); fun(n); fun((n)+1);

#define serie(fun, off) \
    fun (off, 0); \
    fun (off, 1); \
    fun (off, 8); \
    fun (off, BUFFET_SSOMAX-1); \
    fun (off, BUFFET_SSOMAX); \
    fun (off, BUFFET_SSOMAX+1); \
    fun (off, 24); \
    fun (off, 32); \
    fun (off, 48); \
    fun (off, 64); 


//=============================================================================
void unew (size_t cap) {
    Buffet buf = buffet_new(cap); 
    check_props(&buf, 0, 0);
    buffet_free(&buf); 
}

void new() 
{ 
    unew (0);
    unew (1);
    unew (8);
    around (unew, BUFFET_SSOMAX);
    around (unew, sizeof(Buffet));
    around (unew, 32);
    around (unew, 64);
    around (unew, 1024);
    around (unew, 4096);
}

//=============================================================================
void umemcopy (size_t off, size_t len) {
    Buffet buf = buffet_memcopy (alpha+off, len);
    check_props(&buf, off, len);    
    buffet_free(&buf);
}

void memcopy() 
{
    serie(umemcopy, 0);
    serie(umemcopy, 8);
}

//=============================================================================
void umemview (size_t off, size_t len) {
    Buffet buf = buffet_memview (alpha+off, len);
    check_props(&buf, off, len);
    buffet_free(&buf);
}

void memview() 
{
    serie(umemview, 0);
    serie(umemview, 8);
}

//=============================================================================
#define dup_new(len) { \
    Buffet src = buffet_new(len); \
    Buffet buf = buffet_dup(&src); \
    check_props(&buf, 0, 0); \
    buffet_free(&buf); \
    buffet_free(&src); \
}

#define dup_memcopy(len) { \
    Buffet src = buffet_memcopy(alpha, len); \
    Buffet buf = buffet_dup(&src); \
    check_props(&buf, 0, len); \
    buffet_free(&buf); \
    buffet_free(&src); \
}

#define dup_memview(len) { \
    Buffet src = buffet_memview(alpha, len); \
    Buffet buf = buffet_dup(&src); \
    check_props(&buf, 0, len); \
    buffet_free(&buf); \
    buffet_free(&src); \
}

#define dup_view(srclen, len) { \
    Buffet src = buffet_memcopy(alpha, srclen); \
    Buffet ref = buffet_view (&src, 0, len); \
    Buffet buf = buffet_dup(&ref); \
    check_props(&buf, 0, len); \
    buffet_free(&buf); \
    buffet_free(&ref); \
    buffet_free(&src); \
}

void dup()
{
    dup_new(0);
    dup_new(8);
    dup_new(40);
    dup_memcopy(0);
    dup_memcopy(8);
    dup_memcopy(40);
    dup_memview(0);
    dup_memview(8);
    dup_memview(40);
    dup_view(8, 0);
    dup_view(8, 4);
    dup_view(40, 0);
    dup_view(40, 20);
}


//=============================================================================
void ucopy (size_t off, size_t len) {
    Buffet src = buffet_memcopy (alpha, alphalen);
    Buffet buf = buffet_copy(&src, off, len);
    check_props(&buf, off, len);
    buffet_free(&buf);
    buffet_free(&src);
}

void copy() 
{
    serie(ucopy, 0);
    serie(ucopy, 8);
    ucopy (0, alphalen);
}


//=============================================================================
#define viewcheck(src, srclen, off, len) \
    Buffet view = buffet_view (&src, off, len); \
    size_t explen = len; \
    if (off>=srclen) explen = 0; \
    else if (off+len>=srclen) explen = srclen-off; \
    assert_int (buffet_len(&view), explen); \
    assert_stn (buffet_data(&view), alpha+off, explen); \
    buffet_free(&view); \
    buffet_free(&src);

#define view_own(srclen, off, len) { \
    Buffet src = buffet_memcopy (alpha, srclen); \
    viewcheck(src, srclen, off, len) \
}

#define view_ref(srclen, off, len) { \
    Buffet buf = buffet_memcopy (alpha, alphalen); \
    Buffet src = buffet_view (&buf, 0, srclen); \
    viewcheck(src, srclen, off, len) \
    buffet_free(&buf); \
}

#define view_vue(srclen, off, len) { \
    Buffet src = buffet_memview (alpha, srclen); \
    viewcheck(src, srclen, off, len) \
}

#define VIEWCASES(fun, srclen)\
fun (srclen, 0, 0); \
fun (srclen, 0, srclen/2);  /* part */ \
fun (srclen, 0, srclen);    /* full */ \
fun (srclen, 0, srclen+1);  /* beyond */ \
fun (srclen, 2, srclen/2);  /* part */ \
fun (srclen, 2, srclen-2);  /* till end */ \
fun (srclen, 2, srclen+1);  /* beyond */ \
fun (srclen, srclen, 0);    /* bad off */ \
fun (srclen, srclen, 1);    /* bad off */ \
fun (srclen, srclen+1, 0);  /* bad off */ \
fun (srclen, srclen+1, 1);  /* bad off */ \



void view_alias_after_free (size_t initlen)
{
    Buffet src = buffet_memcopy (alpha, initlen);
    Buffet alias = src;
    buffet_free(&src);    
    Buffet ref = buffet_view (&alias, 0, initlen);

    check_props(&ref, 0, 0);
    
    buffet_free(&ref); 
}

void view() 
{
    view_own (0, 0, 0);
    VIEWCASES (view_own, 8) // sso
    VIEWCASES (view_own, 60) // own
    VIEWCASES (view_ref, 8)
    VIEWCASES (view_ref, 60)
    VIEWCASES (view_vue, 8)
    VIEWCASES (view_vue, 60)
    
    // danger

    view_alias_after_free(40);
}


//==============================================================================

#define ucat(dst, buf, buflen, len) {\
    size_t rc = buffet_cat (dst, buf, alpha+buflen, len); \
    size_t explen = buflen+len; \
    assert_int(rc, explen); \
    check_props(dst, 0, explen);\
    buffet_free(dst); \
    buffet_free(buf); \
}

// append : cat(buf,buf)
#define uapn(buf, buflen, len) {\
    size_t rc = buffet_cat (buf, buf, alpha+buflen, len); \
    size_t explen = buflen+len; \
    assert_int(rc, explen); \
    check_props(buf, 0, explen);\
    buffet_free(buf); \
}

#define cat_new(cap, len) {\
    Buffet dst; \
    Buffet buf = buffet_new(cap);\
    ucat (&dst, &buf, 0, len) \
}

#define apn_new(cap, len) {\
    Buffet buf = buffet_new(cap);\
    uapn (&buf, 0, len) \
}

#define cat_init(op, buflen, len) {\
    Buffet dst; \
    Buffet buf = buffet_##op (alpha, buflen); \
    ucat (&dst, &buf, buflen, len); \
}

#define apn_init(op, buflen, len) {\
    Buffet buf = buffet_##op (alpha, buflen); \
    uapn (&buf, buflen, len); \
}

#define cat_view(buflen, len) { \
    Buffet src = buffet_memcopy (alpha, buflen); \
    Buffet ref = buffet_view (&src, 0, buflen); \
    Buffet dst; \
    ucat (&dst, &ref, buflen, len) \
    buffet_free(&src); \
}

#define apn_view(buflen, len) { \
    Buffet src = buffet_memcopy (alpha, buflen); \
    Buffet ref = buffet_view (&src, 0, buflen); \
    uapn (&ref, buflen, len) \
    buffet_free(&src); \
}

#define apn_viewed(buflen, len, exprc) { \
    Buffet buf = buffet_memcopy (alpha, buflen); \
    Buffet ref = buffet_view (&buf, 0, buflen); \
    size_t rc = buffet_cat (&buf, &buf, alpha+buflen, len); \
    assert_int(rc, exprc); \
    if (rc) {check_props(&buf, 0, (buflen+len));} \
    buffet_free(&ref); \
    buffet_free(&buf); \
}

void cat()
{
    cat_new (0, 0);
    cat_new (0, 4);
    cat_new (0, 40);
    cat_new (4, 0);
    cat_new (4, 4);
    cat_new (4, 40);
    cat_new (40, 0);    
    cat_new (40, 4);
    cat_new (40, 40);

    apn_new (0, 0);
    apn_new (0, 4);
    apn_new (0, 40);
    apn_new (4, 0);
    apn_new (4, 4);
    apn_new (4, 40);
    apn_new (40, 0);    
    apn_new (40, 4);
    apn_new (40, 40);

    cat_init (memcopy, 0, 0);
    cat_init (memcopy, 0, 4);
    cat_init (memcopy, 0, 40);
    cat_init (memcopy, 4, 0);
    cat_init (memcopy, 4, 4);
    cat_init (memcopy, 4, BUFFET_SSOMAX-4);
    cat_init (memcopy, 4, BUFFET_SSOMAX-3);
    cat_init (memcopy, 4, 40);
    cat_init (memcopy, 40, 0);
    cat_init (memcopy, 40, 40);

    apn_init (memcopy, 0, 0);
    apn_init (memcopy, 0, 4);
    apn_init (memcopy, 0, 40);
    apn_init (memcopy, 4, 0);
    apn_init (memcopy, 4, 4);
    apn_init (memcopy, 4, BUFFET_SSOMAX-4);
    apn_init (memcopy, 4, BUFFET_SSOMAX-3);
    apn_init (memcopy, 4, 40);
    apn_init (memcopy, 40, 0);
    apn_init (memcopy, 40, 40);

    cat_init (memview, 0, 0);
    cat_init (memview, 0, 4);
    cat_init (memview, 0, 40);
    cat_init (memview, 4, 0);
    cat_init (memview, 4, 4);
    cat_init (memview, 4, BUFFET_SSOMAX-4);
    cat_init (memview, 4, BUFFET_SSOMAX-3);
    cat_init (memview, 4, 40);
    cat_init (memview, 40, 0);
    cat_init (memview, 40, 40);
    
    apn_init (memview, 0, 0);
    apn_init (memview, 0, 4);
    apn_init (memview, 0, 40);
    apn_init (memview, 4, 0);
    apn_init (memview, 4, 4);
    apn_init (memview, 4, BUFFET_SSOMAX-4);
    apn_init (memview, 4, BUFFET_SSOMAX-3);
    apn_init (memview, 4, 40);
    apn_init (memview, 40, 0);
    apn_init (memview, 40, 40);
    
    cat_view (0, 0);
    cat_view (0, 4);
    cat_view (0, 40);
    cat_view (4, 0);
    cat_view (4, 4);
    cat_view (4, 4);
    cat_view (4, BUFFET_SSOMAX-4);
    cat_view (4, BUFFET_SSOMAX-3);
    cat_view (4, 40);
    cat_view (40, 0);
    cat_view (40, 40);
    
    apn_view (0, 0);
    apn_view (0, 4);
    apn_view (0, 40);
    apn_view (4, 0);
    apn_view (4, 4);
    apn_view (4, 4);
    apn_view (4, BUFFET_SSOMAX-4);
    apn_view (4, BUFFET_SSOMAX-3);
    apn_view (4, 40);
    apn_view (40, 0);
    apn_view (40, 40);

    apn_viewed (8, 4, 12);
    apn_viewed (8, 40, 0); // would mutate
    apn_viewed (BUFFET_SSOMAX+1, 40, (BUFFET_SSOMAX+1+40));
}

//==============================================================================

#define usploin(src, sep) { \
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

#define sploin(a,b,sep) \
    usploin ("", #sep); \
    usploin (#sep, #sep); \
    usploin (#a, #sep); \
    usploin (#a #sep,  #sep);  \
    usploin (#a #sep #b,  #sep);  \
    usploin (#a #sep #b #sep,  #sep);  \
    usploin (#sep #a,  #sep); \
    usploin (#sep #a #sep,  #sep);  \
    usploin (#sep #a #sep #b,  #sep);  \
    usploin (#sep #a #sep #b #sep,  #sep);  \
    usploin (#a #sep #sep,  #sep);  \
    usploin (#a #sep #sep #b,  #sep);  \
    usploin (#a #sep #sep #b #sep #sep,  #sep);  \
    usploin (#sep #sep #a,  #sep); \
    usploin (#sep #sep #a #sep #sep,  #sep);  \
    usploin (#sep #sep #a #sep #sep #b,  #sep);  \
    usploin (#sep #sep #a #sep #sep #b #sep #sep, #sep); 

void splitjoin() 
{ 
    sploin (a, b, |)
    sploin (a, b, ||)
    sploin (foo, bar, |)
    sploin (foo, bar, ||)
}

//=============================================================================

#define check_free(buf, exprc) {\
    bool rc = buffet_free(buf); \
    assert_int (rc, exprc); \
    if (exprc) check_zero(buf); \
}

#define free_new(len) { \
    Buffet buf = buffet_new (len); \
    check_free(&buf, true); \
}
#define free_memcopy(len) { \
    Buffet buf = buffet_memcopy (alpha, len); \
    check_free(&buf, true); \
}
#define free_memview(len) { \
    Buffet buf = buffet_memview (alpha, len); \
    check_free(&buf, true); \
}
#define free_view(len) { \
    Buffet own = buffet_memcopy (alpha, 40); \
    Buffet ref = buffet_view (&own, 0, len); \
    check_free(&ref, true); \
    check_free(&own, true); \
}
#define free_copy(len) { \
    Buffet own = buffet_memcopy (alpha, 40); \
    Buffet cpy = buffet_copy (&own, 0, len); \
    check_free(&cpy, true); \
    check_free(&own, true); \
}

#define double_free(len) { \
    Buffet buf = buffet_memcopy (alpha, len); \
    check_free(&buf, true); \
    check_free(&buf, true); \
}
#define double_free_ref(srclen, len) { \
    Buffet src = buffet_memcopy (alpha, srclen); \
    Buffet ref = buffet_view (&src, 0, len); \
    check_free(&ref, true); \
    check_free(&ref, true); \
    check_free(&src, true); \
}
#define free_alias(len, exp) { \
    Buffet src = buffet_memcopy (alpha, len); \
    Buffet alias = src; \
    check_free(&src, true); \
    check_free(&alias, exp); \
}
#define free_ref_alias(len, freeref, freealias, freeown) { \
    Buffet own = buffet_memcopy (alpha, 40); \
    Buffet ref = buffet_view (&own, 0, len); \
    Buffet alias = ref; \
    check_free(&ref, freeref); \
    check_free(&alias, freealias); \
    check_free(&own, freeown); \
}
#define free_own_before_view(len, freeown, freeref) { \
    Buffet own = buffet_memcopy (alpha, 40); \
    Buffet ref = buffet_view (&own, 0, len); \
    check_free(&own, freeown); \
    check_free(&ref, freeref); \
}
#define view_after_free(initlen) { \
    Buffet src = buffet_memcopy (alpha, initlen); \
    Buffet ref = buffet_view (&src, 0, initlen); \
    buffet_free(&src); \
    check_props(&ref, 0, initlen); \
    buffet_free(&ref); \
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

    /**** danger ****/

    double_free(0);
    double_free(8);
    double_free(40);

    double_free_ref(8, 4);
    double_free_ref(40, 8);
    double_free_ref(40, 20);

    free_alias(8, true);
    free_alias(40, true);

    view_after_free(8);
    view_after_free(40);
    
    free_own_before_view (0,  true, true)
    free_own_before_view (8,  true, true)
    free_own_before_view (40, true, true)

    free_ref_alias (0,  true, true, true)
    free_ref_alias (8,  true, true, true)
    free_ref_alias (40, true, true, true)

}

//=============================================================================
void zero()
{
    Buffet buf = BUFFET_ZERO;

    assert (!buf.ptr.data);
    assert (!buf.ptr.len);
    assert (!buf.ptr.off);
    assert (!buf.ptr.tag);

    assert (!strcmp(buf.sso.data, ""));
    assert (!buf.sso.len);
    assert (!buf.sso.refcnt);
    assert (!buf.sso.tag);
}

//=============================================================================
#define GREEN "\033[32;1m"
#define RESET "\033[0m"

#define run(name) \
LOG(#name); \
name(); \
printf(GREEN "%s OK\n" RESET, #name); fflush(stdout); 

int main()
{
    alpha = repeat(ALPHA64, alphalen);
    
    LOG("unit tests... ");
    run(zero);
    run(new);
    run(memcopy);
    run(memview);
    run(dup);
    run(copy);
    run(view);
    run(cat);
    run(splitjoin);
    run(free_);
    LOG(GREEN "unit tests OK" RESET);

    return 0;
}