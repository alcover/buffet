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

static_assert (sizeof(Buffet)==BUFFET_SIZE, "Buffet size");

typedef enum {
    SSO = 0,
    OWN,
    REF,
    VUE
} Tag;

typedef struct {
    uint32_t refcnt;
    uint32_t canary;
    char     data[1];
} Store;

#define SSOMAX (BUFFET_SIZE-2)
#define BUFFET_LIST_LOCAL_MAX (BUFFET_STACK_MEM/sizeof(Buffet))
#define CANARY 0xAAAAAAAA
#define OVERALLOC 1.6
#define ZERO ((Buffet){.fill={0}})
#define SENTINEL ((Buffet){.data={0}, .len=0, .aux=0, .tag=OWN})
#define TAG(buf) ((buf)->sso.tag)
#define STEP (1ull<<BUFFET_TAG)
#define DATAOFF offsetof(Store,data)
#define REFOFF(ref) (STEP * (ref)->ptr.aux + (intptr_t)(ref)->ptr.data % STEP)
#define assert_aligned(ptr) assert(0 == (intptr_t)(ptr) % STEP);

static const char*
getdata (const Buffet *buf) {
    return TAG(buf) ? buf->ptr.data : buf->sso.data;
}

static size_t
getlen (const Buffet *buf) {
    return TAG(buf) ? buf->ptr.len : buf->sso.len;
}

static size_t
getcap (const Buffet *buf, Tag tag) {
    return 
        tag == OWN ? (STEP * buf->ptr.aux)
    :   tag == SSO ? SSOMAX
    :   0;
}

static void 
dbg (const Buffet* buf) 
{
    char tag[10];
    switch(TAG(buf)) {
        case SSO: sprintf(tag,"SSO"); break;
        case OWN: sprintf(tag,"OWN"); break;
        case REF: sprintf(tag,"REF"); break;
        case VUE: sprintf(tag,"VUE"); break;
    }
    bool mustfree;
    const char *cstr = buffet_cstr(buf, &mustfree);

    printf ("tag:%s cap:%zu len:%zu cstr:'%s'\n", 
    tag, buffet_cap(buf), buffet_len(buf), cstr);

    if (mustfree) free((char*)cstr);
}

static void 
setlen (Buffet *buf, size_t len) {
    if (TAG(buf)) buf->ptr.len = len; else buf->sso.len = len;
}

static Store*
getstore (Buffet *buf) {
    ptrdiff_t off = (TAG(buf)==REF) ? REFOFF(buf) : 0;
    return (Store*)(buf->ptr.data - off - DATAOFF);
}


static Store*
new_store (size_t cap)
{
    size_t mem = DATAOFF+cap+1;
    Store *store = aligned_alloc(sizeof(Store), mem);

    if (!store) {
        ERR("failed Store allocation"); 
        return NULL;
    }
    *store = (Store){
        .refcnt = 0,
        .canary = CANARY,
        .data = {0}
    };

    store->data[cap] = 0;

    return store;
}


static char*
new_own (Buffet *dst, size_t cap, const char *src, size_t len)
{
    // round to next STEP
    // optim if pow2 : (cap+STEP-1)&-STEP ?
    size_t fincap = cap+STEP-1 - (cap+STEP-1)%STEP;

    Store *store = new_store(fincap);
    if (!store) return NULL;

    char *data = store->data;
    if (src) memcpy(data, src, len);
    data[len] = 0;

    *dst = (Buffet) {
        .ptr.data = data,
        .ptr.len = len,
        .ptr.aux = fincap/STEP,
        .ptr.tag = OWN
    };

    store->refcnt = 1;

    return data;
}


static void
new_ref (Buffet *dst, const char *owned, ptrdiff_t off, size_t len) 
{
    *dst = (Buffet) {
        .ptr.data = (char*)(owned + off),
        .ptr.len = len,
        .ptr.aux = off/STEP,
        .ptr.tag = REF     
    };

    Store *store = (Store*)(owned-DATAOFF);
    ++ store->refcnt;
}


// separate for future buffet_grow()
static char*
grow_sso (Buffet *buf, size_t newcap)
{
    return new_own (buf, newcap, buf->sso.data, buf->sso.len);
}

static char*
grow_own (Buffet *buf, size_t newcap)
{
    Store *store = getstore(buf);
    
    store = realloc(store, DATAOFF + newcap + 1);
    if (!store) return NULL;

    char *newdata = store->data;
    assert_aligned(newdata);

    buf->ptr.data = newdata;
    buf->ptr.aux = newcap/STEP; // ?

    return newdata;
}

//=============================================================================
// Public
//=============================================================================

void
buffet_new (Buffet *dst, size_t cap)
{
    if (cap <= SSOMAX) {
        *dst = ZERO;
    } else {
        new_own(dst, cap, NULL, 0);
    }
}


void
buffet_memcopy (Buffet *dst, const char* src, size_t len)
{
    if (len <= SSOMAX) {
        *dst = ZERO;
        memcpy(dst->sso.data, src, len);
        dst->sso.len = len;
    } else {
        new_own(dst, len, src, len);
    }
}


void
buffet_memview (Buffet *dst, const char* src, size_t len)
{
    *dst = (Buffet) {
        .ptr.data = (char*)src,
        .ptr.len = len,
        .ptr.tag = VUE
    };
}


Buffet
buffet_copy (const Buffet *src, ptrdiff_t off, size_t len)
{
    if (off+len > getlen(src)) {
        return ZERO;
    }

    Buffet ret;
    buffet_memcopy (&ret, getdata(src)+off, len);
    return ret;
}


Buffet
buffet_view (const Buffet *src, ptrdiff_t off, size_t len)
{
    if (off+len > getlen(src)) {
        return ZERO;
    }
    
    const char *data = getdata(src); 
    Buffet ret;
    
    switch(TAG(src)) {
        
        case SSO:
        case VUE: {
            buffet_memview (&ret, data + off, len); 
        }   break;
        
        case OWN: {
            new_ref (&ret, data, off, len);
        }   break;
        
        case REF: {
            uint32_t refoff = REFOFF(src);
            const char *ownerdata = data-refoff; 
            new_ref (&ret, ownerdata, refoff+off, len);
        }   break;
    }

    return ret;
}


// The canary could prevent some memory faults (if not optimized-out).
// e.g aliasing :
//   Buf owner = own(data)
//   Buf alias = owner // DANGER
//   buf_free(owner)   // checks canary : OK. Free store, erase canary.
//   buf_free(alias)   // (normally fatal) checks canary : BAD. Do nothing. 
bool
buffet_free (Buffet *buf)
{
    Tag tag = TAG(buf);
    bool freed = false;

    if (tag==OWN||tag==REF) {
        
        Store *store = getstore(buf);

        // protection #1 : against aliasing
        if (!store->refcnt) {
            ERR("bad refcnt");
            goto fin;
        }

        // protection #2 : against store overwrite
        if (store->canary != CANARY) {
            ERR("bad canary");
            goto fin;
        }

        --store->refcnt;

        if (!store->refcnt) {
            store->canary = 0;
            free(store);
        }
    }

    freed = true;

    fin:
    *buf = ZERO;
    return freed;
}


size_t 
buffet_append (Buffet *buf, const char *src, size_t srclen) 
{
    const Tag tag = TAG(buf);
    const size_t curlen = getlen(buf);
    const size_t curcap = getcap(buf, tag);
    const char *curdata = getdata(buf);
    size_t newlen = curlen + srclen;
    size_t newcap = OVERALLOC*newlen;

    if (tag==SSO) {

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

    } else if (tag==OWN) {

        char *data = (newlen <= curcap) ? buf->ptr.data : grow_own(buf, newcap);
        if (!data) {return 0;}
        memcpy (data+curlen, src, srclen);
        data[newlen] = 0;
        buf->ptr.len = newlen;

    } else if (tag==REF) {

        Store *store = getstore(buf);
        buffet_memcopy (buf, curdata, curlen); 
        -- store->refcnt;
        buffet_append (buf, src, srclen);

    } else {

        buffet_memcopy (buf, curdata, curlen); 
        buffet_append (buf, src, srclen);
    }

    return newlen;
}


Buffet*
buffet_split (const char* src, size_t srclen, const char* sep, size_t seplen, 
    int *outcnt)
{
    int curcnt = 0; 
    Buffet *ret = NULL;
    
    Buffet  parts_local[BUFFET_LIST_LOCAL_MAX]; 
    Buffet *parts = parts_local;
    bool local = true;
    int partsmax = BUFFET_LIST_LOCAL_MAX;

    const char *beg = src;
    const char *end = beg;

    while ((end = strstr(end, sep))) {

        if (curcnt >= partsmax-1) {

            partsmax *= 2;
            size_t newsz = partsmax * sizeof(Buffet);

            if (local) {
                parts = malloc(newsz); 
                if (!parts) {curcnt = 0; goto fin;}
                memcpy (parts, parts_local, curcnt * sizeof(Buffet));
                local = false;
            } else {
                parts = realloc(parts, newsz); 
                if (!parts) {curcnt = 0; goto fin;}
            }
        }

        buffet_memview (&parts[curcnt++], beg, end-beg);
        beg = end+seplen;
        end = beg;
    };
    
    // last part
    buffet_memview (&parts[curcnt++], beg, src+srclen-beg);

    if (local) {
        ret = malloc(curcnt * sizeof(Buffet));
        memcpy (ret, parts, curcnt * sizeof(Buffet));
    } else {
        ret = parts;  
    }

    fin:
    *outcnt = curcnt;
    return ret;
}


Buffet*
buffet_splitstr (const char* src, const char* sep, int *outcnt) {
    return buffet_split(src, strlen(src), sep, strlen(sep), outcnt);
}


void
buffet_list_free (Buffet *list, int cnt)
{
    Buffet *elt = list;
    while(cnt--) {
        buffet_free(elt++);
    }
    free(list);
}


Buffet 
buffet_join (Buffet *parts, int cnt, const char* sep, size_t seplen)
{
    // opt: local if small; none if too big
    size_t *lengths = malloc(cnt*sizeof(*lengths));
    size_t totlen = 0;

    for (int i=0; i < cnt; ++i) {
        size_t len = getlen(&parts[i]);
        totlen += len;
        lengths[i] = len;
    }
    totlen += (cnt-1)*seplen;
    
    Buffet ret;
    buffet_new(&ret, totlen);
    char* cur = (char*)getdata(&ret);
    cur[totlen] = 0; 

    for (int i=0; i < cnt; ++i) {
        const char *eltdata = getdata(&parts[i]);
        size_t eltlen = lengths[i];
        memcpy(cur, eltdata, eltlen);
        cur += eltlen;
        if (i<cnt-1) {
            memcpy(cur, sep, seplen);
            cur += seplen;
        }
    }    

    setlen(&ret, totlen);
    free(lengths);

    return ret;
}


// todo no allocation if ref/vue whole len or stops at end
const char*
buffet_cstr (const Buffet *buf, bool *mustfree) 
{
    const char *data = getdata(buf);
    char *ret = (char*)data;
    bool tofree = false;

    switch(TAG(buf)) {
        
        case SSO:
        case OWN:
            break;

        case VUE:
        case REF: {
            const size_t len = buf->ptr.len;
            
            // already a proper `len` length C string
            if (data[len]==0) break;                

            ret = malloc(len+1);

            if (!ret) {
                ERR("buffet_cstr: failed allocation"); 
                ret = calloc(1,1); // == "\0"
            } else {
                memcpy (ret, data, len);
                ret[len] = 0;
            }
            tofree = true;
        }
    }

    *mustfree = tofree;
    return ret;
}


char*
buffet_export (const Buffet* buf) 
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
buffet_data (const Buffet* buf) {
    return getdata(buf);
}

size_t
buffet_cap (const Buffet* buf) {
    return getcap(buf, TAG(buf));
}

size_t
buffet_len (const Buffet* buf) {
    return getlen(buf);
}

void
buffet_print (const Buffet *buf) {
    printf("%.*s\n", (int)getlen(buf), getdata(buf));
}

void 
buffet_debug (const Buffet* buf) {
    dbg(buf);
}
