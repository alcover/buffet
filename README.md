# Buffet

*All-inclusive Buffer for C*  

[**API**](#API)  
![CI](https://github.com/alcover/buffet/actions/workflows/ci.yml/badge.svg)

![schema](assets/buffet.png)  

*Buffet* is a polymorphic string buffer type featuring
- automated allocations
- **SSO** (small string optimization)
- **views** : no-cost references to slices of data  
- **refcount** : secure release of owned data
- **cstr** : obtain a null-terminated C string

In mere register-fitting **16 bytes**.  

:construction:  
\- not yet stable, optimized or feature-complete  
\- not yet 100% unit-tested  
\- not yet thread-safe  

---

## How

```C
union Buffet {
        
    // tag = {OWN|REF|VUE}
    struct {
        char*    data
        uint32_t len
        uint32_t aux:30, tag:2 // aux = {cap|off}
    } ptr

    // tag = SSO
    struct {
        char     data[15]
        uint8_t  len:6, tag:2
    } sso
}
```  
The *tag* field sets how a Buffet is interpreted :
- `SSO` : as a char array
- `OWN` : as owning heap-allocated data (with *aux* as capacity)
- `REF` : as a slice of owned data (with *aux* as offset)
- `VUE` : as a slice of other data

Any *proper* data (*SSO*/*OWN*) is null-terminated.  

![schema](assets/schema.png)



### Build & unit-test

`make && make check`

### example

```C
#include <stdio.h>
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

### Speed
Not optimized yet but a basic (maybe unfair) benchmark is available  
`make && make bin/benchcpp`  
`make benchcpp`

On my weak thinkpad :  
```
SPLIT_JOIN_CPPVIEW       5427 ns
SPLIT_JOIN_PLAINC        4860 ns
SPLIT_JOIN_BUFFET        2285 ns
APPEND_CPP/1              223 us
APPEND_CPP/8              331 us
APPEND_CPP/64            1291 us
APPEND_BUFFET/1           134 us
APPEND_BUFFET/8           147 us
APPEND_BUFFET/64          194 us
```


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
// tag:OWN cap:20 len:0 cstr:''
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
// tag:SSO cap:14 len:3 cstr:'Bon'

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
// tag:VUE cap:0 len:6 cstr:'Buffet'
```

### buffet_copy
```C
Buffet buffet_copy (const Buffet *src, ptrdiff_t off, size_t len)
```
Copy *len* bytes of Buffet *src*, starting at offset *off*.  


### buffet_view
```C
Buffet buffet_view (const Buffet *src, ptrdiff_t off, size_t len)
```
View *len* bytes of Buffet *src*, starting at offset *off*.  
You get a window into *src* without copy or allocation.  

Internally the return is either 
- a *REF* to *src* if *src* is *OWN*
- a *REF* to *src*'s target if *src* is *REF*
- a *VUE* on *src*'s data if *src* is *SSO* or *VUE*

If the return is a *REF*, the targetted data cannot be released before either  
- the return is released
- the return is detached, e.g. when you `append` to it.

```C
// view own
Buffet own;
buffet_memcopy(&own, "Bonjour monsieur", 16);
Buffet ref1 = buffet_view(&own, 0, 7);
buffet_debug(&ref1); // tag:REF cstr:'Bonjour'

// view ref
Buffet ref2 = buffet_view(&ref1, 0, 3);
buffet_debug(&ref2); // tag:REF cstr:'Bon'

// detach views
buffet_append(&ref1, "!", 1);   // "Bonjour!"
buffet_append(&ref2, "net", 3); // "Bonnet"
buffet_free(&own); // OK

// view vue
Buffet vue;
buffet_memview(&vue, "Good day", 4); // "Good"
Buffet vue2 = buffet_view(&vue, 0, 3);
buffet_debug(&vue2); // tag:VUE cstr:'Goo'

// view sso
Buffet sso;
buffet_memcopy(&sso, "Bonjour", 7);
Buffet vue3 = buffet_view(&sso, 0, 3);
buffet_debug(&vue3); // tag:VUE cstr:'Bon'
```


### buffet_free
```C
bool buffet_free (Buffet *buf)
```
Discards *buf*.  
If *buf* was the last reference to owned data, the data is released.  
In any case, *buf* is zeroed, making it an empty *SSO*.  

```C
char text[] = "Le grand orchestre de Patato Valdez";

Buffet own;
buffet_memcopy(&own, text, sizeof(text));
buffet_debug(&own);
// tag:OWN cstr:'Le grand orchestre de Patato Valdez'

Buffet ref = buffet_view(&own, 22, 13);
buffet_debug(&ref);
// tag:REF cstr:'Patato Valdez'

// Too soon but data marked for release
buffet_free(&own);
buffet_debug(&own);
// tag:SSO cstr:''

// Release last ref, so data gets released
buffet_free(&ref);
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
// tag:SSO cap:14 len:6 cstr:'abcdef'
```


### buffet_split
```C
Buffet* buffet_split (const char* src, size_t srclen, const char* sep, size_t seplen, 
    int *outcnt)
```
Splits *src* along separator *sep* into a Buffet list of length `*outcnt`.  
The list can be freed using `buffet_list_free(list, outcnt)`

```C
int cnt;
Buffet *parts = buffet_split("Split me", 8, " ", 1, &cnt);
for (int i=0; i<cnt; ++i)
    buffet_debug(&parts[i]);
buffet_list_free(parts, cnt);
// tag:VUE cap:0 len:5 cstr:'Split'
// tag:VUE cap:0 len:2 cstr:'me'
```

### buffet_splitstr
```C
Buffet* buffet_splitstr (const char *src, const char *sep, int *outcnt);
```
Convenient *split* using *strlen* internally.


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
// tag:SSO cap:14 len:8 cstr:'Split me'
```


### buffet_cap  
Get current capacity.  
```C
size_t buffet_cap (Buffet *buf)
```

### buffet_len  
Get current length.  
```C
size_t buffet_len (Buffet *buf)`
```

### buffet_data
Get current data pointer.  
```C
const char* buffet_data (const Buffet *buf)`
```
If *buf* is a ref/view, `strlen(buffet_data)` may be longer than `buf.len`. 

### buffet_cstr
Get current data as a NUL-terminated **C string**.  
```C
const char* buffet_cstr (const Buffet *buf, bool *mustfree)
```
If *REF*/*VUE*, the slice is copied into a fresh C string that must be freed.

### buffet_export
 Copies data up to `buf.len` into a fresh C string that must be freed.
```C
char* buffet_export (const Buffet *buf)
```



### buffet_print
Prints data up to `buf.len`.
```C
void buffet_print (const Buffet *buf)`
```

### buffet_debug  
Prints *buf* state.  
```C
void buffet_debug (Buffet *buf)
```

```C
Buffet buf;
buffet_memcopy(&buf, "foo", 3);
buffet_debug(&buf);
// tag:SSO cap:14 len:3 cstr:'foo'
```