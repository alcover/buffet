/*
Buffet - All-inclusive Buffer for C
Copyright (C) 2022 - Francois Alcover <francois [on] alcover [dot] fr>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "buffet.h"
#include "log.h"

static_assert (sizeof(Buffet)==BFT_SIZE, "Buffet size");

typedef enum {
    SSO = 0,
    OWN,
    REF,
    VUE
} Type;

#define REFC_T uint32_t
typedef struct {
    REFC_T refcnt; 
    char data[1];
} Store;


#define DATAOFF (offsetof(Store, data))
#define TYPE(buf) ((buf)->sso.type)
#define ASDATA(ptr) ((intptr_t)(ptr) >> BFT_TYPE_BITS)
#define DATA(buf) (char*) ((intptr_t)(buf)->ref.data << BFT_TYPE_BITS)
// #define SRC(view) ((Buffet*)(DATA(view)))
#define ZERO ((Buffet){.sso.data={0}, .sso.len=0, .sso.type=SSO})

// check if pointer is safely aligned to be shifted for 62 bits
#define assert_aligned(p) assert(0 == (intptr_t)(p) % (1ull<<BFT_TYPE_BITS));

const int refcnt_max = 100; //(1ull << 8 * sizeof((Buffet){}.refcnt)) - 2;

//=============================================================================
static REFC_T* 
getrefcnt (Buffet *buf) {
    char *data = buf->ptr.data;
    return (REFC_T*)(data-DATAOFF);
}

static char*
getdata (const Buffet *buf) 
{
    switch (TYPE(buf)) {
        case SSO:
            return (char*)buf->sso.data;
        case OWN:
        case VUE:
            return buf->ptr.data;
        case REF:
            return DATA(buf);
    }
    return NULL;
}

static size_t
getlen (const Buffet *buf) {
    switch (TYPE(buf)) {
        case SSO:
            return buf->sso.len;
        case OWN:
        case VUE:
            return buf->ptr.len;
        case REF:
            return buf->ref.len;
    }
}

// static void
// make_own (Buffet *dst, uint8_t cap, size_t len, const char *data)
// {
//     *dst = ZERO;
//     *dst = (Buffet){
//         .ptr.len = len,
//         .ptr.cap = cap,
//         .ptr.data = (char*)data,
//         .ptr.type = OWN
//     };
// }

// static char*
// alloc_own (Buffet *dst, size_t cap)
// {
//     // aligned_alloc ?
//     Store *store = malloc(offsetof(Store, data) + cap + 1);

//     if (!store) {ERR("failed allocation"); return NULL;}

//     *store = (Store){.refcnt=1, .data={'\0'}};
//     char *data = store->data;
//     make_own (dst, cap, 0, data);

//     return data;
// }

static char*
alloc_own (size_t cap)
{
    // aligned_alloc ?
    Store *store = malloc(DATAOFF + cap + 1);
    if (!store) {ERR("failed allocation"); return NULL;}
    *store = (Store){.refcnt=1, .data={'\0'}};

    return store->data;
}

// TODO cap/2 ! (mod?)
#define set_own(dst,datav,lenv,capv) \
    *dst = (Buffet){ \
        .ptr.data = datav, \
        .ptr.len = lenv, \
        .ptr.cap = capv, \
        .ptr.type = OWN \
    }

static char*
grow_sso (Buffet *buf, size_t newcap)
{
    size_t len = buf->sso.len;
    char *newdata = alloc_own(newcap);
    if (!newdata) {return NULL;}

    memcpy (newdata, buf->sso.data, len + 1);
    
    // *buf = (Buffet){
    //     .ptr.data = newdata,
    //     .ptr.len = len,
    //     .ptr.cap = newcap,
    //     .ptr.type = OWN
    // };
    set_own(buf, newdata, len, newcap);

    return newdata;
}

static char*
grow_own (Buffet *buf, size_t newcap)
{
    Store *store = (Store*)(buf->ptr.data-DATAOFF);
    /*char *newdata*/store = realloc(store, newcap);

    if (!store) return NULL;
    // since there is no aligned_realloc()...
    char *newdata = store->data;
    assert_aligned(newdata);

    buf->ptr.data = newdata;
    buf->ptr.cap = newcap;

    return newdata;
}

//=============================================================================

void
bft_new (Buffet *dst, size_t cap)
{
    *dst = ZERO; //sure init? see below

    if (cap > BFT_SSO_CAP) {
        char* data = alloc_own(cap);
        if (!data) return;
        data[0] = 0;
        set_own(dst, data, 0, cap);
    }
}


void
bft_strcopy (Buffet *dst, const char* src, size_t len)
{
    // TODO check ! 
    // - warrant real zeroes
    // - C spec read union field w/o setting it explicitly 
    *dst = ZERO;
    
    if (len <= BFT_SSO_CAP) {

        memcpy(dst->sso.data, src, len);
        dst->sso.len = len;
    
    } else {
    
        char* data = alloc_own(len);
        if (!data) return;
        memcpy(data, src, len);
        data[len] = 0;
        set_own(dst, data, len, len); //?
    }
}


void
bft_strview (Buffet *dst, const char* src, size_t len)
{
    *dst = (Buffet) {
        .ptr.data = (char*)src,
        .ptr.len = len,
        .ptr.type = VUE
    };
}


Buffet
bft_copy (const Buffet *src, ptrdiff_t off, size_t len)
{
    Buffet ret;
    bft_strcopy (&ret, getdata(src)+off, len);
    return ret;
}


Buffet
bft_view (Buffet *src, ptrdiff_t off, size_t len)
{
    Buffet ret = ZERO;
    
    switch(TYPE(src)) {
        
        case SSO:
        case VUE: {
            const char* data = getdata(src); 
            bft_strview (&ret, data + off, len); 
        }   break;
        
        case OWN: {

            const char* data = getdata(src); 
            REFC_T *refcnt = getrefcnt(src);
            
            // only if makes sense. Not if 64-bit refcnt...
            // if (*refcnt < refcnt_max) ...

            ret = (Buffet) {
                .ref.len = len,
                .ref.off = off,
                .ref.data = ASDATA(src),
                .ref.type = REF     
            };
            ++ *refcnt;
        }   break;
        
        case REF: {
            Buffet* ref = SRC(src);
            ret = bft_view (ref, src->off + off, len);
        }   break;
    }

    return ret;
}


void
bft_free (Buffet *buf)
{
    switch(TYPE(buf)) {
        
        case OWN:
            // if no ref left and owner is marked for release, we are go.
            // else mark owner for release
            if (!(buf->refcnt-1)) {
                free(buf->ptr.data);
                *buf = ZERO;
            } else {
                -- buf->refcnt;
            }
            break;

        case REF: {
            Buffet *owner = SRC(buf);
            *buf = ZERO;
            // tell owner it has one less ref..
            -- owner->refcnt;
            // ..and if it was the last on a free-waiting owner, free the owner
            if (!owner->refcnt) {
                free(DATA(owner));
                *owner = ZERO;
            }
        }   break;

        case SSO:
        case VUE:
            *buf = ZERO;
            break;
    }
}


size_t 
bft_append (Buffet *buf, const char *src, size_t srclen) 
{
    const Type type = TYPE(buf);
    const size_t curlen = getlen(buf);
    size_t newlen = curlen + srclen;

    if (type==SSO) {

        if (newlen <= BFT_SSO_CAP) {
            memcpy (buf->sso.data+curlen, src, srclen);
            buf->sso.data[newlen] = 0;
            buf->sso.len = newlen;
        } else {
            char *data = grow_sso(buf, newlen);
            if (!data) {return 0;}
            memcpy (data+curlen, src, srclen);
            data[newlen] = 0;
            buf->ptr.len = newlen;
        }

    } else if (type==OWN) {

        char *data = (newlen <= buf->cap) ? buf->ptr.data : grow_own(buf, newlen);
        if (!data) {return 0;}
        memcpy (data+curlen, src, srclen);
        data[newlen] = 0;
        buf->len = newlen;

    } else {

        const char *curdata = getdata(buf);
        Buffet *ref = SRC(buf);
        bft_strcopy (buf, curdata, curlen); 
        if (type == REF) --ref->refcnt;
        bft_append (buf, src, srclen);
    }

    return newlen;
}


// todo no allocation if ref/vue whole len or stops at end
const char*
bft_cstr (const Buffet *buf, bool *mustfree) 
{
    const char *data = getdata(buf);
    *mustfree = false;

    switch(TYPE(buf)) {
        
        case SSO:
        case OWN:
            return data;
        
        case VUE:
        case REF: {
            const size_t len = buf->len;
            char* ret = malloc(len+1);

            if (!ret) {
                ERR("failed allocation"); 
                *mustfree = true;
                return calloc(1,1);
            }
            
            memcpy (ret, data, len);
            ret[len] = 0;
            *mustfree = true;
            return ret;
        }
    }

    return NULL;
}


char*
bft_export (const Buffet* buf) 
{
    const size_t len = getlen(buf);
    const char* data = getdata(buf);
    char* ret = malloc(len+1);

    if (!ret) {
        ERR("failed allocation"); 
        return calloc(1,1);
    }
    
    memcpy (ret, data, len);
    ret[len] = 0;

    return ret;
}

const char*
bft_data (const Buffet* buf) {
    return getdata(buf);
}

size_t
bft_cap (const Buffet* buf) {
    Type t = TYPE(buf);
    return 
        t == OWN ? (1ull << buf->ptr.cap)
    :   t == SSO ? BFT_SSO_CAP
    :   0;
}

size_t
bft_len (const Buffet* buf) {
    return getlen(buf);
}

static void
print_type (Buffet *buf, char *out) {
    switch(TYPE(buf)) {
        case SSO: sprintf(out,"SSO"); break;
        case OWN: sprintf(out,"OWN"); break;
        case REF: sprintf(out,"REF"); break;
        case VUE: sprintf(out,"VUE"); break;
    }
}

void
bft_print (const Buffet *buf) {
    printf("%.*s\n", (int)getlen(buf), getdata(buf));
}

void 
bft_dbg (Buffet* buf) 
{
    char type[5];
    print_type(buf, type);
    printf ("type:%s cap:%zu len:%zu data:'%s' ", 
        type, bft_cap(buf), bft_len(buf), bft_data(buf));
    printf ("viewing:");
    bft_print(buf);
}