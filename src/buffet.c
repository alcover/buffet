/*
Buffet - All-inclusive Buffer for C
Copyright (C) 2022 - Francois Alcover <francois [on] alcover [dot] fr>
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdalign.h>
#include <string.h>
#include <assert.h>

#include "buffet.h"
#include "log.h"

typedef enum {
    SSO = 0,
    OWN,
    REF,
    VUE
} Tag;

typedef struct {
    uint32_t refcnt;
    volatile
    uint32_t canary;
    char     data[];
} Store;

#define STEPLOG 3
#define STEP (1u<<STEPLOG)
#define CANARY 0xfacebeac
#define OVERALLOC 1.6
#define ZERO ((Buffet){.fill={0}})
#define DATAOFF offsetof(Store,data)
#define TAG(buf) ((buf)->sso.tag)
#define CAP(buf) ((size_t)buf->ptr.aux << STEPLOG)
#define REFOFF(ref) (((size_t)(ref)->ptr.aux << STEPLOG) + ((intptr_t)(ref)->ptr.data % STEP))
#define OWNSTORE(buf) ((Store*)(buf->ptr.data - DATAOFF))
#define REFSTORE(buf) ((Store*)(buf->ptr.data - REFOFF(buf) - DATAOFF))
#define ERR_ALLOC ERR("failed allocation\n");

static_assert (sizeof(Buffet) == BUFFET_SIZE, "Buffet size");
static_assert (alignof(Store*) % STEP == 0, "Store align");
static_assert (offsetof(Store,data) % STEP == 0, "Store.data align");

static inline const char*
getdata (const Buffet *buf, Tag tag) {
    return tag ? buf->ptr.data : buf->sso.data;
}

static inline size_t
getlen (const Buffet *buf, Tag tag) {
    return tag ? buf->ptr.len : buf->sso.len;
}

static inline void 
setlen (Buffet *buf, size_t len, Tag tag) {
    if (tag) buf->ptr.len = len; else buf->sso.len = len;
}


static inline char*
new_own (Buffet *dst, size_t cap, const char *src, size_t len)
{
    // round to next STEP
    size_t fincap = ((cap>>STEPLOG) + 1) << STEPLOG;
    Store *store = malloc(DATAOFF + fincap + 1);

    if (!store) {
        ERR_ALLOC
        return NULL;
    }

    *store = (Store){.refcnt=1, .canary=CANARY};

    char *data = store->data;
    if (src) memcpy(data, src, len);
    data[len] = 0;

    *dst = (Buffet) {
        .ptr.data = data,
        .ptr.len = len,
        .ptr.aux = fincap >> STEPLOG,
        .ptr.tag = OWN
    };

    return data;
}


static inline void
new_ref (Buffet *dst, const char *owned, ptrdiff_t off, size_t len) 
{
    Store *store = (Store*)(owned-DATAOFF);

    if (store->canary != CANARY) {
        ERR("bad canary\n");
        *dst = ZERO;
        return;
    }

    *dst = (Buffet) {
        .ptr.data = (char*)(owned + off),
        .ptr.len = len,
        .ptr.aux = off >> STEPLOG,
        .ptr.tag = REF     
    };

    ++ store->refcnt;
}


static inline void
new_vue (Buffet *dst, const char *src, size_t len)
{
    *dst = (Buffet) {
        .ptr.data = (char*)src,
        .ptr.len = len,
        .ptr.tag = VUE
    };
}


static inline char*
grow_own (Buffet *buf, size_t newcap)
{
    Store *store = OWNSTORE(buf);

    if (store->canary != CANARY) {
        ERR("bad canary\n");
        return NULL;
    }
    
    store = realloc(store, DATAOFF + newcap + 1);
    if (!store) return NULL;

    char *newdata = store->data;

    buf->ptr.data = newdata;
    buf->ptr.aux = newcap >> STEPLOG;

    return newdata;
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

    LOG("tag:%s cap:%zu len:%zu cstr:'%s'", 
    tag, buffet_cap(buf), buffet_len(buf), cstr);

    if (mustfree) free((char*)cstr);
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
buffet_memcopy (Buffet *dst, const char *src, size_t len)
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
buffet_memview (Buffet *dst, const char *src, size_t len) {
    return new_vue(dst, src, len);
}


Buffet
buffet_copy (const Buffet *src, ptrdiff_t off, size_t len)
{
    const Tag tag = TAG(src);

    if (off+len > getlen(src,tag)) return ZERO;

    Buffet ret;
    buffet_memcopy (&ret, getdata(src,tag)+off, len);
    return ret;
}


Buffet
buffet_clone (const Buffet *src)
{
    Tag tag = TAG(src);
    Buffet ret;
    
    switch(tag) {
        case SSO:
        case VUE:
            ret = *src;
            break;
        
        case OWN: {
            size_t len = src->ptr.len;
            new_own (&ret, len, src->ptr.data, len);
        }   break;
        
        case REF: {
            Store *store = REFSTORE(src);
            ++ store->refcnt;
            ret = *src;
        }   break;
    }

    return ret;
}


Buffet
buffet_view (const Buffet *src, ptrdiff_t off, size_t len)
{
    const Tag tag = TAG(src);

    if (off+len > getlen(src,tag)) return ZERO;
    
    const char *data = getdata(src,tag); 
    Buffet ret;
    
    switch(tag) {
        
        case SSO:
        case VUE:
            new_vue (&ret, data + off, len); 
            break;
        
        case OWN:
            new_ref (&ret, data, off, len);
            break;
        
        case REF: {
            uint32_t refoff = REFOFF(src);
            const char *ownerdata = data-refoff; 
            new_ref (&ret, ownerdata, refoff+off, len);
        }   break;
    }

    return ret;
}


// The canary prevents some memory faults (if not optimized-out).
// e.g aliasing :
//   Buf owner = own(data)
//   Buf alias = owner // DANGER
//   buf_free(owner)   // checks canary : OK. Free store, erase canary.
//   buf_free(alias)   // (normally fatal) checks canary : BAD. Do nothing. 
bool
buffet_free (Buffet *buf)
{
    Tag tag = TAG(buf);
    bool ret = true;

    if (tag==OWN||tag==REF) {
        
        Store *store = (tag==OWN) ? OWNSTORE(buf) : REFSTORE(buf);

        if (store->canary != CANARY) {
            // store freed/overwritten
            ERR("bad canary\n");
            ret = false;
        } else if (!store->refcnt) {
            // aliasing
            ERR("bad refcnt\n");
            ret = false;
        } else {
            --store->refcnt;
            if (!store->refcnt) {
                store->canary = 0;
                free(store);
            }
        }
    }

    *buf = ZERO;
    return ret;
}



size_t 
buffet_append (Buffet *buf, const char *src, size_t srclen) 
{
    const Tag tag = TAG(buf);
    const size_t curlen = tag ? buf->ptr.len : buf->sso.len;
    const size_t newlen = curlen + srclen;
    const size_t newcap = OVERALLOC*newlen;
    char *data;

    if (tag==SSO) {

        if (newlen <= SSOMAX) {
            memcpy (buf->sso.data+curlen, src, srclen);
            buf->sso.data[newlen] = 0;
            buf->sso.len = newlen;
        } else {
            data = new_own (buf, newcap, buf->sso.data, buf->sso.len);
            if (!data) {return 0;}
            memcpy (data+curlen, src, srclen);
            data[newlen] = 0;
            buf->ptr.len = newlen;
        }

    } else if (tag==OWN) {

        if (newlen <= CAP(buf)) {
            data = buf->ptr.data; // check store ?
        } else {
            data = grow_own(buf, newcap);
            if (!data) return 0;
        }
        
        memcpy (data+curlen, src, srclen);
        data[newlen] = 0;
        buf->ptr.len = newlen;

    } else if (tag==REF) {

        Store *store = REFSTORE(buf);

        if (store->canary != CANARY) {
            ERR("bad canary\n");
            *buf = ZERO; // then append ?
            return 0;
        }

        buffet_memcopy (buf, buf->ptr.data, curlen); 
        -- store->refcnt;
        buffet_append (buf, src, srclen);

    } else {

        buffet_memcopy (buf, buf->ptr.data, curlen); 
        buffet_append (buf, src, srclen);
    }

    return newlen;
}


#define LIST_STACK_MAX (BUFFET_STACK_MEM/sizeof(Buffet))

Buffet*
buffet_split (const char* src, size_t srclen, const char* sep, size_t seplen, 
    int *outcnt)
{
    int curcnt = 0; 
    Buffet *ret = NULL;
    
    Buffet  parts_local[LIST_STACK_MAX]; 
    Buffet *parts = parts_local;
    bool local = true;
    int partsmax = LIST_STACK_MAX;

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

        new_vue (&parts[curcnt++], beg, end-beg);
        beg = end+seplen;
        end = beg;
    };
    
    // last part
    new_vue (&parts[curcnt++], beg, src+srclen-beg);

    if (local) {
        size_t outlen = curcnt * sizeof(Buffet);
        ret = malloc(outlen);
        memcpy (ret, parts, outlen);
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


Buffet 
buffet_join (const Buffet *parts, int cnt, const char* sep, size_t seplen)
{
    // opt: local if small; none if too big
    size_t *lengths = malloc(cnt*sizeof(*lengths));
    size_t totlen = 0;

    for (int i=0; i < cnt; ++i) {
        const Buffet *part = &parts[i];
        size_t len = getlen(part,TAG(part));
        totlen += len;
        lengths[i] = len;
    }
    totlen += (cnt-1)*seplen;
    
    Buffet ret;
    buffet_new(&ret, totlen);
    const Tag rettag = TAG(&ret);
    char* cur = (char*)getdata(&ret,rettag);
    cur[totlen] = 0; 

    for (int i=0; i < cnt; ++i) {
        const Buffet *part = &parts[i];
        const char *eltdata = getdata(part,TAG(part));
        size_t eltlen = lengths[i];
        memcpy(cur, eltdata, eltlen);
        cur += eltlen;
        if (i<cnt-1) {
            memcpy(cur, sep, seplen);
            cur += seplen;
        }
    }    

    setlen(&ret, totlen, rettag);
    free(lengths);

    return ret;
}


const char*
buffet_cstr (const Buffet *buf, bool *mustfree) 
{
    const Tag tag = TAG(buf);
    const char *data = getdata(buf,tag);
    char *ret = (char*)data;
    bool tofree = false;

    switch(tag) {
        
        case SSO:
        case OWN:
            break;

        case VUE:
        case REF: {
            const size_t len = buf->ptr.len;
            // already a proper `buf.len`-length C string
            if (data[len]==0) break;                

            ret = malloc(len+1);

            if (!ret) {
                ERR_ALLOC
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
    const Tag tag = TAG(buf);
    const size_t len = getlen(buf,tag);
    const char* data = getdata(buf,tag);
    char* ret = malloc(len+1);

    if (!ret) {
        ERR_ALLOC
        return calloc(1,1);
    }
    
    memcpy (ret, data, len);
    ret[len] = 0;

    return ret;
}

const char*
buffet_data (const Buffet* buf) {
    return getdata(buf, TAG(buf));
}

size_t
buffet_len (const Buffet* buf) {
    return getlen(buf, TAG(buf));
}

size_t
buffet_cap (const Buffet* buf) {
    Tag tag = TAG(buf);
    return 
        tag == OWN ? CAP(buf)
    :   tag == SSO ? SSOMAX
    :   0;
}

void
buffet_print (const Buffet *buf) {
    Tag tag = TAG(buf);
    LOG("%.*s", (int)getlen(buf,tag), getdata(buf,tag));
}

void 
buffet_debug (const Buffet* buf) {
    dbg(buf);
}
