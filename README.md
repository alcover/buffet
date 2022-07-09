# Buffet

*All-inclusive Buffer for C*  

![schema](assets/buffet.png)  

Experimental string buffer featuring

- **SSO** (small string optimization) inline short data inside the type.
- **views** (or 'slices') no-copy references to sub-strings.
- **refcount** (reference counting) account for views before mutation or release.
- automated allocations

Aims at [**security**](#Security) with decent [**speed**](#Speed).  
Todo: thread safety


[**API**](#API)  

![CI](https://github.com/alcover/buffet/actions/workflows/ci.yml/badge.svg)

## Layout

Buffet is a tagged union with 4 modes.  

```C
// Hard values illustrate 64-bit

union Buffet {
    struct ptr {
        char*   data
        size_t  len
        size_t  off:62, tag:2 // tag = OWN|SSV|VUE
    }
    struct sso {
        char    data[22]
        uint8_t refcnt
        uint8_t len:6, tag:2  // tag = SSO
    }
}

sizeof(Buffet) == 24
```
The *tag* sets a Buffet's mode :  

- `OWN` : co-owning slice of a store
- `SSO` : embedded char array
- `SSV` : (small string view) view on an SSO
- `VUE` : non-owning view on any data

In *OWN* mode, *Buffet.data* points into an allocated heap store.

```C
struct Store {
    size_t   cap    // store capacity
    size_t   len    // store active length
    uint32_t refcnt // number of views on the store
    uint32_t canary // invalidates the store if modified
    char     data[] // buffer data, shared by owning views
}
```


#### Schema

![schema](assets/schema.png)

```C
#include "../buffet.h"

int main() {

// SHARED OWN ===================================

    char large[] = "DATA STORE IS HEAP ALLOCATION.";

    // own1 will own a store holding a copy of `large`
    Buffet own1 = bft_memcopy(large, sizeof(large)-1);
    bft_dbg(&own1);
    // View "STORE" in own1
    Buffet own2 = bft_view(&own1, 5, 5);
    // Now own1 and own2 share the store, whose refcount is 2
    bft_dbg(&own2);

// SSO & SSV ===================================

    char small[] = "SMALL STRING";

    Buffet sso1 = bft_memcopy(small, sizeof(small)-1);
    bft_dbg(&sso1);
    // View "STRING" in sso1
    Buffet ssv1 = bft_view(&sso1, 6, 6);
    bft_dbg(&ssv1);

// VUE =========================================

    char any[] = "SOME BYTES";

    // View "BYTES" in `any`
    Buffet vue1 = bft_memview(any+5, 5);
    bft_dbg(&vue1);

    return 0;
}
```

```
$ cc schema.c libbuffet.a -o schema && ./schema
OWN 30 "DATA STORE IS HEAP ALLOCATION."
OWN 5 "STORE"
SSO 12 "SMALL STRING"
SSV 6 "STRING"
VUE 5 "BYTES"
```


### Build & check

`make && make check`

While extensive, unit tests may not yet cover all cases.

### Speed

`make && make bench` (requires *libbenchmark-dev*)  

NB: The lib is not much optimized and the bench maybe amateurish.  
On a weak Core i3 :  
<pre>
MEMVIEW_cpp/8                1 ns          1 ns 1000000000
MEMVIEW_buffet/8             7 ns          7 ns  103101477
MEMCOPY_c/8                 17 ns         17 ns   41771438
MEMCOPY_buffet/8            12 ns         12 ns   59136008
MEMCOPY_c/32                16 ns         16 ns   44280093
MEMCOPY_buffet/32           27 ns         27 ns   25569904
MEMCOPY_c/128               17 ns         17 ns   41047431
MEMCOPY_buffet/128          31 ns         31 ns   22837310
MEMCOPY_c/512               28 ns         28 ns   25499520
MEMCOPY_buffet/512          45 ns         45 ns   15477668
MEMCOPY_c/2048              94 ns         94 ns    7401330
MEMCOPY_buffet/2048        110 ns        110 ns    6405395
MEMCOPY_c/8192             199 ns        199 ns    3509839
MEMCOPY_buffet/8192        284 ns        284 ns    2461398
APPEND_cpp/8/4              12 ns         12 ns   57633324
APPEND_buffet/8/4           22 ns         22 ns   32506683
APPEND_cpp/8/16             32 ns         32 ns   21765849
APPEND_buffet/8/16          30 ns         30 ns   23278437
APPEND_cpp/24/4             48 ns         48 ns   14586050
APPEND_buffet/24/4          31 ns         31 ns   22807349
APPEND_cpp/24/32            47 ns         47 ns   14982967
APPEND_buffet/24/32         29 ns         29 ns   24023279
SPLITJOIN_c               2913 ns       2913 ns     240557
SPLITJOIN_cpp             3128 ns       3128 ns     223070
SPLITJOIN_buffet          1431 ns       1431 ns     487453
</pre>


### Security

Buffet aims at preventing memory faults, including from user.  
(Except of course losing scope and such.)  


```C
// (pseudo code)

// overflow
Buffet buf = bft_new(8)
bft_append(buf, longstr) // Done

// dangling ref
Buffet buf = bft_memcopy(shortstr)
Buffet view = bft_view(buf)
bft_append(buf, longstr) // would mutate buf into an OWN
// warning: "Append would invalidate views on SSO"

// double-free
bft_free(buf)
bft_free(buf) // OK

// use-after-free
bft_free(buf)
bft_append(buf, "foo") // Done. Now buf="foo".

// aliasing
Buffet alias = buf // should instead be `alias = bft_dup(buf)`
bft_free(buf)
bft_free(alias) // OK. Possible warning "Bad canary. Double free ?"

// Etc...

```

To achieve this, on operations like *view*, *append* or *free*, we check the store's canary and refcount.    
If they're wrong, the operation aborts and possibly returns an empty buffet.  

See *src/check.c* unit-tests and warnings output.


# API

[bft_new](#bft_new)  
[bft_memcopy](#bft_memcopy)  
[bft_memview](#bft_memview)  
[bft_copy](#bft_copy)  
[bft_view](#bft_view)  
[bft_dup](#bft_dup)  (don't alias buffets, use this)  
[bft_append](#bft_append)  
[bft_split](#bft_split)  
[bft_splitstr](#bft_splitstr)  
[bft_join](#bft_join)  
[bft_free](#bft_free)  

[bft_cap](#bft_cap)  
[bft_len](#bft_len)  
[bft_data](#bft_data)  
[bft_cstr](#bft_cstr)  
[bft_export](#bft_export)  

[bft_print](#bft_print)  
[bft_dbg](#bft_dbg)  


### bft_new

    Buffet bft_new (size_t cap)

Create a new empty Buffet of minimum capacity *cap*.  

```C
Buffet buf = bft_new(40);
bft_dbg(&buf); 
// OWN 0 ""
```

### bft_memcopy

    Buffet bft_memcopy (const char *src, size_t len)

Create a new Buffet by copying *len* bytes from *src*.  

```C
Buffet copy = bft_memcopy("Bonjour", 3);
// SSO 3 "Bon"
```

### bft_memview

    Buffet bft_memview (const char *src, size_t len)

Create a new Buffet viewing *len* bytes from *src*.  
You get a window into *src* without copy or allocation.  
NB: You shouldn't directly *memview* a Buffet's data. Use *view()*

```C
char src[] = "Eat Buffet!";
Buffet view = bft_memview(src+4, 6);
// VUE 6 "Buffet"
```

### bft_copy

    Buffet bft_copy (const Buffet *src, ptrdiff_t off, size_t len)

Copy *len* bytes of Buffet *src*, starting at *off*.  


### bft_view

    Buffet bft_view (Buffet *src, ptrdiff_t off, size_t len)

View *len* bytes of Buffet *src*, starting at *off*.  
You get a window into *src* without copy or allocation.  

The return internal type depends on *src* type : 

- `view(SSO) -> SSV` (refcounted)
- `view(SSV) -> SSV` on *src*'s target
- `view(OWN) -> OWN` (as refcounted store co-owner)
- `view(VUE) -> VUE` on *src*'s target

If the return is *OWN*, the target store won't be released before either  
- the return is discarded by *bft_free*
- the return is detached by e.g. appending to it.

```C
#include "../buffet.h"

int main() {

    char text[] = "Bonjour monsieur Buddy. Already speaks french!";

    // view sso
    Buffet sso = bft_memcopy(text, 16); // "Bonjour monsieur"
    Buffet ssv = bft_view(&sso, 0, 7);
    bft_dbg(&ssv);

    // view ssv
    Buffet Bon = bft_view(&ssv, 0, 3);
    bft_dbg(&Bon);

    // view own
    Buffet own = bft_memcopy(text, sizeof(text));
    Buffet ownview = bft_view(&own, 0, 7);
    bft_dbg(&ownview);

    // detach view
    bft_append (&ownview, "!", 1);
    // bft_free(&ownview); 
    bft_free(&own); // Done

    // view vue
    Buffet vue = bft_memview(text+8, 8); // "Good"
    Buffet mon = bft_view(&vue, 0, 3);
    bft_dbg(&mon);

    return 0;
}
```

```
$ cc view.c libbuffet.a -o view && ./view
SSV 7 data:"Bonjour"
SSV 3 data:"Bon"
OWN 7 data:"Bonjour"
VUE 3 data:"mon"
```

### bft_dup

    Buffet bft_dup (const Buffet *src)

Create a shallow copy of *src*.  
**Use this intead of aliasing a Buffet.**  

```C
Buffet src = bft_memcopy("Hello", 5);
Buffet cpy = src; // BAD
Buffet cpy = bft_dup(&src); // GOOD
bft_dbg(&cpy);
// SSO 5 "Hello"
```

Rem: aliasing would mostly work but mess up refcounting (without crash if store protections are enabled) :  

```C
Buffet alias = sso; //ok if sso was not viewed
Buffet alias = own; //not refcounted
Buffet alias = vue; //ok
```

### bft_free

    void bft_free (Buffet *buf)

Discards *buf*.  

- aborts if buf is an SSO with views.
- otherwise, buf is zeroed into an empty SSO.
- if buf was a view, its target refcount is decremented.
- if buf was the last view on a store, the store is released.  

Security:
- the zeroing makes double-free harmless.
- the only problematic use-after-free would be of a OWN alias (not recommended), but the store management prevents stale memory access.

```C
char text[] = "Le grand orchestre de Patato Valdez";

Buffet own = bft_memcopy(text, sizeof(text));
Buffet ref = bft_view(&own, 9, 9); // "orchestre"
bft_free(&own); // A bit soon but ok, --refcnt
bft_dbg(&own);  // SSO 0 ""
bft_free(&ref); // Was last co-owner, store is released

Buffet sso = bft_memcopy(text, 8); // "Le grand"
Buffet ref2 = bft_view(&sso, 3, 5); // "grand"
bft_free(&sso); // WARN line:328 bft_free: SSO has views on it
bft_free(&ref2);
bft_free(&sso); // OK now
bft_dbg(&sso);  // SSO 0 ""
```

```
$ valgrind  --leak-check=full ./bin/ex/free
All heap blocks were freed -- no leaks are possible
```

### bft_cat

    size_t bft_cat (Buffet *dst, const Buffet *buf, const char *src, size_t len)

Concatenates *buf* and *len* bytes of *src* into resulting *dst*.  
Returns total length or 0 on error.

```C
Buffet buf = bft_memcopy("abc", 3);
Buffet dst;
size_t totlen = bft_cat(&dst, &buf, "def", 3);
bft_dbg(&dst);
// SSO 6 "abcdef"
```

### bft_append

    size_t bft_append (Buffet *dst, const char *src, size_t len)

Appends *len* bytes from *src* to *dst*.  
Returns new length or 0 on error.

```C
Buffet buf = bft_memcopy("abc", 3); 
size_t newlen = bft_append(&buf, "def", 3);
bft_dbg(&buf);
// SSO 6 "abcdef"
```

NB: returns failure if *buf* has views and would mutate from SSO to OWN to increase capacity, invalidating the views :  

```C
Buffet foo = bft_memcopy("short foo ", 10);
Buffet view = bft_view(&foo, 0, 5);
// would mutate to OWN :
size_t rc = bft_append(&foo, "now too long for SSO");
assert(rc==0); // meaning aborted
```

To prevent this, release views before appending to a small buffet.  

### bft_split

    Buffet* bft_split (const char* src, size_t srclen, const char* sep, size_t seplen, 
    int *outcnt)

Splits *src* along separator *sep* into a Buffet Vue list of length `*outcnt`.  

Being made of views, you can `free(list)` without leak provided no element was made an owner by e.g appending to it.

### bft_splitstr

    Buffet* bft_splitstr (const char *src, const char *sep, int *outcnt);

Convenient *split* using *strlen* internally.

```C
int cnt;
Buffet *parts = bft_splitstr("Split me", " ", &cnt);
for (int i=0; i<cnt; ++i)
    bft_print(&parts[i]);
// VUE 5 "Split"
// VUE 2 "me"
free(parts);
```

### bft_join

    Buffet bft_join (Buffet *list, int cnt, const char* sep, size_t seplen);

Joins *list* on separator *sep* into a new Buffet.  

```C
int cnt;
Buffet *parts = bft_splitstr("Split me", " ", &cnt);
Buffet back = bft_join(parts, cnt, " ", 1);
bft_dbg(&back);
// SSO 8 'Split me'
```

### bft_equal

    bool bft_equal (const Buffet *a, const Buffet *b)

Compare two buffets' data. Lengths are compared first.

### bft_cap  

    size_t bft_cap (Buffet *buf)

Get current capacity.  

### bft_len  

    size_t bft_len (Buffet *buf)`

Get current length.  

### bft_data

    const char* bft_data (const Buffet *buf)`

Get current data pointer.  
To ensure null-termination at `buf.len`, use *bft_cstr*. 

### bft_cstr

    const char* bft_cstr (const Buffet *buf, bool *mustfree)

Get current data as a null-terminated C string of max length `buf.len`.  
If needed (when *buf* is a view), the data is copied into a new C string that must be freed if *mustfree* is set.

### bft_export

    char* bft_export (const Buffet *buf)

 Copies data up to `buf.len` into a new C string that must be freed.

### bft_print

    void bft_print (const Buffet *buf)`

Prints data up to `buf.len`.

### bft_dbg  

    void bft_dbg (Buffet *buf)

Prints *buf* state.  

```C
Buffet buf;
bft_memcopy(&buf, "foo", 3);
bft_dbg(&buf);
// SSO 3 "foo"
```

## TODO

- ! views : decide clearly if r/o + CoW or writable
- store.len :  not updated when tailing view is detached..  
We lose future in-place optimization.  
Maybe record second to last range ?
- store checks : too many ?  
Decide user responsabilty on mis-handling.  
Add #define ENABLE_MEMCHECKS
- SSO auto-release like own ?
- test 32-bit and small RAM

#### API

- write(), sprintf()
- resize()