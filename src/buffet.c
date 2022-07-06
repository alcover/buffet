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

// shared heap allocation
typedef struct {
    size_t   cap;       // capacity
    size_t   len;       // current length (for append in place)
    uint32_t refcnt;    // number of co-owners
    volatile
    uint32_t canary;    // prevents accessing stale store
    char     data[1];
} Store;

#define CANARY 0xbeacface   
#define OVERALLOC 2  // growth factor
#define SSO_MAXREF 255 // maximum number of views on an SSO
#define ZERO BUFFET_ZERO // neutralized empty Buffet
#define DATAOFF offsetof(Store,data)
#define TAG(buf) ((buf)->sso.tag)
#define STOREMEM(cap) (DATAOFF+(cap)+1) // alloc for store of capacity `cap`

#define ERR_ALLOC ERR("Failed allocation\n")
#define ERR_CANARY WARN("Bad canary. Double free ?\n")
#define ERR_REFCNT WARN("Bad refcnt. Rogue alias ?\n")

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
newstore (size_t cap, size_t len)
{
    Store *store = malloc(STOREMEM(cap));
    if (!store) {ERR_ALLOC; return NULL;}

    *store = (Store){
        .cap = cap,
        .len = len,
        .refcnt = 1, 
        .canary = CANARY,
    };

    return store;
}

static inline Buffet
newvue (const char *src, size_t len)
{
    return (Buffet) {
        .ptr.data = (char*)src,
        .ptr.len = len,
        .ptr.tag = VUE
    };
}

// view on SSO
static inline Buffet
newssovue (Buffet *src, size_t len, size_t off)
{
    if (src->sso.refcnt >= SSO_MAXREF) {
        ERR("reached max views on SSO.\n");
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
dbgstore (const Store *store) 
{
    const char *data = store->data;
    LOG("store: cap:%zu refcnt:%d data:\"%.*s\"", 
        store->cap, store->refcnt, (int)(store->len +1), data);
}

static void 
dbg (const Buffet* buf) 
{
    Tag tag = TAG(buf);
    char stag[10] = {0};
    switch(tag) {
        case SSO: sprintf(stag,"SSO"); break;
        case OWN: sprintf(stag,"OWN"); break;
        case VUE: sprintf(stag,"VUE"); break;
        case SSV: sprintf(stag,"SSV"); break;
    }
    char *data = getdata(buf,tag);
    size_t len = getlen(buf,tag);
    
    LOG("%s %zu \"%.*s\"", stag, len, (int)len, data);
}

//============================================================================
// Public
//============================================================================

/**
 * Create a new empty Buffet
 * @param[in] cap capacity
 */
Buffet
bft_new (size_t cap)
{
    Buffet ret = ZERO;

    if (cap > BUFFET_SSOMAX) {
        Store *store = newstore(cap, 0);
        if (store) {
            return (Buffet) {
                .ptr.data = store->data,
                .ptr.len = 0,
                .ptr.off = 0,
                .ptr.tag = OWN
            };
        }
    }

    return ret;
}

/**
 * Create a new Buffet copying a range of bytes
 * @param[in] src the source address
 * @param[in] len the arbitrary length of the copy
 */
Buffet
bft_memcopy (const char *src, size_t len)
{
    Buffet ret = ZERO;
    
    if (len <= BUFFET_SSOMAX) {
        memcpy(ret.sso.data, src, len);
        ret.sso.len = len;
    } else {
        Store *store = newstore(len, len);
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

/**
 * Create a new Buffet viewing a range of bytes
 * @param[in] src the source address
 * @param[in] len the arbitrary length of the view
 */
Buffet
bft_memview (const char *src, size_t len) {
    return newvue(src, len);
}

/**
 * Create a new Buffet copying a Buffet's data
 * @param[in] src the source Buffet
 * @param[in] off offset to start from
 * @param[in] len length in bytes
 */
Buffet
bft_copy (const Buffet *src, size_t off, size_t len)
{
    Tag tag = TAG(src);

    if (off+len > getlen(src,tag)) {
        return ZERO;
    } else {
        return bft_memcopy(getdata(src,tag)+off, len);
    }
}

/**
 * Create a new Buffet copying a Buffet's whole data
 * @param[in] src the source Buffet
 */
// todo optim + check
Buffet
bft_copyall (const Buffet *src)
{
    Tag tag = TAG(src);
    return bft_memcopy(getdata(src,tag), getlen(src,tag));
}

/**
 * Create a shallow copy of a Buffet.
 * Use this instead of aliasing a Buffet.
 * Only an SSO gets its data actually copied.
 * @param[in] src the source Buffet
 */
Buffet
bft_dup (const Buffet *src)
{
    Tag tag = TAG(src);
    Buffet ret = *src;

    switch(tag) {
        
        case SSO:
            ret.sso.refcnt = 0;
            break;

        case OWN: {
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

/**
 * Create a new Buffet viewing a Buffet
 * @param[in] src the source Buffet
 * @param[in] off offset to start from
 * @param[in] len length in bytes
 */
Buffet
bft_view (Buffet *src, size_t off, size_t len)
{
    Tag tag = TAG(src);
    size_t srclen = getlen(src,tag);

    if (!len||off>=srclen) return ZERO;
    // clipping
    if (off+len > srclen) len = srclen-off;

    switch(tag) {

        case SSO: 
            return newssovue(src, len, off);

        case VUE:
            // sub-vue on src's target
            return newvue(src->ptr.data + off, len);

        case SSV: {
            size_t vueoff = src->ptr.off;
            Buffet *target = (Buffet*)(src->ptr.data - vueoff);
            return newssovue(target, len, vueoff+off);
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

/**
 * Discard a Buffet.
 * aborts if buf is an SSO with views
 * otherwise, buf is zeroed-out, making it an empty SSO.
 * if buf was a view, its target refcount is decremented.
 * if buf was the last view on a store, the store is released.  
 * 
 * @param[in] buf the target Buffet to discard
 */
void
bft_free (Buffet *buf)
{   
    Tag tag = TAG(buf);

    if (tag==SSO && buf->sso.refcnt) {
        
        WARN("bft_free: SSO has views on it\n");
        return;
    
    } else if (tag==OWN) {
            
        Store *store = getstore(buf);
        if (store->canary != CANARY) {
            ERR_CANARY;
        } else if (!store->refcnt) {
            ERR_REFCNT;
        } else {
            -- store->refcnt;
            if (!store->refcnt) {
                store->canary = 0;
                // LOG("free store");
                free(store);
            } 
        }

    } else if (tag==SSV) {

        BuffetSSO *target = (BuffetSSO*)(buf->ptr.data - buf->ptr.off);
        -- target->refcnt; //check?
    }

    // memset(buf, 0, sizeof(Buffet));
    *buf = ZERO;
}


/**
 * Concatenates a Buffet and a byte array into a new Buffet.
 * Returns total length, or zero on allocation failure.
 * 
 * Rem: a Buffet-returning profile "Buffet cat(Buffet, char*)"
 * would miss insecure appending :
 *     Buffet foo = copy("short foo ")
 *     Buffet vue = view(foo)
 *     foo = cat(foo, "now too long for SSO") -> mutates into OWN
 *     print(vue) -> garbage
 *
 * @param[in] buf the Buffet source
 * @param[in] src the byte array source
 * @param[in] srclen the source array length
 * @param[out] dst the destination Buffet
 
todo copy append impl ?
*/
size_t
bft_cat (Buffet *dst, const Buffet *buf, const char *src, size_t srclen)
{
    if (dst==buf) return bft_append((Buffet*)buf, src, srclen);

    Tag tag = TAG(buf);
    char *curdata;
    char *dstdata = NULL;
    Store *store = NULL;
    size_t curlen;
    size_t newlen;
    size_t writeoff = 0;

    if (tag == SSO) {

        curdata = (char*)buf->sso.data;
        curlen = buf->sso.len;
        newlen = curlen + srclen;

        if (newlen <= BUFFET_SSOMAX) {

            Buffet out = *buf;
            memcpy(out.sso.data + curlen, src, srclen);
            out.sso.data[newlen] = 0;
            out.sso.len = newlen;
            out.sso.refcnt = 0;
            *dst = out;
            return newlen;
        }
    
    } else {

        curdata = buf->ptr.data;
        curlen = buf->ptr.len;
        newlen = curlen + srclen;
        writeoff = buf->ptr.off + curlen;

        // todo decide if downsized owner (unique) could convert to SSO
        // if (newlen <= BUFFET_SSOMAX) {}

        if (tag == OWN) {

            store = getstore(buf);
            bool alone = store->refcnt < 2;

            // in-place optimization:
            // if store has room and `buf` is unique owner or at end,
            // we append in place and return a view.
            if ((writeoff+srclen <= store->cap)
                && (alone || writeoff == store->len)) {

                //LOG("cat OWN: inplace");
                dstdata = store->data + writeoff;
                memcpy(dstdata, src, srclen);
                dstdata[srclen] = 0;
                store->len = writeoff+srclen;
                *dst = *buf;
                dst->ptr.len = newlen;
                ++ store->refcnt;
                return newlen;
            }
        }

        // rem: could realloc store if alone (like append())
        // but implies mutable buf arg
        // if (alone) {}
    }

    store = newstore(OVERALLOC*newlen, newlen);
    
    if (!store) {
        *dst = ZERO; //?
        return 0;
    }

    // copy current data
    dstdata = store->data;
    memcpy(dstdata, curdata, curlen);
    dstdata += curlen;

    // append new data
    memcpy(dstdata, src, srclen);
    dstdata[srclen] = 0;

    *dst = (Buffet){
        .ptr.data = store->data,
        .ptr.len = newlen,
        .ptr.off = 0,
        .ptr.tag = OWN
    };

    return newlen;
}


/**
 * Append a byte array to a Buffet.
 * Returns new length, or zero on allocation failure or insecure mutation.
 *
 * @param[in,out] buf the destination Buffet
 * @param[in] src the byte array source
 * @param[in] srclen the source length
 * @return the Buffet new length or zero on error
*/
// todo self-data cases
size_t
bft_append (Buffet *buf, const char *src, size_t srclen)
{
    Tag tag = TAG(buf);
    const char *curdata;
    char *writer = NULL;
    Store *store = NULL;
    size_t curlen;
    size_t newlen;
    size_t writeoff = 0;
    bool ssofit = false;

    if (tag == SSO) {

        curdata = (char*)buf->sso.data;
        curlen = buf->sso.len;
        newlen = curlen + srclen;
        ssofit = (newlen <= BUFFET_SSOMAX);

        if (ssofit) {

            writer = (char*)curdata+curlen;
            memcpy(writer, src, srclen);
            writer[srclen] = 0;
            buf->sso.len = newlen;
            return newlen;
        
        } else if (buf->sso.refcnt) {
            // Relocation would mutate `buf` to OWN
            // while views still point directly into it.
            WARN("Append would invalidate views on SSO\n");
            return 0;
        }
    
    } else {

        curdata = buf->ptr.data;
        curlen = buf->ptr.len;
        newlen = curlen + srclen;
        writeoff = buf->ptr.off + curlen;
        ssofit = (newlen <= BUFFET_SSOMAX);

        if (tag == OWN) {

            store = getstore(buf);
            bool alone = store->refcnt < 2;

            // append in-place: only if store has room
            // and `buf` is unique owner or at end.
            if ((writeoff+srclen <= store->cap)
                && (alone || writeoff == store->len)) {

                //LOG("append OWN: inplace");
                writer = store->data + writeoff;
                memcpy(writer, src, srclen);
                writer[srclen] = 0;
                store->len = writeoff+srclen;   
                buf->ptr.len = newlen;

                return newlen;
            }
            
            // realloc store
            // optim: shift left if off=0 ?
            if (alone) {
                //LOG("append OWN: realloc");
                size_t newcap = writeoff + OVERALLOC*srclen;
                store = realloc(store, STOREMEM(newcap));
                if (!store) {
                    ERR("append realloc\n");
                    return 0;
                }
                writer = store->data + writeoff;
                goto appn;
            }

            // detach
            -- store->refcnt; 
            // rem: without list of views, no way to adjust end*
        
        } else if (tag==SSV) {

            BuffetSSO *target = (BuffetSSO*)(buf->ptr.data - buf->ptr.off);
            bool alone = target->refcnt < 2;
            
            // overdone ?
            // Append in-place: only if sso has room
            // and `buf` is unique view or at end.
            if ((writeoff+srclen <= BUFFET_SSOMAX)
                && (alone || writeoff == target->len)) {

                //LOG("append SSV: inplace");
                writer = target->data + writeoff;
                memcpy(writer, src, srclen);
                writer[srclen] = 0;
                target->len = writeoff+srclen;
                buf->ptr.len = newlen;
                return newlen;
            }

            // detach
            -- target->refcnt;
        }
    } // end case ptr

    if (ssofit) {

        // assert(tag!=SSO);
        // LOG("ssofit");
        TAG(buf) = SSO;
        buf->sso.len = newlen;
        buf->sso.refcnt = 0;

        writer = buf->sso.data;
        memcpy(writer, curdata, curlen);
        writer += curlen;
        memcpy(writer, src, srclen);
        writer[srclen] = 0;

        return newlen;
    }

    store = newstore(OVERALLOC*newlen, newlen);
    if (!store) {return 0;}

    writer = store->data;
    memcpy(writer, curdata, curlen);
    writer += curlen;

    TAG(buf) = OWN;
    buf->ptr.off = 0;
appn:
    memcpy(writer, src, srclen);
    writer[srclen] = 0;
    buf->ptr.data = store->data;
    buf->ptr.len = newlen;

    return newlen;               
}



#define LIST_STACK_MAX (BUFFET_STACK_MEM/sizeof(Buffet))

/**
 * Split a bytes source into a list of Buffets.
 *
 * @param[in] src the bytes source
 * @param[in] srclen the source length in bytes
 * @param[in] sep the separator string
 * @param[in] seplen the separator length in bytes
 * @param[out] outcnt the resulting list length
 * @return the resulting parts Buffet array
*/
Buffet*
bft_split (const char* src, size_t srclen, const char* sep, size_t seplen, 
    int *outcnt)
{
    int curcnt = 0; 
    Buffet *ret = NULL;
    
    Buffet parts_local[LIST_STACK_MAX]; 
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
                memcpy(parts, parts_local, curcnt * sizeof(Buffet));
                local = false;
            } else {
                parts = realloc(parts, newsz); 
                if (!parts) {curcnt = 0; goto fin;}
            }
        }

        parts[curcnt++] = newvue(beg, end-beg);
        beg = end+seplen;
        end = beg;
    };
    
    // last part
    parts[curcnt++] = newvue(beg, src+srclen-beg);

    if (local) {
        size_t outlen = curcnt * sizeof(Buffet);
        ret = malloc(outlen);
        memcpy(ret, parts, outlen);
    } else {
        ret = parts;  
    }

    fin:
    *outcnt = curcnt;
    return ret;
}


/**
 * Split a bytes source into a list of Buffets.
 *
 * @param[in] src the source c-string
 * @param[in] sep the separator c-string
 * @param[out] outcnt the resulting list length
 * @return the resulting parts Buffet array
*/
Buffet*
bft_splitstr (const char* src, const char* sep, int *outcnt) {
    return bft_split(src, strlen(src), sep, strlen(sep), outcnt);
}


/**
 * Join a list of Buffet along a separator into a new Buffet.
 *
 * @param[in] parts the Buffet source array
 * @param[in] cnt the source array length
 * @param[in] sep the separator string
 * @param[in] seplen the separator length in bytes
 * @return the resulting Buffet
*/
Buffet 
bft_join (const Buffet *parts, int cnt, const char* sep, size_t seplen)
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
    
    Buffet ret = bft_new(totlen);
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


/**
 * Get a Buffet data as a null-terminated C string of length buf.len.
 * 
 * @param[in] buf the Buffet source
 * @param[out] mustfree output flag set if the return must be freed
 * @return the c-string data
 */
const char*
bft_cstr (const Buffet *buf, bool *mustfree) 
{
    Tag tag = TAG(buf);
    const size_t len = getlen(buf, tag);
    bool tofree = false;
    char *ret = NULL;

    if (!len) {
        ret = ""; //calloc(1) ?
    } else if (tag==SSO) {
        ret = (char*)buf->sso.data;
    } else {
        const char *data = buf->ptr.data;
        if (data[len]==0) {
            ret = (char*)data;
        } else {
            ret = malloc(len+1);
            if (!ret) {
                ERR_ALLOC;
                ret = calloc(1,1);
            } else {
                memcpy(ret, data, len);
                ret[len] = 0;
            }
            tofree = true;
        }               
    }

    *mustfree = tofree;
    return ret;
}


/**
 * Get a null-terminated copy of a Buffet data.
 * @param[in] buf the Buffet source
 * @return string copy that must be freed, or NULL on error
 */
char*
bft_export (const Buffet* buf) 
{
    const Tag tag = TAG(buf);
    const size_t len = getlen(buf,tag);
    const char* data = getdata(buf,tag);
    char* ret = malloc(len+1);

    if (!ret) {
        ERR_ALLOC; return NULL;
    } else {
        memcpy(ret, data, len);
    }

    ret[len] = 0;
    return ret;
}


/**
 * Get a Buffet read-only data pointer.
 * @param[in] buf the Buffet source
 */
const char*
bft_data (const Buffet* buf) {
    return getdata(buf, TAG(buf));
}

/**
 * Get a Buffet length.
 * @param[in] buf the Buffet source
 */
size_t
bft_len (const Buffet* buf) {
    return getlen(buf, TAG(buf));
}

/**
 * Get a Buffet total capacity.
 * @param[in] buf the Buffet source
 */
size_t
bft_cap (const Buffet* buf) {
    Tag tag = TAG(buf);
    if (tag==SSO) return BUFFET_SSOMAX;
    if (tag==OWN) {
        Store* store = getstore(buf);
        return store->cap;
    }
    return 0;
}

/**
 * Print a Buffet data
 * @param[in] buf the Buffet source
 */
void
bft_print (const Buffet *buf) {
    Tag tag = TAG(buf);
    printf("%.*s\n", (int)getlen(buf,tag), getdata(buf,tag));
    fflush(stdout);
}

/**
 * Print a Buffet state.
 * @param[in] buf the Buffet source
 */
void 
bft_dbg (const Buffet* buf) {
    dbg(buf);
}
