#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "buffet.h"
#include "log.h"

static_assert (sizeof(Buffet)==BFT_SIZE, "Buffet size");

typedef enum {
	SSO = 0,
	OWN,
	REF,
	VUE
} Type;


#define ASDATA(ptr) ((intptr_t)(ptr) >> BFT_TYPE_BITS)
#define DATA(buf) ((intptr_t)((buf)->data) << BFT_TYPE_BITS)
#define SRC(view) ((Buffet*)(DATA(view)))
#define assert_aligned(p) assert(0 == (intptr_t)(p) % (1ull<<BFT_TYPE_BITS));
#define ZERO ((Buffet){.fill={0}})

//==============================================================================

static unsigned int 
nextlog2 (unsigned long long n) {
	return 8 * sizeof(unsigned long long) - __builtin_clzll(n-1);
}


static char*
getdata (const Buffet *b, const Type t) 
{
	switch(t) {
		case SSO:
			return (char*)b->sso;
		case OWN:
			return (char*)DATA(b);
		case VUE:
			return (char*)(DATA(b) + b->off);
		case REF: {
			Buffet* ref = SRC(b);
			return (char*)DATA(ref) + b->off;
		}
	}
	return NULL;
}


static size_t
getcap (const Buffet *b, Type t) {
	return 
		t == OWN ? (1ull << b->cap)
	:	t == SSO ? BFT_SSO_CAP
	: 	0;
}


// optim (!t)*ssolen + (!!t)*len
static size_t
getlen (const Buffet *b, Type t) {
	return (t == SSO) ? b->ssolen : b->len;
}


static void 
setlen (Buffet* b, Type t, size_t len) {
	if (t==SSO) b->ssolen=len; else b->len=len;
}


static char*
alloc (Buffet* dst, size_t cap)
{
	const uint8_t caplog = nextlog2(cap+1);
	const size_t mem = 1ull << caplog;
	char* data = malloc(mem); 
	
	assert_aligned(data);
	// data[0] = 0;
	// data[mem-1] = 0;
	
	*dst = (Buffet){
		.data = ASDATA(data),
		.cap = caplog,
		// .len = 0,
		.type = OWN
	};

	return data;
}

//==============================================================================

void
bft_new (Buffet* dst, size_t cap)
{
	*dst = ZERO;

	if (cap > BFT_SSO_CAP) {
		char* data = alloc(dst, cap);
		data[0] = 0;
	}
}


void
bft_strcopy (Buffet* dst, const char* src, size_t len)
{
	if (len <= BFT_SSO_CAP) {

		*dst = ZERO;
		memcpy(dst->sso, src, len);
		dst->ssolen = len;
	
	} else {
	
		char* data = alloc(dst, len);
		memcpy(data, src, len);
		data[len] = 0;
		dst->len = len;
	}
}


void
bft_strview (Buffet* dst, const char* src, size_t len)
{
	// save remainder before src address is shifted.
	dst->off = (intptr_t)src % (1 << BFT_TYPE_BITS);
	dst->data = ASDATA(src);
	dst->len = len;
	dst->type = VUE;
}


Buffet
bft_slice (Buffet *src, size_t off, size_t len)
{
	Buffet ret;
	const char* data = getdata(src, src->type);
	bft_strcopy (&ret, data+off, len);
	return ret;
}


Buffet
bft_view (Buffet *src, size_t off, size_t len)
{
	const Type type = src->type;
	Buffet ret = ZERO;
	
	switch(type) {
		
		case SSO:
		case VUE: {
			char* data = getdata(src, type); 
			bft_strview (&ret, data + off, len); }
			break;
		
		case OWN:
			assert_aligned(src);
			ret.data = ASDATA(src);
			ret.off = off;
			ret.len = len;
			ret.type = REF;		
			src->refcnt += 1;
			break;
		
		case REF: {
			Buffet* ref = SRC(src);
			ret = bft_view (ref, src->off + off, len); }
			break;
	}

	return ret;
}


void
bft_free (Buffet* buf)
{
	const Type type = buf->type;

	if (type == OWN && !buf->refcnt) {

		free((char*)DATA(buf));
	
	} else if (type == REF) {
	
		Buffet* ref = SRC(buf);
		--ref->refcnt;
	}

	*buf = ZERO;
}


size_t 
bft_append (Buffet *buf, const char* src, size_t srclen) 
{
	const Type type = buf->type;
	char* data = getdata(buf, type); //LOGVS(data);
	const size_t len = getlen(buf, type); //LOGVI(len);
	const size_t cap = getcap(buf, type);
	const size_t newlen = len + srclen;
	char* newdata = NULL;
	uint8_t newcaplog = 0;

	if (newlen >= cap) {  

		newcaplog = nextlog2(newlen+1);
		const size_t newcap = 1ull << newcaplog;

		if (type == OWN) {
			newdata = realloc(data, newcap);
		} else {
			newdata = malloc(newcap);
			memcpy (newdata, data, len);
			
			if (type == REF) {
				Buffet* ref = SRC(buf);
				--ref->refcnt;
			}

			buf->type = OWN; // or SSO ?
			buf->refcnt = 0;
		}

		assert_aligned(newdata);
		buf->data = ASDATA(newdata);
			
	} else {
		newdata = data;
	}
	// assert(newdata==(char*)buf);
	memcpy (newdata+len, src, srclen);
	newdata[newlen] = 0; 

	setlen(buf, buf->type, newlen);

	if (newcaplog) {
		buf->cap = newcaplog;
	}
	
	return newlen;
}


const char*
bft_cstr (const Buffet* buf) 
{
	const Type type = buf->type;

	switch(type) {
		
		case SSO:
			return (const char*)buf->sso;
		
		case OWN:
			return (const char*)DATA(buf);
		
		case REF: {
			const size_t len = buf->len;
			Buffet *ref = SRC(buf);
			char* cpy = malloc(len+1);
			memcpy(cpy, (char*)DATA(ref)+buf->off, len);
			cpy[len] = 0;
			return (const char*)cpy; }
		
		case VUE:
			return NULL;
	}

	return NULL;
}


char*
bft_export (const Buffet* buf) 
{
	const Type type = buf->type;
	char* data = getdata(buf, type);
	const size_t len = getlen(buf, type); //LOGVI(len); LOGVS(data);
	char* ret = malloc(len+1);
	
	memcpy (ret, data, len);
	ret[len] = 0;

	return ret;
}

char*
bft_data(const Buffet* buf) {
	return getdata(buf, buf->type);
}

size_t
bft_cap(const Buffet* buf) {
	return getcap(buf, buf->type);
}

size_t
bft_len(const Buffet* buf) {
	return getlen(buf, buf->type);
}

void bft_dbg(Buffet* b)
{
	printf ("type %d cap %zu len %zu data '%s'\n", 
		b->type, bft_cap(b), bft_len(b), bft_data(b));
}