#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "buffet.h"
#include "log.h"

const char alpha[] = 
"0000000011111111222222223333333344444444555555556666666677777777"
"8888888899999999aaaaaaaabbbbbbbbccccccccddddddddeeeeeeeeffffffff";
const size_t alphalen = sizeof(alpha);
char tmp[sizeof(alpha)+1];

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

#define check_cstr(buf, off, len) {\
    bool mustfree; \
    const char *cstr = buffet_cstr(buf, &mustfree); \
    const char *expstr = take(off,len); \
    assert_str (cstr, expstr); \
    if (mustfree) free((char*)cstr); \
}

void check_export (Buffet *buf, size_t off, size_t len) {
    char *export = buffet_export(buf);
    const char *expstr = take(off,len);
    assert_str (export, expstr);
    free(export);
}

#define check(buf, off, len) \
    assert_int (buffet_len(&buf), (len)); \
    assert_stn (buffet_data(&buf), alpha+off, len); \
    check_cstr (&buf, off, len); \
    check_export (&buf, off, len)

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
    check(buf, 0, 0);
    buffet_free(&buf); 
}

void new() 
{ 
    unew (0);
    unew (1);
    unew (8);
    around (unew, BUFFET_SSOMAX);
    around (unew, BUFFET_SIZE);
    around (unew, 32);
    around (unew, 64);
    around (unew, 1024);
    around (unew, 4096);
}

//=============================================================================
void umemcopy (size_t off, size_t len)
{
    Buffet buf = buffet_memcopy (alpha+off, len);
    check(buf, off, len);    
    buffet_free(&buf);
}

void memcopy() 
{
    serie(umemcopy, 0);
    serie(umemcopy, 8);
}

//=============================================================================
void umemview (size_t off, size_t len)
{
    Buffet buf = buffet_memview (alpha+off, len);

    check(buf, off, len);
    buffet_free(&buf);
}

void memview() 
{
    serie(umemview, 0);
    serie(umemview, 8);
}

//=============================================================================
void ucopy (size_t off, size_t len) {
    Buffet src = buffet_memcopy (alpha, alphalen);
    Buffet buf = buffet_copy(&src, off, len);

    check(buf, off, len);
    buffet_free(&buf);
    buffet_free(&src);
}

void copy() {
    serie(ucopy, 0);
    serie(ucopy, 8);
    ucopy (0, alphalen);
}

//=============================================================================
void uclone (ptrdiff_t off, size_t len) {
    Buffet src = buffet_memcopy (alpha+off, len);
    Buffet buf = buffet_dup(&src);
    check(buf, off, len);
    buffet_free(&buf);
    buffet_free(&src);
}

void clone() {
    serie(uclone, 0);
    serie(uclone, 8);
}

//=============================================================================

#define uview(srclen, off, len) { \
    Buffet src = buffet_memcopy (alpha, srclen); \
    Buffet view = buffet_view (&src, off, len); \
    check(view, off, len); \
    buffet_free(&view); \
    buffet_free(&src); \
}

void uview_ref (size_t srclen, size_t off, size_t len)
{
    Buffet buf = buffet_memcopy (alpha, alphalen);
    Buffet src = buffet_view (&buf, 0, srclen);
    Buffet view = buffet_view (&src, off, len);

    check(view, off, len);
    
    buffet_free(&view);
    buffet_free(&src);
    buffet_free(&buf);
}

void uview_vue (size_t off, size_t len)
{
    Buffet vue = buffet_memview (alpha, alphalen);
    Buffet view = buffet_view (&vue, off, len);

    check(view, off, len);

    buffet_free(&view);
    buffet_free(&vue);
}

void view()
{
    uview (8, 0, 0);
    uview (8, 0, 4);
    uview (8, 0, 8);
    uview (8, 2, 4);
    uview (8, 2, 6);

    uview (40, 0, 0);
    uview (40, 0, 20);
    uview (40, 0, 40);
    uview (40, 20, 10);
    uview (40, 20, 20);

    uview_ref (40, 0, 0);
    uview_ref (40, 0, 20);
    uview_ref (40, 0, 40);
    uview_ref (40, 20, 10);
    uview_ref (40, 20, 20);

    uview_vue (0, 8);
    uview_vue (0, 40);
}

//=============================================================================

void uappend_new (size_t cap, size_t len)
{
    Buffet buf = buffet_new (cap);
    buffet_append (&buf, alpha, len);

    check(buf, 0, len);

    buffet_free(&buf);    
}

#define uappend_memcopy(initlen, len) {\
    Buffet buf = buffet_memcopy (alpha, initlen); \
    buffet_append (&buf, alpha+initlen, len); \
    size_t totlen = initlen+len;\
    check(buf, 0, totlen);\
    buffet_free(&buf);    \
}

void uappend_memview (size_t initlen, size_t len)
{
    size_t totlen = initlen+len;
    Buffet buf = buffet_memview (alpha, initlen);
    buffet_append (&buf, alpha+initlen, len);

    check(buf, 0, totlen);
    
    buffet_free(&buf);    
}

void uappend_view (size_t initlen, size_t len)
{
    Buffet src = buffet_memcopy (alpha, initlen);
    Buffet ref = buffet_view (&src, 0, initlen);
    buffet_append (&ref, alpha+initlen, len);

    check(ref, 0, initlen+len);
    
    buffet_free(&ref); 
    buffet_free(&src);    
}
// justas idea
void uappend_self (size_t len)
{
    Buffet buf = buffet_memcopy (alpha, len);
    size_t finlen = 2*len;
    memcpy(tmp, alpha, len);
    memcpy(tmp+len, alpha, len);
    tmp[finlen] = 0;

    buffet_append (&buf, buffet_data(&buf), buffet_len(&buf));
    assert_int(buffet_len(&buf), finlen);
    assert_str(buffet_data(&buf), tmp);

    buffet_free(&buf);
}

void append()
{
    uappend_new (0, 0);
    uappend_new (0, 8);
    uappend_new (0, 40);
    uappend_new (8, 0);
    uappend_new (8, 5);
    uappend_new (8, 6);
    uappend_new (8, 7);
    uappend_new (8, 8);
    uappend_new (40, 0);    
    uappend_new (40, 8);
    uappend_new (40, 40);

    uappend_memcopy (4, 4);
    uappend_memcopy (8, 5);
    uappend_memcopy (8, 6);
    uappend_memcopy (8, 7);
    uappend_memcopy (8, 8);
    uappend_memcopy (8, 20);
    uappend_memcopy (20, 20);

    uappend_memview (4, 4);
    uappend_memview (8, 5);
    uappend_memview (8, 6);
    uappend_memview (8, 7);
    uappend_memview (8, 8);
    uappend_memview (8, 20);
    uappend_memview (20, 20);

    uappend_view (8, 4);
    uappend_view (8, 20);
    uappend_view (20, 20);

    uappend_self (0);
    uappend_self (4);
    uappend_self (10);
    uappend_self (16);
    // todo crazy self cases : crossing NUL, garbage only, ...
}

//==============================================================================

#define usplitjoin(src, sep) { \
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

#define check_free(buf, exprc) {\
    bool rc = buffet_free(buf); \
    assert_int (rc, exprc); \
    if (exprc) { \
        assert_int (buffet_len(buf), 0); \
        assert_str (buffet_data(buf), ""); \
        bool mustfree; \
        const char *cstr = buffet_cstr(buf,&mustfree); \
        assert_str (cstr, ""); \
        assert(!mustfree); \
    } \
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
void double_free(size_t len) {
    Buffet buf = buffet_memcopy (alpha, len);
    check_free(&buf, true);
    check_free(&buf, true);
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

void view_after_reloc (size_t initlen)
{
    Buffet src = buffet_memcopy (alpha, initlen);
    Buffet ref = buffet_view (&src, 0, initlen);

    buffet_append (&src, alpha, alphalen);
    // buffet_debug(&ref);
    check(ref, 0, initlen);
    
    buffet_free(&ref); 
    // buffet_debug(&src);
    buffet_free(&src);    
}

void append_view_after_reloc (size_t initlen, size_t len)
{
    Buffet src = buffet_memcopy (alpha, initlen);
    Buffet ref = buffet_view (&src, 0, initlen);
    // buffet_debug(&ref);
    
    // trigger reloc
    const char *loc = buffet_data(&src);
    buffet_append (&src, alpha, alphalen);
    if (buffet_data(&src) == loc) {
        LOG("append_view_after_reloc : not relocated, skipping.");
        goto fin;
    }
    
    buffet_append (&ref, alpha+initlen, len);
    check(ref, 0, initlen+len);
    
    fin:
    buffet_free(&ref); 
    buffet_free(&src);    
}

void danger()
{
    double_free(0);
    double_free(8);
    double_free(40);

    double_free_ref(8, 4);
    double_free_ref(40, 8);
    double_free_ref(40, 20);

    free_alias(8, true);
    free_alias(40, false);
    
    free_own_before_view (0, true, true)
    free_own_before_view (8, false, true)
    free_own_before_view (40, false, true)

    free_ref_alias (0, true, true, true)
    free_ref_alias (8, true, true, true)
    free_ref_alias (40, true, true, true)

    view_after_reloc(8);

    append_view_after_reloc(8,4);
    append_view_after_reloc(8,20);
    append_view_after_reloc(20,20);
}

//=============================================================================
int main()
{
    LOG("unit tests... ");

    new();
    memcopy();
    memview();
    copy();
    clone();
    view();
    append();
    splitjoin();
    free_();
    danger();
    
    LOG("unit tests OK");
    return 0;
}