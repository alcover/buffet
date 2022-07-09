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
char alpha[alphalen+1];
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
    "%d: %d bytes %s:'%.*s' != '%s'\n", __LINE__, (int)(n), #val, (int)(n), (val), (exp)); \
exit(EXIT_FAILURE);}}


#define check_props(buf, off, len) do{ \
    /* len */ \
    assert_int (bft_len(buf), len); \
    /* data */ \
    const char *exp = take(off,len); \
    assert_stn (bft_data(buf), exp, len); \
    /* cstr */ \
    bool mustfree; \
    const char *cstr = bft_cstr(buf, &mustfree); \
    assert_str (cstr, exp); \
    if (mustfree) free((char*)cstr); \
    /* export */ \
    char *export = bft_export(buf); \
    assert_str (export, exp); \
    free(export); \
}while(0)

#define check_zero(buf) { \
    assert_int (bft_len(buf), 0); \
    assert_str (bft_data(buf), ""); \
    bool mustfree; \
    const char *cstr = bft_cstr(buf, &mustfree); \
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
    Buffet buf = bft_new(cap); 
    check_props(&buf, 0, 0);
    bft_free(&buf); 
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
void umemcopy(size_t off, size_t len) {
    Buffet buf = bft_memcopy(alpha+off, len);
    check_props(&buf, off, len);    
    bft_free(&buf);
}

void memcopy() 
{
    serie(umemcopy, 0);
    serie(umemcopy, 8);
}

//=============================================================================
void umemview (size_t off, size_t len) {
    Buffet buf = bft_memview (alpha+off, len);
    check_props(&buf, off, len);
    bft_free(&buf);
}

void memview() 
{
    serie(umemview, 0);
    serie(umemview, 8);
}

//=============================================================================
#define dup_new(len) { \
    Buffet src = bft_new(len); \
    Buffet buf = bft_dup(&src); \
    check_props(&buf, 0, 0); \
    bft_free(&buf); \
    bft_free(&src); \
}

#define dup_memcopy(len) { \
    Buffet src = bft_memcopy(alpha, len); \
    Buffet buf = bft_dup(&src); \
    check_props(&buf, 0, len); \
    bft_free(&buf); \
    bft_free(&src); \
}

#define dup_memview(len) { \
    Buffet src = bft_memview(alpha, len); \
    Buffet buf = bft_dup(&src); \
    check_props(&buf, 0, len); \
    bft_free(&buf); \
    bft_free(&src); \
}

#define dup_view(srclen, len) { \
    Buffet src = bft_memcopy(alpha, srclen); \
    Buffet ref = bft_view(&src, 0, len); \
    Buffet buf = bft_dup(&ref); \
    check_props(&buf, 0, len); \
    bft_free(&buf); \
    bft_free(&ref); \
    bft_free(&src); \
}

void dup()
{
    dup_new(0);
    dup_new(8);
    dup_new(32);
    dup_memcopy(0);
    dup_memcopy(8);
    dup_memcopy(32);
    dup_memview(0);
    dup_memview(8);
    dup_memview(32);
    dup_view(8, 0);
    dup_view(8, 4);
    dup_view(32, 0);
    dup_view(32, 16);
}


//=============================================================================
void ucopy (size_t off, size_t len) {
    Buffet src = bft_memcopy(alpha, alphalen);
    Buffet buf = bft_copy(&src, off, len);
    check_props(&buf, off, len);
    bft_free(&buf);
    bft_free(&src);
}

void copy() 
{
    serie(ucopy, 0);
    serie(ucopy, 8);
    ucopy (0, alphalen);
}


//===== VIEW ==================================================================

#define viewcheck(src, srclen, off, len) \
    Buffet view = bft_view(&src, off, len); \
    size_t explen = len; \
    if (off>=srclen) explen = 0; \
    else if (off+len>=srclen) explen = srclen-off; \
    assert_int (bft_len(&view), explen); \
    assert_stn (bft_data(&view), alpha+off, explen); \
    bft_free(&view); \
    bft_free(&src);


#define view_new(cap, off, len) { \
    Buffet src = bft_new(cap); \
    Buffet view = bft_view(&src, off, len); \
    assert_int (bft_len(&view), 0); \
    assert_str (bft_data(&view), ""); \
}

#define view_memcopy(srclen, off, len) { \
    Buffet src = bft_memcopy(alpha, srclen); \
    viewcheck(src, srclen, off, len) \
}

#define view_memview(srclen, off, len) { \
    Buffet src = bft_memview (alpha, srclen); \
    viewcheck(src, srclen, off, len) \
}

#define view_view(srclen, off, len) { \
    Buffet buf = bft_memcopy(alpha, alphalen); \
    Buffet src = bft_view (&buf, 0, srclen); \
    viewcheck(src, srclen, off, len) \
    bft_free(&buf); \
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

#define view_alias_after_free(initlen) { \
    Buffet src = bft_memcopy(alpha, initlen); \
    Buffet alias = src; \
    bft_free(&src);     \
    Buffet ref = bft_view (&alias, 0, initlen); \
    check_props(&ref, 0, 0); \
    bft_free(&ref);  \
}

void view() 
{
    view_new (0, 0, 0);
    view_new (0, 8, 0);
    view_new (0, 8, 8);
    view_new (8, 0, 0);
    view_new (8, 0, 4);
    view_new (8, 0, 8);
    view_new (32, 0, 0);
    view_new (32, 0, 16);
    view_new (32, 0, 32);

    VIEWCASES (view_memcopy, 8)
    VIEWCASES (view_memcopy, 32)
    VIEWCASES (view_memview, 8)
    VIEWCASES (view_memview, 32)
    VIEWCASES (view_view, 8)
    VIEWCASES (view_view, 32)
    
    // view_alias_after_free(8); // useless. SSO alias is copy.
    view_alias_after_free(32);
}

//==============================================================================

#define ucat(dst, buf, buflen, len) {\
    size_t rc = bft_cat(dst, buf, alpha+buflen, len); \
    size_t explen = buflen+len; \
    assert_int(rc, explen); \
    check_props(dst, 0, explen);\
    bft_free(dst); \
    bft_free(buf); \
}

#define cat_new(cap, len) {\
    Buffet dst; \
    Buffet buf = bft_new(cap);\
    ucat (&dst, &buf, 0, len) \
}

#define cat_init(op, buflen, len) {\
    Buffet dst; \
    Buffet buf = bft_##op(alpha, buflen); \
    ucat (&dst, &buf, buflen, len); \
}

#define cat_view(buflen, len) { \
    Buffet src = bft_memcopy(alpha, buflen); \
    Buffet ref = bft_view (&src, 0, buflen); \
    Buffet dst; \
    ucat (&dst, &ref, buflen, len) \
    bft_free(&src); \
}

void cat()
{
    cat_new (0, 0);
    cat_new (0, 4);
    cat_new (0, 32);
    cat_new (4, 0);
    cat_new (4, 4);
    cat_new (4, 32);
    cat_new (32, 0);    
    cat_new (32, 4);
    cat_new (32, 32);

    cat_init (memcopy, 0, 0);
    cat_init (memcopy, 0, 4);
    cat_init (memcopy, 0, 32);
    cat_init (memcopy, 4, 0);
    cat_init (memcopy, 4, 4);
    cat_init (memcopy, 4, BUFFET_SSOMAX-4);
    cat_init (memcopy, 4, BUFFET_SSOMAX-3);
    cat_init (memcopy, 4, 32);
    cat_init (memcopy, 32, 0);
    cat_init (memcopy, 32, 32);

    cat_init (memview, 0, 0);
    cat_init (memview, 0, 4);
    cat_init (memview, 0, 32);
    cat_init (memview, 4, 0);
    cat_init (memview, 4, 4);
    cat_init (memview, 4, BUFFET_SSOMAX-4);
    cat_init (memview, 4, BUFFET_SSOMAX-3);
    cat_init (memview, 4, 32);
    cat_init (memview, 32, 0);
    cat_init (memview, 32, 32);
    
    cat_view (0, 0);
    cat_view (0, 4);
    cat_view (0, 32);
    cat_view (4, 0);
    cat_view (4, 4);
    cat_view (4, 4);
    cat_view (4, BUFFET_SSOMAX-4);
    cat_view (4, BUFFET_SSOMAX-3);
    cat_view (4, 32);
    cat_view (32, 0);
    cat_view (32, 32);
}

//==============================================================================

void uapn(Buffet *buf, size_t buflen, size_t len) {
    size_t rc = bft_append (buf, alpha+buflen, len); 
    size_t explen = buflen+len; 
    assert_int(rc, explen); 
    check_props(buf, 0, explen); 
    bft_free(buf); 
}

#define apn_new(cap, len) {\
    Buffet buf = bft_new(cap);\
    uapn (&buf, 0, len); \
}

#define apn_init(op, buflen, len) {\
    Buffet buf = bft_##op (alpha, buflen); \
    uapn (&buf, buflen, len); \
}

#define apn_view(buflen, len) { \
    Buffet src = bft_memcopy(alpha, buflen); \
    Buffet ref = bft_view (&src, 0, buflen); \
    uapn (&ref, buflen, len); \
    bft_free(&src); \
}

#define apn_viewed(buflen, len, exprc) { \
    Buffet buf = bft_memcopy(alpha, buflen); \
    Buffet ref = bft_view (&buf, 0, buflen); \
    /*bft_dbg(&buf);*/ \
    size_t rc = bft_append (&buf, alpha+buflen, len); \
    assert_int(rc, exprc); \
    if (rc) check_props(&buf, 0, (buflen+len)); \
    bft_free(&ref); \
    bft_free(&buf); \
}

void append()
{
    apn_new (0, 0);
    apn_new (0, 4);
    apn_new (0, 32);
    apn_new (4, 0);
    apn_new (4, 4);
    apn_new (4, 32);
    apn_new (32, 0);    
    apn_new (32, 4);
    apn_new (32, 32);

    apn_init (memcopy, 0, 0);
    apn_init (memcopy, 0, 4);
    apn_init (memcopy, 0, 32);
    apn_init (memcopy, 4, 0);
    apn_init (memcopy, 4, 4);
    apn_init (memcopy, 4, BUFFET_SSOMAX-4);
    apn_init (memcopy, 4, BUFFET_SSOMAX-3);
    apn_init (memcopy, 4, 32);
    apn_init (memcopy, 32, 0);
    apn_init (memcopy, 32, 32);
    
    apn_init (memview, 0, 0);
    apn_init (memview, 0, 4);
    apn_init (memview, 0, 32);
    apn_init (memview, 4, 0);
    apn_init (memview, 4, 4);
    apn_init (memview, 4, BUFFET_SSOMAX-4);
    apn_init (memview, 4, BUFFET_SSOMAX-3);
    apn_init (memview, 4, 32);
    apn_init (memview, 32, 0);
    apn_init (memview, 32, 32);
    
    apn_view (0, 0);
    apn_view (0, 4);
    apn_view (0, 32);
    apn_view (4, 0);
    apn_view (4, 4);
    apn_view (4, 4);
    apn_view (4, BUFFET_SSOMAX-4);
    apn_view (4, BUFFET_SSOMAX-3);
    apn_view (4, 32);
    apn_view (32, 0);
    apn_view (32, 32);

    apn_viewed (8, 4, 12);
    apn_viewed (8, 32, 0); // would mutate
    apn_viewed (BUFFET_SSOMAX+1, 32, (BUFFET_SSOMAX+1+32));
}


//==============================================================================

#define usploin(src, sep) { \
    size_t srclen = strlen(src);\
    size_t seplen = strlen(sep);\
    int cnt; \
    Buffet* parts = bft_split (src, srclen, sep, seplen, &cnt); \
    Buffet joined = bft_join (parts, cnt, sep, seplen); \
    assert_str (bft_data(&joined), src); \
    assert_int (bft_len(&joined), srclen); \
    free(parts);\
    bft_free(&joined);\
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

#define check_free(buf) {\
    bft_free(buf); \
    check_zero(buf); \
}

#define free_new(len) { \
    Buffet buf = bft_new (len); \
    check_free(&buf); \
}
#define free_memcopy(len) { \
    Buffet buf = bft_memcopy(alpha, len); \
    check_free(&buf); \
}
#define free_memview(len) { \
    Buffet buf = bft_memview (alpha, len); \
    check_free(&buf); \
}
#define free_view(len) { \
    Buffet own = bft_memcopy(alpha, 32); \
    Buffet ref = bft_view (&own, 0, len); \
    check_free(&ref); \
    check_free(&own); \
}
#define free_copy(len) { \
    Buffet own = bft_memcopy(alpha, 32); \
    Buffet cpy = bft_copy (&own, 0, len); \
    check_free(&cpy); \
    check_free(&own); \
}

#define double_free(len) { \
    Buffet buf = bft_memcopy(alpha, len); \
    check_free(&buf); \
    check_free(&buf); \
}
#define double_free_ref(srclen, len) { \
    Buffet src = bft_memcopy(alpha, srclen); \
    Buffet ref = bft_view (&src, 0, len); \
    check_free(&ref); \
    check_free(&ref); \
    check_free(&src); \
}
#define free_alias(len) { \
    Buffet src = bft_memcopy(alpha, len); \
    Buffet alias = src; \
    check_free(&src); \
    check_free(&alias); \
}
#define free_ref_alias(len) { \
    Buffet own = bft_memcopy(alpha, 32); \
    Buffet ref = bft_view (&own, 0, len); \
    Buffet alias = ref; \
    check_free(&ref); \
    check_free(&alias); \
    check_free(&own); \
}
#define free_viewed(len, intact) { \
    Buffet src = bft_memcopy(alpha, len); \
    Buffet ref = bft_view (&src, 0, len); \
    bft_free(&src); \
    if(intact) check_props(&src, 0, len); \
    else check_zero(&src); \
    check_free(&ref); \
}


void free_()
{
    free_new(0)
    free_new(8)
    free_new(32)
    free_memcopy(0)
    free_memcopy(8)
    free_memcopy(32)
    free_memview(0)
    free_memview(8)
    free_memview(32)
    free_copy(0)
    free_copy(8)
    free_copy(32)
    free_view(0)
    free_view(8)
    free_view(32)

    /**** danger ****/

    double_free(0);
    double_free(8);
    double_free(32);

    double_free_ref(8, 4);
    double_free_ref(32, 8);
    double_free_ref(32, 20);

    free_alias(8);
    free_alias(32);
    
    free_viewed (0, true)
    free_viewed (8, true)
    free_viewed (32, false)

    free_ref_alias (0)
    free_ref_alias (8)
    free_ref_alias (32)
}

//=============================================================================

#define assert_cmp(a, b, exp) \
int cmp = bft_cmp(a, b); \
if (cmp) cmp = cmp/abs(cmp); /*exp = abs(exp);*/ \
assert_int(cmp, exp);


#define ucmp_memop(op, lena, lenb, exp) { \
    Buffet a = bft_##op(alpha, lena); \
    Buffet b = bft_##op(alpha, lenb); \
    assert_cmp(&a, &b, exp); \
    bft_free(&a); \
    bft_free(&b); \
}

#define ucmp_view(len, exp) { \
    Buffet buf = bft_memcopy(alpha, len); \
    Buffet view = bft_view(&buf, 0, len); \
    assert_int (!!bft_cmp(&view, &buf), !!exp); \
    bft_free(&view); \
    bft_free(&buf); \
}

void cmp()
{
    ucmp_memop(memcopy, 0, 0, 0);
    ucmp_memop(memcopy, 1, 1, 0);
    ucmp_memop(memcopy, 8, 8, 0);
    ucmp_memop(memcopy, 32, 32, 0);
    ucmp_memop(memcopy, 1, 2, -1);
    ucmp_memop(memcopy, 8, 9, -1);
    ucmp_memop(memcopy, 32, 33, -1); 

    ucmp_memop(memview, 0, 0, 0);
    ucmp_memop(memview, 1, 1, 0);
    ucmp_memop(memview, 8, 8, 0);
    ucmp_memop(memview, 32, 32, 0);
    ucmp_memop(memview, 1, 2, -1);
    ucmp_memop(memview, 8, 9, -1);
    ucmp_memop(memview, 32, 33, -1);

    ucmp_view(0, 0);
    ucmp_view(1, 0);
    ucmp_view(8, 0);
    ucmp_view(32, 0);

    // todo other combins
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
    assert (!buf.sso.tag); // better cmp to SSO tag.. make it public ?
}

//=============================================================================
#define GREEN "\033[32;1m"
#define RESET "\033[0m"

#define run(name) \
LOG("- " #name); \
name(); \
printf(GREEN "%s OK\n" RESET, #name); fflush(stdout); 

int main()
{
    repeatat(alpha, alphalen, ALPHA64);
    
    LOG("unit tests... ");
    run(zero);
    run(new);
    run(memcopy);
    run(memview);
    run(dup);
    run(copy);
    run(view);
    run(cat);
    run(append);
    run(splitjoin);
    run(free_);
    run(cmp);
    LOG(GREEN "unit tests OK" RESET);

    return 0;
}