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

typedef struct {
    uint64_t  refcnt; 
    char    data[8];
} Store;

#define OVERALLOC 1.6
#define STEP (1ull<<BFT_TYPE_BITS)
#define ZERO ((Buffet){.fill={0}})
#define DATAOFF offsetof(Store,data)
#define TYPE(buf) ((buf)->sso.type)
#define REFOFF(ref) (STEP * (ref)->ptr.aux + (intptr_t)(ref)->ptr.data % STEP);
#define assert_aligned(p) assert(0 == (intptr_t)(p) % STEP);

static const char*
getdata (const Buffet *buf) {
    return TYPE(buf) ? buf->ptr.data : buf->sso.data;
}

static size_t
getlen (const Buffet *buf) {
    return TYPE(buf) ? buf->ptr.len : buf->sso.len;
}

static size_t
getcap (const Buffet *buf, Type type) {
    return 
        type == OWN ? (STEP * buf->ptr.aux)
    :   type == SSO ? BFT_SSO_CAP
    :   0;
}

static Store*
getstore (Buffet *buf, Type type) 
{
    const char *data = getdata(buf);
    if (type==REF) data -= REFOFF(buf);
    return (Store*)(data-DATAOFF);
}


static void
new_own (Buffet *dst, size_t cap, const char *src, size_t len)
{
    cap = cap+1; //?
    if (cap % STEP) cap = (cap/STEP+1)*STEP; // round to next STEP

    Store *store = aligned_alloc(BFT_SIZE, DATAOFF + cap + 1); //1?
    if (!store) {ERR("failed allocation"); return;}
    *store = (Store){
        .refcnt = 1,
        .data = {'\0'}
    };

    char *data = store->data;
    if (src) memcpy(data, src, len);
    data[len] = 0;

    *dst = (Buffet) {
        .ptr.data = data,
        .ptr.len = len,
        .ptr.aux = cap/STEP,
        .ptr.type = OWN
    };
}

static char*
grow_sso (Buffet *buf, size_t newcap)
{
    Buffet dst;
    new_own (&dst, newcap, buf->sso.data, buf->sso.len);
    *buf = dst;

    return buf->ptr.data; //?
}

static char*
grow_own (Buffet *buf, size_t newcap)
{
    Store *store = getstore(buf, OWN);
    
    store = realloc(store, DATAOFF + newcap + 1);
    if (!store) return NULL;

    char *newdata = store->data;
    assert_aligned(newdata);
    buf->ptr.data = newdata;
    buf->ptr.aux = newcap/STEP; // ?

    return newdata;
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

//=============================================================================
// Public
//=============================================================================

void
bft_new (Buffet *dst, size_t cap)
{
    *dst = ZERO;

    if (cap > BFT_SSO_CAP) {
        new_own(dst, cap, NULL, 0);
    }
}


void
bft_strcopy (Buffet *dst, const char* src, size_t len)
{
    *dst = ZERO;
    
    if (len <= BFT_SSO_CAP) {
        memcpy(dst->sso.data, src, len);
        dst->sso.len = len;
    } else {
        new_own(dst, len, src, len); //?
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


static void
view_data (Buffet *dst, const char *ownerdata, ptrdiff_t off, size_t len) 
{
    *dst = (Buffet) {
        .ptr.data = (char*)(ownerdata + off),
        .ptr.len = len,
        .ptr.aux = off/STEP,
        .ptr.type = REF     
    };

    Store *store = (Store*)(ownerdata-DATAOFF);
    ++ store->refcnt;
}


Buffet
bft_view (const Buffet *src, ptrdiff_t off, size_t len)
{
    const char *data = getdata(src); 
    Buffet ret;// = ZERO;
    
    switch(TYPE(src)) {
        
        case SSO:
        case VUE: {
            bft_strview (&ret, data + off, len); 
        }   break;
        
        case OWN: {
            view_data (&ret, data, off, len);
        }   break;
        
        case REF: {
            uint32_t refoff = REFOFF(src);
            const char *ownerdata = data-refoff; 
            view_data (&ret, ownerdata, refoff+off, len);
        }   break;
    }

    return ret;
}


void
bft_free (Buffet *buf)
{
    const Type type = TYPE(buf);

    if (type == SSO || type == VUE) {
        *buf = ZERO;
        return;
    }
        
    Store *store = getstore(buf, type);
    
    if (type == OWN) {

        if (!(store->refcnt-1)) {
            free(store);
            *buf = ZERO;
        } else {
            --store->refcnt;
        }

    } else { // REF

        *buf = ZERO;
        if (!--store->refcnt) {
            free(store);
        }
    }
}


size_t 
bft_append (Buffet *buf, const char *src, size_t srclen) 
{
    const Type type = TYPE(buf);
    const size_t curlen = getlen(buf);
    const size_t curcap = getcap(buf, type);
    const char *curdata = getdata(buf);
    size_t newlen = curlen + srclen;
    size_t newcap = OVERALLOC*newlen;

    if (type==SSO) {

        if (newlen <= curcap) {
            memcpy (buf->sso.data+curlen, src, srclen);
            buf->sso.data[newlen] = 0;
            buf->sso.len = newlen;
        } else {
            char *data = grow_sso(buf, newcap);
            if (!data) {return 0;}
            memcpy (data+curlen, src, srclen);
            data[newlen] = 0;
            buf->ptr.len = newlen;
        }

    } else if (type==OWN) {

        char *data = (newlen <= curcap) ? buf->ptr.data : grow_own(buf, newcap);
        if (!data) {return 0;}
        memcpy (data+curlen, src, srclen);
        data[newlen] = 0;
        buf->ptr.len = newlen;

    } else if (type==REF) {

        Store *store = getstore(buf, type);
        bft_strcopy (buf, curdata, curlen); 
        -- store->refcnt;
        bft_append (buf, src, srclen);

    } else {

        bft_strcopy (buf, curdata, curlen); 
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
            const size_t len = buf->ptr.len;
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
    return getcap(buf, TYPE(buf));
}

size_t
bft_len (const Buffet* buf) {
    return getlen(buf);
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