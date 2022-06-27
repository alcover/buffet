/*
Buffet - All-inclusive Buffer for C
Copyright (C) 2022 - Francois Alcover <francois [on] alcover [dot] fr>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buffet.h"
#include "log.h"

typedef enum {SSO=0, OWN, SSV, VUE} Tag;

// Shared heap allocation
typedef struct {
    size_t   cap;       // capacity
    char*    end;       // current end of data (for append in place)
    uint32_t refcnt;    // number of owners
    volatile
    uint32_t canary;    // prevents accessing stale store
    char     data[1];
} Store;

#define CANARY 0xbeacface   
#define OVERALLOC 2  // growth factor
#define ZERO BUFFET_ZERO
#define DATAOFF offsetof(Store,data)
#define TAG(buf) ((buf)->sso.tag)
#define MEM(cap) (DATAOFF+cap+1) // total mem for store of capacity `cap`

#define ERR_ALLOC ERR("Failed allocation\n")
#define ERR_CANARY WARN("Bad canary. Double free ?\n")
#define ERR_REFCNT WARN("Bad refcnt. Rogue alias ?\n")
#define ERR_SHARED WARN("Shared store\n")

static inline char*
getdata (const Buffet *buf, Tag tag) {
    return tag==SSO ? (char*)buf->sso.data : buf->ptr.data;
}

static inline size_t
getlen (const Buffet *buf, Tag tag) {
    return tag==SSO ? buf->sso.len : buf->ptr.len;
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

static inline Buffet
new_vue (const char *src, size_t len)
{
    return (Buffet) {
        .ptr.data = (char*)src,
        .ptr.len = len,
        // .ptr.off = 0,
        .ptr.tag = VUE
    };
}

// view on SSO
static inline Buffet
new_ssovue (Buffet *src, size_t len, size_t off)
{
    if (src->sso.refcnt >= 255) {
        ERR("reached max views on SSO. Returning BUFFET_ZERO.\n");
        return ZERO;
    }

    ++ src->sso.refcnt;

    return (Buffet) {
        .ptr.data = (char*)src->sso.data + off,
        .ptr.len = len,
        .ptr.off = off,
        .ptr.tag = SSV
    };
}

static void 
dbg (const Buffet* buf) 
{
    Tag tag = TAG(buf);
    char stag[10];
    switch(tag) {
        case SSO: sprintf(stag,"SSO"); break;
        case OWN: sprintf(stag,"OWN"); break;
        case VUE: sprintf(stag,"VUE"); break;
        case SSV: sprintf(stag,"SSV"); break;
    }

    bool mustfree;
    const char *cstr = buffet_cstr(buf, &mustfree);

    LOG("%s cap:%zu len:%zu cstr:'%s'", 
    stag, buffet_cap(buf), buffet_len(buf), cstr);

    if (mustfree) free((char*)cstr);
}

//=============================================================================
// Public
//=============================================================================

Buffet
buffet_new (size_t cap)
{
    Buffet ret = ZERO;

    if (cap > BUFFET_SSOMAX) {
        Store *store = new_store(cap, 0);
        if (store) 
            return (Buffet) {
                .ptr.data = store->data,
                .ptr.len = 0,
                .ptr.off = 0,
                .ptr.tag = OWN
            };
    }

    return ret;
}


Buffet
buffet_memcopy (const char *src, size_t len)
{
    Buffet ret = ZERO;
    
    if (len <= BUFFET_SSOMAX) {
        memcpy(ret.sso.data, src, len);
        ret.sso.len = len;
    } else {
        Store *store = new_store(len, len);
        if (store) {
            char *data = store->data;
            memcpy(data, src, len);
            data[len] = 0;
            ret = (Buffet) {
                .ptr.data = data,
                .ptr.len = len,
                .ptr.off = 0,
                .ptr.tag = OWN
            };
        }
    }

    return ret;
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

// todo optim
Buffet
buffet_copyall (const Buffet *src)
{
    Tag tag = TAG(src);
    return buffet_memcopy (getdata(src,tag), getlen(src,tag));
}

// todo better naming ?
Buffet
buffet_dup (const Buffet *src)
{
    Tag tag = TAG(src);
    Buffet ret = *src;

    switch(tag) {
        
        case SSO:
            ret.sso.refcnt = 0;
            break;

        case OWN: {
            // size_t len = src->ptr.len;
            // return new_own (len, src->ptr.data, len);
            Store *store = getstore(src); //check?
            ++ store->refcnt;
            break;
        }
        
        case VUE:
            break;

        case SSV: {
            BuffetSSO *target = (BuffetSSO*)(src->ptr.data - src->ptr.off);
            ++ target->refcnt;
            break;
        }
    }

    return ret;
}


Buffet
buffet_view (Buffet *src, ptrdiff_t off, size_t len)
{
    Tag tag = TAG(src);
    size_t srclen = getlen(src,tag);

    // clipping
    if ((size_t)off >= srclen) return ZERO;
    if (off+len > srclen) len = srclen-off;

    switch(tag) {

        case SSO: 
            return new_ssovue (src, len, off);

        case VUE:
            // sub-vue on src's target
            return new_vue (src->ptr.data + off, len);

        case SSV: {
            size_t vueoff = src->ptr.off;
            Buffet *target = (Buffet*)(src->ptr.data - vueoff);
            return new_ssovue (target, len, vueoff+off);
        }

        case OWN: {
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
    }

    return ZERO;
}


bool
buffet_free (Buffet *buf)
{   
    Tag tag = TAG(buf);

    if (tag==SSO && buf->sso.refcnt) {
        buf->sso.len = 0;
        return true;
    }

    if (tag==OWN) {
            
        Store *store = getstore(buf);

        if (store->canary != CANARY) {
            ERR_CANARY; //buffet_debug(buf);
        } else if (!store->refcnt) {
            ERR_REFCNT;
        } else {
            -- store->refcnt;
            if (!store->refcnt) {
                store->canary = 0;
                free(store);
            } else {
                // return false;
            }
        }

    } else if (tag==SSV) {
        BuffetSSO *target = (BuffetSSO*)(buf->ptr.data - buf->ptr.off);
        -- target->refcnt; //check?
    }

    memset(buf, 0, sizeof(Buffet));
    // setlen(buf, 0, tag);
    return true;
}




// todo self-data cases
size_t
buffet_cat (Buffet *dst, const Buffet *buf, const char *src, size_t srclen)
{
    Tag tag = TAG(buf);
    char *curdata, *newdata;
    size_t curlen = 0, newlen = 0;
    Buffet out = ZERO;

    if (tag == SSO) {

        curdata = (char*)buf->sso.data;
        curlen = buf->sso.len;
        newlen = curlen + srclen;

        if (newlen <= BUFFET_SSOMAX) {

            out.sso.refcnt = 0;
            out.sso.len = newlen;
            out.sso.tag = SSO;
            newdata = out.sso.data;
            goto fin;
        
        } else if (dst == buf && buf->sso.refcnt) {
            // In-place append would mutate SSO `buf` to OWN
            // while views still point directly into `buf`
            ERR ("In-place buffet_cat would invalidate views on SSO\n");
            return 0;
        }
    
    } else {

        curdata = buf->ptr.data;
        curlen = buf->ptr.len;
        newlen = curlen + srclen;

        // opinion: no small owner. Downsized owner should convert to SSO
        // if (newlen <= BUFFET_SSOMAX) 

        if (tag == OWN) {

            Store *store = getstore(buf);
            newdata = buf->ptr.data + curlen;
            const size_t off = buf->ptr.off;

            // In-place if src fits and buf is unique owner or lies at store end
            if ((off + newlen <= store->cap) &&
               (store->refcnt < 2 || newdata == store->end)) {
                // LOG("cat: inplace");
                memcpy(newdata, src, srclen);
                newdata[srclen] = 0;
                store->end = newdata + srclen;

                *dst = (Buffet){
                    .ptr.data = curdata,
                    .ptr.len = newlen,
                    .ptr.off = off,
                    .ptr.tag = OWN
                };
                ++ store->refcnt;
                return newlen;
            }
        }
    }

    Store *newstore = new_store (OVERALLOC*newlen, newlen);
    if (!newstore) {
        *dst = *buf; //?
        return 0;
    }

    newdata = newstore->data;
    out = (Buffet){
        .ptr.data = newdata,
        .ptr.len = newlen,
        .ptr.off = 0,
        .ptr.tag = OWN
    };

    fin:
    memcpy (newdata, curdata, curlen);
    memcpy (newdata+curlen, src, srclen);
    newdata[newlen] = 0;

    *dst = out;    
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

    // set length
    if (rettag==SSO) ret.sso.len = totlen; 
    else ret.ptr.len = totlen;

    free(lengths);

    return ret;
}


const char*
buffet_cstr (const Buffet *buf, bool *mustfree) 
{
    Tag tag = TAG(buf);
    const size_t len = getlen(buf, tag);
    bool tofree = false;
    char *ret;

    if (!len) {
        ret = ""; //?
    } else if (tag==SSO) {
        ret = (char*)buf->sso.data;
    } else {
        // size_t len = buf->ptr.len;
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
    if (tag==OWN) {
        Store* store = getstore(buf);
        return store->cap;
    }
    return 0;
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
