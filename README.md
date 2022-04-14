# Buffet

*All-inclusive Buffer for C*  

[**API**](#API)  
![CI](https://github.com/alcover/buffet/actions/workflows/ci.yml/badge.svg)

![schema](assets/buffet.png)  

Buffet is an experimental polymorphic string buffer with
- **SSO** (small string optimization)
- **views** : no-copy references to slices of data  
- **refcount** : secure release of owned data
- 4GB max length (possibly 16)

It is compact (**16 bytes**), reasonably [**fast**](#Speed) and aims at total [**security**](#Security).  
Coming: thread safety

---

## How

```C
union Buffet {
        
    // OWN|REF|VUE
    struct {
        char*    data
        uint32_t len
        uint32_t aux:30, 2 // aux = cap|off
    } ptr

    // SSO
    struct {
        char     data[15]
        uint8_t  len:6, 2
    } sso
}
```  
The *tag* sets how Buffet is interpreted :
- `SSO` : as in-situ char array
- `OWN` : as owning heap-allocated data (*aux* = capacity)
- `REF` : as slice of owned data (*aux* = offset)
- `VUE` : as slice of other data

Any proper data (*SSO*/*OWN*) is null-terminated.  

![schema](assets/schema.png)


### example

```C
#include "buffet.h"

int main()
{
    char text[] = "The train goes";
    
    Buffet vue;
    buffet_memview (&vue, text+4, 5);
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


### Build & unit-test

`make && make check`

While extensive, tests may not yet cover *all* cases.


### Speed

`$ make && make benchcpp`  
(requires *libbenchmark-dev*)  

NB: No effort has yet been done on optimization.

On my weak Thinkpad :  
```
SPLIT_JOIN_CPPVIEW       3225 ns
SPLIT_JOIN_PLAINC        2973 ns
SPLIT_JOIN_BUFFET        1518 ns *
APPEND_CPP/1               53 us
APPEND_CPP/8              110 us
APPEND_CPP/64             711 us
APPEND_BUFFET/1            98 us *
APPEND_BUFFET/8           111 us *
APPEND_BUFFET/64          153 us *
```


### Security

Buffet aims at preventing any memory fault, including from user.  
(Except of course when a reference target goes out of scope..)  

See *check.c : danger()*.  

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
Buffet alias = buf; // not recommended. Use buffet_clone().
buffet_free(&buf);
buffet_free(&alias); // OK
```

Etc...

For this, heap-stored data is controlled by a header :

```C
struct Store {
    refcnt
    canary
    data[]
}
```

On some operations like *view*, *append* or *free*, we check the Store header for a live canary and coherent refcount.  
If either fails, the operation is aborted and the Buffet struct possibly zeroed. 

---

# API

[buffet_new](#buffet_new)  
[buffet_memcopy](#buffet_memcopy)  
[buffet_memview](#buffet_memview)  
[buffet_copy](#buffet_copy)  
[buffet_view](#buffet_view)  
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
void buffet_new (Buffet *dst, size_t cap)
```
Create Buffet *dst* of minimum capacity *cap*.  

```C
Buffet buf;
buffet_new(&buf, 20);
buffet_debug(&buf); 
// OWN cap:20 len:0 cstr:''
```

### buffet_memcopy
```C
void buffet_memcopy (Buffet *dst, const char *src, size_t len)
```
Copy *len* bytes from *src* into new Buffet *dst*.  

```C
Buffet copy;
buffet_memcopy(&copy, "Bonjour", 3);
buffet_debug(&copy); 
// SSO cap:14 len:3 cstr:'Bon'

```

### buffet_memview
```C
void buffet_memview (Buffet *dst, const char *src, size_t len)
```
View *len* bytes from *src* into new Buffet *dst*.  
You get a window into *src* without copy or allocation.

```C
char src[] = "Eat Buffet!";
Buffet view;
buffet_memview(&view, src+4, 6);
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
Buffet buffet_view (const Buffet *src, ptrdiff_t off, size_t len)
```
View *len* bytes of Buffet *src*, starting at *off*.  
You get a window into *src* without copy or allocation.  

Internally the return is either 
- a *REF* to *src* if *src* is *OWN* or *SSO*
- a *REF* to *src*'s target if *src* is *REF*
- a *VUE* on *src*'s target if *src* is *VUE*

If the return is a *REF*, the targetted data cannot be released before either  
- the return is released
- the return is detached, e.g. when you `append` to it.

```C
// view own
Buffet own;
buffet_memcopy(&own, "Bonjour monsieur buddy", 16);
Buffet Bonjour = buffet_view(&own, 0, 7);
buffet_debug(&Bonjour); // REF cstr:'Bonjour'

// view ref
Buffet Bon = buffet_view(&Bonjour, 0, 3);
buffet_debug(&Bon); // REF cstr:'Bon'

// detach views
buffet_append(&Bonjour, "!", 1);
buffet_free(&Bon); 
buffet_free(&own); // OK

// view vue
Buffet vue;
buffet_memview(&vue, "Good day", 4); // "Good"
Buffet Goo = buffet_view(&vue, 0, 3);
buffet_debug(&Goo); // VUE cstr:'Goo'

// view sso
Buffet sso;
buffet_memcopy(&sso, "Hello", 5);
Buffet Hell = buffet_view(&sso, 0, 4);
buffet_debug(&Hell); // REF cstr:'Hell'
buffet_free(&Hell); // OK
buffet_free(&sso); // OK
```


### buffet_free
```C
bool buffet_free (Buffet *buf)
```
Discards *buf*.  
If *buf* was the last reference to owned data, the data is released.  

Returns *true* and zeroes-out *buf* into an empty *SSO* if all good.  
Returns *false* if not. E.g when *buf* is owning data with live views.

```C
char text[] = "Le grand orchestre";

Buffet own;
buffet_memcopy(&own, text, sizeof(text));
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


### buffet_append
```C
size_t buffet_append (Buffet *dst, const char *src, size_t len)
```
Appends *len* bytes from *src* to *dst*.  
Returns new length or 0 on error.
If over capacity, *dst* gets reallocated. 

```C
Buffet buf;
buffet_memcopy(&buf, "abc", 3); 
size_t newlen = buffet_append(&buf, "def", 3); // newlen == 6 
buffet_debug(&buf);
// SSO cap:14 len:6 cstr:'abcdef'
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