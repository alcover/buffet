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

#define EXP(v) (1ull << (v))
#define ASDATA(ptr) ((intptr_t)(ptr) >> BFT_TYPE_BITS)
#define DATA(buf) ((char*)((intptr_t)((buf)->data) << BFT_TYPE_BITS))
#define SRC(view) ((Buffet*)(DATA(view)))
#define ZERO ((Buffet){.fill={0}})

// check if pointer is safely aligned to be shifted for 62 bits
#define assert_aligned(p) assert(0 == (intptr_t)(p) % (1ull<<BFT_TYPE_BITS));

const int refcnt_max = (1ull << 8 * sizeof((Buffet){}.refcnt)) - 2;

//==============================================================================

static unsigned int 
nextlog2 (unsigned long long n) {
    return 8 * sizeof(unsigned long long) - __builtin_clzll(n-1);
}

static char*
getdata (const Buffet *buf) 
{
    switch(buf->type) {
        case SSO:
            return (char*)buf->sso;
        case OWN:
            return DATA(buf);
        case VUE:
            return DATA(buf) + buf->off;
        case REF:
            return DATA(SRC(buf)) + buf->off;
    }
    return NULL;
}

static size_t
getcap (const Buffet *buf, Type t) {
    return 
        t == OWN ? (1ull << buf->cap)
    :   t == SSO ? BFT_SSO_CAP
    :   0;
}

// optim (!t)*ssolen + (!!t)*len
static size_t
getlen (const Buffet *buf) {
    return (buf->type == SSO) ? buf->ssolen : buf->len;
}

static void
make_own (Buffet *dst, uint8_t caplog, size_t len, const char *data)
{
    *dst = ZERO;
    *dst = (Buffet){
        .len = len,
        .cap = caplog,
        .refcnt = 1, // init as flag : 1 == idle, 0 == marked for freeing 
        .data = ASDATA(data),
        .type = OWN
    };
}

static char*
alloc (Buffet *dst, size_t cap)
{
    const uint8_t caplog = nextlog2(cap+1);
    size_t mem = EXP(caplog);
    char *data = aligned_alloc(BFT_SIZE, mem);

    if (!data) {ERR("failed alloc"); return NULL;}
    
    make_own (dst, caplog, 0, data);

    return data;
}

static char*
grow_sso (Buffet *buf, size_t cap)
{
    uint8_t caplog = nextlog2(cap+1);
    char *data = malloc(EXP(caplog));
    
    if (!data) {ERR("failed alloc"); return NULL;}

    const size_t len = buf->ssolen;
    memcpy (data, buf->sso, len + 1);
    make_own (buf, caplog, len, data);

    return data;
}

static char*
grow_own (Buffet *buf, size_t cap)
{
    uint8_t caplog = nextlog2(cap+1);
    char *data = realloc(DATA(buf), EXP(caplog));

    if (!data) return NULL;
    // since there is no aligned_realloc()...
    assert_aligned(data);

    buf->data = ASDATA(data);
    buf->cap = caplog;

    return data;
}

//==============================================================================

void
bft_new (Buffet *dst, size_t cap)
{
    *dst = ZERO; //sure init? see below

    if (cap > BFT_SSO_CAP) {
        char* data = alloc(dst, cap);
        if (!data) return;
        data[0] = 0;
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

        memcpy(dst->sso, src, len);
        dst->sso[len] = 0; // redundant if zero-init
        dst->ssolen = len;
    
    } else {
    
        char* data = alloc(dst, len);
        if (!data) return;
        memcpy(data, src, len);
        data[len] = 0;
        dst->len = len;
    }
}


void
bft_strview (Buffet *dst, const char* src, size_t len)
{
    *dst = ZERO;
    // save remainder before src address is shifted.
    dst->off = (intptr_t)src % (1 << BFT_TYPE_BITS);
    dst->data = ASDATA(src);
    dst->len = len;
    dst->type = VUE;
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
    
    switch(src->type) {
        
        case SSO:
        case VUE: {
            const char* data = getdata(src); 
            bft_strview (&ret, data + off, len); 
        }   break;
        
        case OWN: {
            if (src->refcnt < refcnt_max) {
                // if refcnt field is not full, make a REF...
                ret.data = ASDATA(src);
                ret.off = off;
                ret.len = len;
                ret.type = REF;     
                ++ src->refcnt;
            } else {
                // ...else make a copy ?
                // or empty SSO ?
                const char* data = getdata(src); 
                bft_strcopy (&ret, data + off, len);
            }
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
    switch(buf->type) {
        
        case OWN:
            // if no ref left and owner is marked for release, we are go.
            // else mark owner for release
            if (!(buf->refcnt-1)) {
                free(DATA(buf));
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
    const Type type = buf->type;
    const size_t curlen = getlen(buf);
    size_t newlen = curlen + srclen;

    if (type==SSO) {

        if (newlen <= BFT_SSO_CAP) {
            memcpy (buf->sso+curlen, src, srclen);
            buf->sso[newlen] = 0;
            buf->ssolen = newlen;
        } else {
            char *data = grow_sso(buf, newlen);
            if (!data) {return 0;}
            memcpy (data+curlen, src, srclen);
            data[newlen] = 0;
            buf->len = newlen;
        }

    } else if (type==OWN) {

        char *data = (newlen <= buf->cap) ? DATA(buf) : grow_own(buf, newlen);
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


// todo no alloc if ref/vue whole len or stops at end
const char*
bft_cstr (const Buffet *buf, bool *mustfree) 
{
    const char *data = getdata(buf);
    *mustfree = false;

    switch(buf->type) {
        
        case SSO:
        case OWN:
            return data;
        
        case VUE:
        case REF: {
            const size_t len = buf->len;
            char* ret = malloc(len+1);

            if (!ret) {
                ERR("failed alloc"); 
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
        ERR("failed alloc"); 
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
    return getcap(buf, buf->type);
}

size_t
bft_len (const Buffet* buf) {
    return getlen(buf);
}

static void
print_type (Buffet *buf, char *out) {
    switch(buf->type) {
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