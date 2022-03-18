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


#define CANARY 0xAAAAAAAA
#define OVERALLOC 1.6
#define STEP (1ull<<BUFFET_TAG)
#define ZERO ((Buffet){.fill={0}})
#define STORE_DATAOFF offsetof(Store,data)
#define TYPE(buf) ((buf)->sso.tag)
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
getcap (const Buffet *buf, Tag tag) {
    return 
        tag == OWN ? (STEP * buf->ptr.aux)
    :   tag == SSO ? BUFFET_SSO
    :   0;
}

static void 
setlen (Buffet *buf, size_t len) {
    if (TYPE(buf)) buf->ptr.len = len; else buf->sso.len = len;
}

// todo: defensive check tag
static Store*
getstore_own (Buffet *own) {
    return (Store*)(own->ptr.data - STORE_DATAOFF);
}

static Store*
getstore_ref (Buffet *ref) {
    ptrdiff_t off = REFOFF(ref); // for debug
    return (Store*)(ref->ptr.data - off - STORE_DATAOFF);
}


static Store*
new_store (size_t cap)
{
    size_t mem = STORE_DATAOFF+cap+1;
    Store *store = aligned_alloc (sizeof(Store), mem);

    if (!store) {
        ERR("failed Store allocation"); 
        return NULL;
    }
    *store = (Store){
        .refcnt = 0,
        .canary = CANARY,
        .data = {'\0'}
    };

    store->data[cap] = '\0'; // sec

    return store;
}


static char*
new_own (Buffet *dst, size_t cap, const char *src, size_t len)
{
    size_t fincap = cap;
    // round to next STEP
    if (fincap % STEP) fincap = (fincap/STEP+1)*STEP;

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

    Store *store = (Store*)(owned-STORE_DATAOFF);
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
    Store *store = getstore_own(buf);
    
    store = realloc(store, STORE_DATAOFF + newcap + 1);
    if (!store) return NULL;

    char *newdata = store->data;
    assert_aligned(newdata);
    buf->ptr.data = newdata;
    buf->ptr.aux = newcap/STEP; // ?

    return newdata;
}

static void
print_type (const Buffet *buf, char *out) {
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
buffet_new (Buffet *dst, size_t cap)
{
    *dst = ZERO;

    if (cap > BUFFET_SSO) {
        new_own(dst, cap, NULL, 0);
    }
}


void
buffet_strcopy (Buffet *dst, const char* src, size_t len)
{
    *dst = ZERO;
    
    if (len <= BUFFET_SSO) {
        memcpy(dst->sso.data, src, len);
        dst->sso.len = len;
    } else {
        new_own(dst, len, src, len);
    }
}


void
buffet_strview (Buffet *dst, const char* src, size_t len)
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
    Buffet ret;
    buffet_strcopy (&ret, getdata(src)+off, len);
    return ret;
}


Buffet
buffet_view (const Buffet *src, ptrdiff_t off, size_t len)
{
    const char *data = getdata(src); 
    Buffet ret;
    
    switch(TYPE(src)) {
        
        case SSO:
        case VUE: {
            buffet_strview (&ret, data + off, len); 
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
// e.g aliasing (buf1=buf2) :
//   Buf owner = own(data)
//   Buf alias = owner // DANGER
//   buf_free(owner)   // checks canary : OK. Free store, erase canary.
//   buf_free(alias)   // (normally fatal) checks canary : BAD. Do nothing. 
void
buffet_free (Buffet *buf)
{
    const Tag tag = TYPE(buf);

    if (tag == SSO || tag == VUE) {
        *buf = ZERO;
        return;
    }
        
    if (tag == OWN) {
        
        Store *store = getstore_own(buf);

        if (store->canary != CANARY) {
            ERR ("store canary not found");
            *buf = ZERO;
            return;
        }
        // LOG("read refcnt");
        if (!(store->refcnt-1)) {
            store->canary = 0;
            free(store);
            *buf = ZERO;
        } else {
            --store->refcnt;
        }

    } else { // REF

        Store *store = getstore_ref(buf);
        *buf = ZERO; //anyway

        if (store->canary != CANARY) {
            ERR ("store canary not found");
            return;
        }

        if (!--store->refcnt) {
            store->canary = 0;
            free(store);
        }
    }
}


size_t 
buffet_append (Buffet *buf, const char *src, size_t srclen) 
{
    const Tag tag = TYPE(buf);
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

        Store *store = getstore_ref(buf);
        buffet_strcopy (buf, curdata, curlen); 
        -- store->refcnt;
        buffet_append (buf, src, srclen);

    } else {

        buffet_strcopy (buf, curdata, curlen); 
        buffet_append (buf, src, srclen);
    }

    return newlen;
}

#define BUFFET_STACK_MEM 1024
#define LIST_LOCAL_MAX (BUFFET_STACK_MEM/sizeof(Buffet))

Buffet*
buffet_splitstr (const char* src, size_t srclen, const char* sep, size_t seplen, 
    int *outcnt)
{
    int curcnt = 0; 
    Buffet *ret = NULL;
    
    Buffet  list_local[LIST_LOCAL_MAX]; 
    Buffet *list = list_local;
    bool local = true;
    int listmax = LIST_LOCAL_MAX;

    const char *beg = src;
    const char *end = src;

    while ((end = strstr(end, sep))) {

        if (curcnt >= listmax-1) {

            listmax *= 2;
            size_t newsz = listmax * sizeof(Buffet);

            if (local) {
                list = malloc(newsz); 
                if (!list) {curcnt = 0; goto fin;}
                memcpy (list, list_local, curcnt * sizeof(Buffet));
                local = false;
            } else {
                list = realloc(list, newsz); 
                if (!list) {curcnt = 0; goto fin;}
            }
        }

        buffet_strview (&list[curcnt++], beg, end-beg);
        end += seplen;
        beg = end;
    };
    
    // last part
    buffet_strview (&list[curcnt++], beg, src+srclen-beg);

    if (local) {
        ret = malloc(curcnt * sizeof(Buffet));
        memcpy (ret, list, curcnt * sizeof(Buffet));
    } else {
        ret = list;  
    }

    fin:
    *outcnt = curcnt;
    return ret;
}


void
buffet_list_free (Buffet *list, int cnt)
{
    Buffet *elt = list;
    while(cnt--) {
        buffet_free(elt++); //useless if split-view
    }
    free(list);
}


Buffet 
buffet_join (Buffet *list, int cnt, const char* sep, size_t seplen)
{
    // opt: local if small; no alloc if too big
    size_t *lengths = malloc(cnt*sizeof(*lengths));
    size_t totlen = 0;

    for (int i=0; i < cnt; ++i) {
        size_t len = getlen(&list[i]);
        totlen += len;
        lengths[i] = len;
    }
    totlen += (cnt-1)*seplen;
    
    Buffet ret;
    buffet_new(&ret, totlen);
    char* cur = (char*)getdata(&ret);
    cur[totlen] = 0; 

    for (int i=0; i < cnt; ++i) {
        const char *eltdata = getdata(&list[i]);
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
    return getcap(buf, TYPE(buf));
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
buffet_debug (const Buffet* buf) 
{
    char tag[5];
    print_type(buf, tag);
    bool mustfree;
    const char *cstr = buffet_cstr(buf, &mustfree);

    printf ("tag:%s cap:%zu len:%zu data:'%s' cstr:'%s'\n", 
    tag, buffet_cap(buf), buffet_len(buf), buffet_data(buf), cstr);

    if (mustfree) free((char*)cstr);
}