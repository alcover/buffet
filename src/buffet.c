/*
Buffet - All-inclusive Buffer for C
Copyright (C) 2022 - Francois Alcover <francois [on] alcover [dot] fr>
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdalign.h>
#include <string.h>

#include "buffet.h"
#include "log.h"

typedef enum {SSO=0, OWN, VUE} Tag;

typedef struct {
    size_t   cap;
    char*    end; // for append in place
    uint32_t refcnt;
    volatile
    uint32_t canary;
    char     data[1];
} Store;

#define CANARY 0xbeacface   // prevents accessing stale data
#define OVERALLOC 2         // growth factor
#define ZERO BUFFET_ZERO
#define DATAOFF offsetof(Store,data)
#define TAG(buf) ((buf)->sso.tag)
#define MEM(cap) (DATAOFF+cap+1) // space for a store of capacity `cap`
#define ERASE(buf) memset(buf, 0, sizeof(Buffet))

#define ERR_ALLOC ERR("Failed allocation\n")
#define ERR_CANARY WARN("Bad canary. Double free ?\n")
#define ERR_REFCNT WARN("Bad refcnt. Rogue alias ?\n")
#define ERR_SHARED WARN("Shared store\n")

static char*
getdata (const Buffet *buf, Tag tag) {
    return tag==SSO ? (char*)buf->sso.data : buf->ptr.data;
}

static size_t
getlen (const Buffet *buf, Tag tag) {
    return tag==SSO ? buf->sso.len : buf->ptr.len;
}

static void 
setlen (Buffet *buf, size_t len, Tag tag) {
    if (tag==SSO) buf->sso.len = len; else buf->ptr.len = len;
}

static inline Store*
getstore (const Buffet *buf) {
    return (Store*)(buf->ptr.data - buf->ptr.off - DATAOFF);
}


static inline Store*
new_store (size_t cap, size_t len)
{
    Store *store = malloc(MEM(cap));
    if (!store) {ERR_ALLOC; return NULL;}

    *store = (Store){
        .cap = cap,
        .end = store->data + len,
        .refcnt = 1, 
        .canary = CANARY,
    };

    return store;
}


static Buffet
new_own (size_t cap, const char *src, size_t len)
{
    Store *store = new_store(cap, len);
    if (!store) {return ZERO;}

    char *data = store->data;
    memcpy(data, src, len);
    data[len] = 0;

    return (Buffet) {
        .ptr.data = data,
        .ptr.len = len,
        .ptr.off = 0,
        .ptr.tag = OWN
    };
}


static inline Buffet
new_vue (const char *src, size_t len)
{
    return (Buffet) {
        .ptr.data = (char*)src,
        .ptr.len = len,
        .ptr.off = 0,
        .ptr.tag = VUE
    };
}


static void 
dbg (const Buffet* buf) 
{
    char tag[10];
    switch(TAG(buf)) {
        case SSO: sprintf(tag,"SSO"); break;
        case OWN: sprintf(tag,"OWN"); break;
        case VUE: sprintf(tag,"VUE"); break;
    }

    bool mustfree;
    const char *cstr = buffet_cstr(buf, &mustfree);

    LOG("%s cap:%zu len:%zu cstr:'%s'", 
    tag, buffet_cap(buf), buffet_len(buf), cstr);

    if (mustfree) free((char*)cstr);
}

//=============================================================================
// Public
//=============================================================================

Buffet
buffet_new (size_t cap)
{
    if (cap <= BUFFET_SSOMAX) {
        return ZERO;
    } else {
        Store *store = new_store(cap, 0);
        if (!store) return ZERO;
        return (Buffet) {
            .ptr.data = store->data,
            .ptr.len = 0,
            .ptr.off = 0,
            .ptr.tag = OWN
        };
    }
}


Buffet
buffet_memcopy (const char *src, size_t len)
{
    if (len <= BUFFET_SSOMAX) {
        Buffet ret = ZERO;
        memcpy(ret.sso.data, src, len);
        ret.sso.len = len;
        return ret;
    }

    return new_own(len, src, len);
}


Buffet
buffet_memview (const char *src, size_t len) {
    return new_vue(src, len);
}


Buffet
buffet_copy (const Buffet *src, ptrdiff_t off, size_t len)
{
    Tag tag = TAG(src);

    if (off+len > getlen(src,tag)) {
        return ZERO;
    } else {
        return buffet_memcopy (getdata(src,tag)+off, len);
    }
}


Buffet
buffet_dup (const Buffet *src)
{
    Tag tag = TAG(src);
    
    if (tag==OWN) {

        size_t len = src->ptr.len;
        return new_own (len, src->ptr.data, len); // or inc rc, ret src + cow?
    }

    return *src;
}
 

Buffet
buffet_view (Buffet *src, ptrdiff_t off, size_t len)
{
    // if (!len) return ZERO;

    Tag tag = TAG(src);
    size_t srclen = getlen(src,tag);

    // Enforce clipping
    if ((size_t)off >= srclen) return ZERO;
    if (off+len > srclen) len = srclen-off;

    if (tag==VUE) {
        return new_vue (src->ptr.data + off, len); //?
    }

    if (tag==SSO) {

        // convert src to OWN to get a refcnt.
        // (SSO refcnt was tested and seemed too slow and branchy)
        // todo: implications
        //   ex: if SSO had a VUE on it through `view(sso.data)`
        //       warn bad practice ? user should `view(sso)`
        
        Buffet own = new_own (srclen, src->sso.data, srclen);
        if (!own.ptr.data) return ZERO;
        *src = own;
    }

    Store *store = getstore(src);

    if (store->canary != CANARY) {
        ERR_CANARY; return ZERO;
    }

    ++ store->refcnt; 

    return (Buffet) {
        .ptr.data = src->ptr.data + off,
        .ptr.len = len,
        .ptr.off = src->ptr.off + off,
        .ptr.tag = OWN     
    };              
}


bool
buffet_free (Buffet *buf)
{   
    Tag tag = TAG(buf);

    if (tag==OWN) {
            
        Store *store = getstore(buf);

        if (store->canary != CANARY) {
            ERR_CANARY;
        } else if (!store->refcnt) {
            ERR_REFCNT;
        } else {
            --store->refcnt;
            if (!store->refcnt) {
                store->canary = 0;
                free(store);
            } else {
                return false;
            }
        }
    }

    ERASE(buf);
    return true;
}



// todo: other crazy self-appends
size_t 
buffet_append (Buffet *buf, const char *src, size_t srclen) 
{
    Tag tag = TAG(buf);
    char *curdata, *newdata;
    size_t curlen, newlen = 0;

    if (tag == SSO) {

        curdata = buf->sso.data;
        curlen = buf->sso.len;
        newlen = curlen + srclen;

        if (newlen <= BUFFET_SSOMAX) {
            newdata = curdata + curlen;
            memcpy (newdata, src, srclen);
            newdata[srclen] = 0;
            buf->sso.len = newlen;
            return newlen;
        }
    
    } else {

        curdata = buf->ptr.data;
        curlen = buf->ptr.len;
        newlen = curlen + srclen;

        // choice: no small owner. Downsized owner converts to SSO
        // if (newlen <= BUFFET_SSOMAX) 

        if (tag == OWN) {

            Store *store = getstore(buf);
            newdata = buf->ptr.data + curlen;

            // In-place if src fits and buf is unique owner or lies at store end
            if ((buf->ptr.off + newlen <= store->cap) &&
               (store->refcnt < 2 || newdata == store->end)) {
                
                memcpy(newdata, src, srclen);
                newdata[srclen] = 0;
                store->end = newdata + srclen;
                buf->ptr.len = newlen;
                return newlen;
            }
        
            // detach
            -- store->refcnt; //check?
        }
    }

    Store *newstore = new_store (OVERALLOC*newlen, newlen);
    if (!newstore) return 0;
    newdata = newstore->data;

    memcpy (newdata, curdata, curlen);
    memcpy (newdata+curlen, src, srclen);
    newdata[newlen] = 0;

    *buf = (Buffet){
        .ptr.data = newdata,
        .ptr.len = newlen,
        .ptr.off = 0,
        .ptr.tag = OWN
    };
    
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

        parts[curcnt++] = new_vue(beg, end-beg);
        beg = end+seplen;
        end = beg;
    };
    
    // last part
    parts[curcnt++] = new_vue(beg, src+srclen-beg);

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
    // optim: local if small; none if too big ?
    size_t *lengths = malloc(cnt*sizeof(*lengths));
    size_t totlen = 0;

    for (int i=0; i < cnt; ++i) {
        const Buffet *part = &parts[i];
        size_t len = getlen(part,TAG(part));
        totlen += len;
        lengths[i] = len;
    }
    
    totlen += (cnt-1)*seplen;
    
    Buffet ret = buffet_new(totlen);
    const Tag rettag = TAG(&ret);
    char* cur = getdata(&ret,rettag);
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
    Tag tag = TAG(buf);
    bool tofree = false;
    char *ret;

    if (tag==SSO) {
        ret = (char*)buf->sso.data;
    } else {
        size_t len = buf->ptr.len;
        const char *data = buf->ptr.data;
        if (data[len]==0) {
            ret = (char*)data;
        } else {
            ret = malloc(len+1);
            if (!ret) {
                ERR_ALLOC;
                ret = calloc(1,1);
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
        ERR_ALLOC; return NULL;
    } else {
        memcpy (ret, data, len);
    }

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
    if (tag==SSO) return BUFFET_SSOMAX;
    if (tag==VUE) return 0;
    Store* store = getstore(buf);
    return store->cap;
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
