# Buffet

*All-inclusive Buffer for C*  

[**API**](#API)  
![CI](https://github.com/alcover/buffet/actions/workflows/ci.yml/badge.svg)

![schema](assets/buffet.png)  

Buffet is an experimental polymorphic string buffer with
- **SSO** : small string optimization
- **views** : no-copy slices  
- **refcount** : secure release of referenced data

aiming at [**security**](#Security) with decent [**speed**](#Speed).  
Coming: thread safety

---

## How

```C
union Buffet {
        
    // OWN / VUE
    struct {
        char*  data
        size_t len
        size_t off:62 tag:2
    } ptr

    // SSO
    struct {
        char    data[22]
        uint8_t refcnt
        uint8_t len:6 tag:2
    } sso
}
```  
The *tag* sets how a Buffet is interpreted :
- `SSO` : in-situ char array
- `OWN` : sharing heap-allocated data
- `VUE` : view on SSO or any data

#### Schema

![schema](assets/schema.png)

Say a long text is transient and must be copied.

```C
char longtext[] = "DATA STORE IS HEAP ALLOCATION";
Buffet own1 = buffet_memcopy(longtext, sizeof(longtext));
```
Since *longtext* is too large for an SSO, a heap Store is allocated to house it.  
*Own1* owns the store.

Say we want to *look* at the word "STORE" in *own1*.  
No need to copy again, suffice to take a view.

```C
Buffet own2 = buffet_view(own1, 5, 5);
```
Now *own2* shares the store with *own1*.  
The store's reference count bumps to 2.  
Only when it gets back to 0 can the store be released.


Say we do the same as above but on a short input.

```C
char shorttext[] = "BUFFET";
Buffet sso = buffet_memcopy(shorttext, 6);
```

We get an SSO Buffet embedding the text. No allocation.  

Now taking a view of *sso* produces another sub-type : VUE.  
This special case of VUE targets an SSO. This is indicated by the non-zero
offset field : 4-1 = 3 bytes into the SSO.

```C
Buffet vue1 = buffet_memview(sso, 3, 3);
```
*sso*, like a store, tracks views in a refcount.

Say we want to view data from a source that stays in scope.  
Here *memview* produces a plain VUE with a 0 offset meaning the source
is not controlled by Buffet.

```C
char somebytes[] = "SOME BYTES";
Buffet vue2 = buffet_memview(somebytes, 5);
```

### example

```C
#include "buffet.h"

int main()
{
    char text[] = "The train goes";
    
    Buffet vue = buffet_memview (text+4, 5);
    buffet_print(&vue); // "train"

    text[4] = 'b';
    buffet_print(&vue); // "brain"

    Buffet ref = buffet_view (&vue, 1, 4);
    buffet_print(&ref); // "rain"

    char tail[] = "ing";
    buffet_append (&ref, tail, sizeof(tail));
    buffet_print(&ref); // "raining"

    return 0;
}
```

```
$ gcc example.c buffet -o example && ./example
train
brain
rain
raining
```


### Build & test

`make && make check`

While extensive, unit tests may not yet cover *all* cases.


### Speed

`$ make && make benchcpp`  
(requires *libbenchmark-dev*)  

NB: No effort has been done on optimization, plus bench may be unfair.  

On a weak Thinkpad :  
```
MEMVIEW_cppview/8            1 ns
MEMVIEW_buffet/8             6 ns
MEMCOPY_plainc/8            17 ns
MEMCOPY_buffet/8            12 ns
MEMCOPY_plainc/32           16 ns
MEMCOPY_buffet/32           27 ns
MEMCOPY_plainc/128          17 ns
MEMCOPY_buffet/128          30 ns
MEMCOPY_plainc/512          26 ns
MEMCOPY_buffet/512          42 ns
MEMCOPY_plainc/2048         95 ns
MEMCOPY_buffet/2048        104 ns
MEMCOPY_plainc/8192        199 ns
MEMCOPY_buffet/8192        285 ns
APPEND_cppstr/8/4           13 ns
APPEND_buffet/8/4           33 ns
APPEND_cppstr/8/16          34 ns
APPEND_buffet/8/16          34 ns
APPEND_cppstr/24/4          49 ns
APPEND_buffet/24/4          34 ns
APPEND_cppstr/24/32         48 ns
APPEND_buffet/24/32         33 ns
SPLITJOIN_plainc          2864 ns
SPLITJOIN_cppview         3087 ns
SPLITJOIN_buffet          1380 ns
```


### Security

Buffet aims at preventing memory faults, including from user.  
(Except of course losing scope and such..)  

- double-free

```C
Buffet buf;
// (...)
buffet_free(&buf);
buffet_free(&buf); // OK
```
- use-after-free

```C
Buffet buf;
// (...)
buffet_free(&buf);
buffet_append(&buf, foo); // OK
```

- aliasing

```C
Buffet buf;
// (...)
Buffet alias = buf; // not recommended. Use buffet_dup().
buffet_free(&buf);
buffet_free(&alias); // OK
```

Etc...

For this, heap-stored data is controlled by a header :

```C
struct Store {
    cap
    end
    refcnt
    canary
    data[]
}
```

On some operations like *view*, *append* or *free*, we check the Store for a live canary and coherent refcount.  
If either fails, operation is aborted and the return possibly a zero-buffet. 

---

# API

[buffet_new](#buffet_new)  
[buffet_memcopy](#buffet_memcopy)  
[buffet_memview](#buffet_memview)  
[buffet_copy](#buffet_copy)  
[buffet_view](#buffet_view)  
[buffet_dup](#buffet_dup)  
[buffet_append](#buffet_append)  
[buffet_split](#buffet_split)  
[buffet_splitstr](#buffet_splitstr)  
[buffet_join](#buffet_join)  
[buffet_free](#buffet_free)  

[buffet_cap](#buffet_cap)  
[buffet_len](#buffet_len)  
[buffet_data](#buffet_data)  
[buffet_cstr](#buffet_cstr)  
[buffet_export](#buffet_export)  

[buffet_print](#buffet_print)  
[buffet_debug](#buffet_debug)  



### buffet_new
```C
Buffet buffet_new (size_t cap)
```
Create a new Buffet of minimum capacity *cap*.  

```C
Buffet buf = buffet_new(40);
buffet_debug(&buf); 
// OWN cap:40 len:0 cstr:''
```

### buffet_memcopy
```C
Buffet buffet_memcopy (const char *src, size_t len)
```
Create a new Buffet by copying *len* bytes from *src*.  

```C
Buffet copy = buffet_memcopy("Bonjour", 3);
buffet_debug(&copy); 
// SSO cap:14 len:3 cstr:'Bon'

```

### buffet_memview
```C
Buffet buffet_memview (const char *src, size_t len)
```
Create a new Buffet viewing *len* bytes from *src*.  
You get a window into *src* without copy or allocation.  
NB: You shouldn't directly *memview* a Buffet's *data*. Use *view()*

```C
char src[] = "Eat Buffet!";
Buffet view = buffet_memview(src+4, 6);
buffet_debug(&view);
// VUE cap:0 len:6 cstr:'Buffet'
```

### buffet_copy
```C
Buffet buffet_copy (const Buffet *src, ptrdiff_t off, size_t len)
```
Copy *len* bytes of Buffet *src*, starting at *off*.  


### buffet_view
```C
Buffet buffet_view (Buffet *src, ptrdiff_t off, size_t len)
```
View *len* bytes of Buffet *src*, starting at *off*.  
You get a window into *src* without copy or allocation.  

The return's internal type depends on *src* type : 
- *OWN* (as store co-owner) if *OWN*
- refcounted *VUE* if *SSO*
- plain *VUE* on *src*'s target if plain *VUE*
- refcounted *VUE* on *src*'s target if *SSO-VUE*

If the return is a *OWN*, the targetted won't be released before either  
- the return is released
- the return is detached, e.g. when you `append` to it.

```C
// view own
Buffet own = buffet_memcopy("Bonjour monsieur buddy", 16);
Buffet Bonjour = buffet_view(&own, 0, 7);
buffet_debug(&Bonjour); // OWN cstr:'Bonjour'

// sub-view
Buffet Bon = buffet_view(&Bonjour, 0, 3);
buffet_debug(&Bon); // OWN cstr:'Bon'

// detach views
buffet_append(&Bonjour, "!", 1); // "Bonjour!"
buffet_free(&Bon); 
buffet_free(&own); // OK

// view vue
Buffet vue = buffet_memview("Good day", 4); // "Good"
Buffet Goo = buffet_view(&vue, 0, 3);
buffet_debug(&Goo); // VUE cstr:'Goo'

// view sso
Buffet sso = buffet_memcopy("Hello", 5);
Buffet Hell = buffet_view(&sso, 0, 4);
buffet_debug(&Hell); // VUE cstr:'Hell'
buffet_free(&Hell); // OK
buffet_free(&sso); // OK
```


### buffet_dup
```C
Buffet buffet_dup (const Buffet *src)
```
Duplicates *src*.  
Use this intead of assigning a Buffet to another.  

```C
Buffet src = buffet_memcopy("Hello", 5);
Buffet cpy = src; // BAD
Buffet cpy = buffet_dup(&src); // GOOD
buffet_debug(&cpy);
// SSO cap:14 len:5 cstr:'Hello'
```

Rem: assigning would mostly work but mess up refcounting (without crash if Store protections are enabled) :  
```C
Buffet alias = sso; //ok
Buffet alias = vue; //ok
Buffet alias = own; //not refcounted
```


### buffet_free
```C
bool buffet_free (Buffet *buf)
```
Discards *buf*.  
If *buf* was the last reference to owned data, the data is released.  

Returns *true* and zeroes-out *buf* into an empty *SSO* if all good.  
Returns *true* also on double-free or aliasing.  
Returns *false* otherwise. E.g when *buf* is owning data with live views.

```C
char text[] = "Le grand orchestre";

Buffet own = buffet_memcopy(text, sizeof(text));
Buffet ref = buffet_view(&own, 9, 9); // 'orchestre'

// Too soon but marked for release
buffet_free(&own);

// Was last ref, data gets actually released
buffet_free(&ref);
```

```
$ valgrind  --leak-check=full ./app
All heap blocks were freed -- no leaks are possible
```



### buffet_cat
```C
size_t buffet_cat (Buffet *dst, const Buffet *buf, const char *src, size_t len)
```
Concatenates *buf* and *len* bytes of *src* into resulting *dst*.  
Returns total length or 0 on error.

```C
Buffet buf = buffet_memcopy("abc", 3);
Buffet dst;
size_t totlen = buffet_cat(&dst, &buf, "def", 3);
buffet_debug(&dst);
// SSO len:6 cstr:'abcdef'
```

NB: in-place **appending** by *buffet_cat(buf, buf, ...)* may abort and return failure if *buf* is an SSO with live views and is too small for the new data.  
Because *buf* would need to mutate into an OWN, invalidating the views.  
To avoid this, you should release views prior to appending.  

This is why *buffet_append(dst)* is an alias for *buffet_cat(dst, dst)*.  
An independant *append* with the following profile :  
```Buffet append(Buffet *dst, char *src)```  
would not prevent the user from 

```C
dst = append(dst, foo);
```


### buffet_append
```C
size_t buffet_append (Buffet *dst, const char *src, size_t len)
```
Alias for *buffet_cat(dst, dst, src, len)*.  
Appends *len* bytes from *src* to *dst*.  
Returns new length or 0 on error.

```C
Buffet buf = buffet_memcopy("abc", 3); 
size_t newlen = buffet_append(&buf, "def", 3);
buffet_debug(&buf);
// SSO len:6 cstr:'abcdef'
```


### buffet_split
```C
Buffet* buffet_split (const char* src, size_t srclen, const char* sep, size_t seplen, 
    int *outcnt)
```
Splits *src* along separator *sep* into a Buffet Vue list of length `*outcnt`.  

Being made of views, you can `free(list)` without leak provided no element was made an owner by e.g appending to it.

### buffet_splitstr
```C
Buffet* buffet_splitstr (const char *src, const char *sep, int *outcnt);
```
Convenient *split* using *strlen* internally.

```C
int cnt;
Buffet *parts = buffet_splitstr("Split me", " ", &cnt);
for (int i=0; i<cnt; ++i)
    buffet_print(&parts[i]);
// VUE len:5 cstr:'Split'
// VUE len:2 cstr:'me'
free(parts);
```


### buffet_join
```C
Buffet buffet_join (Buffet *list, int cnt, const char* sep, size_t seplen);
```
Joins *list* on separator *sep* into a new Buffet.  

```C
int cnt;
Buffet *parts = buffet_splitstr("Split me", " ", &cnt);
Buffet back = buffet_join(parts, cnt, " ", 1);
buffet_debug(&back);
// SSO cap:14 len:8 cstr:'Split me'
```


### buffet_cap  
```C
size_t buffet_cap (Buffet *buf)
```
Get current capacity.  

### buffet_len  
```C
size_t buffet_len (Buffet *buf)`
```
Get current length.  

### buffet_data
```C
const char* buffet_data (const Buffet *buf)`
```
Get current data pointer.  
To ensure null-termination at `buf.len`, use *buffet_cstr*. 

### buffet_cstr
```C
const char* buffet_cstr (const Buffet *buf, bool *mustfree)
```
Get current data as a NUL-terminated C string of length `buf.len`.  
If needed (when *buf* is a view), the slice is copied into a fresh C string that must be freed if *mustfree* is set.

### buffet_export
```C
char* buffet_export (const Buffet *buf)
```
 Copies data up to `buf.len` into a fresh C string that must be freed.

### buffet_print
```C
void buffet_print (const Buffet *buf)`
```
Prints data up to `buf.len`.

### buffet_debug  
```C
void buffet_debug (Buffet *buf)
```
Prints *buf* state as viewed by the API.  

```C
Buffet buf;
buffet_memcopy(&buf, "foo", 3);
buffet_debug(&buf);
// SSO cap:14 len:3 cstr:'foo'
```