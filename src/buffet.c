#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "buffet.h"
#include "log.h"

static_assert (sizeof(Buffet)==BFT_SIZE, "Buffet size");

typedef enum {SSO = 0, OWN,	REF, VUE} Type;

#define EXP(v) (1ull << (v))
#define ASDATA(ptr) ((intptr_t)(ptr) >> BFT_TYPE_BITS)
#define DATA(buf) ((char*)((intptr_t)((buf)->data) << BFT_TYPE_BITS))
#define SRC(view) ((Buffet*)(DATA(view)))
#define assert_aligned(p) assert(0 == (intptr_t)(p) % (1ull<<BFT_TYPE_BITS));
#define ZERO ((Buffet){.fill={0}})

//==============================================================================

static unsigned int 
nextlog2 (unsigned long long n) {
	return 8 * sizeof(unsigned long long) - __builtin_clzll(n-1);
}


static char*
getdata (const Buffet *buf) 
{
	switch(buf->type) {
		case SSO:
			return (char*)buf->sso;
		case OWN:
			return DATA(buf);
		case VUE:
			return DATA(buf) + buf->off;
		case REF:
			return DATA(SRC(buf)) + buf->off;
	}
	return NULL;
}


static size_t
getcap (const Buffet *buf, Type t) {
	return 
		t == OWN ? (1ull << buf->cap)
	:	t == SSO ? BFT_SSO_CAP
	: 	0;
}


// optim (!t)*ssolen + (!!t)*len
static size_t
getlen (const Buffet *buf) {
	return (buf->type == SSO) ? buf->ssolen : buf->len;
}


static void 
setlen (Buffet* buf, Type t, size_t len) {
	if (t==SSO) buf->ssolen=len; else buf->len=len;
}


static char*
alloc (Buffet *dst, size_t cap)
{
	const uint8_t caplog = nextlog2(cap+1);
	char *data = malloc(EXP(caplog)); 
	
	assert_aligned(data);
	
	*dst = ZERO;
	*dst = (Buffet){
		.len = 0,
		.cap = caplog,
		.data = ASDATA(data),
		.type = OWN
	};

	return data;
}


static char*
grow_sso (Buffet *buf, size_t cap)
{
	uint8_t caplog = nextlog2(cap+1);
	char *data = malloc(EXP(caplog));
	
	if (!data) return NULL;

	memcpy (data, buf->sso, buf->ssolen + 1);
	*buf = ZERO;
	buf->len = buf->ssolen;
	buf->cap = caplog;
	buf->data = ASDATA(data);
	buf->type = OWN;

	return data;
}

static char*
grow_own (Buffet *buf, size_t cap)
{
	uint8_t caplog = nextlog2(cap+1);
	char *data = realloc(DATA(buf), EXP(caplog));

	if (!data) return NULL;

	buf->data = ASDATA(data);
	buf->cap = caplog;

	return data;
}

//==============================================================================

void
bft_new (Buffet* dst, size_t cap)
{
	*dst = ZERO; //sure init?

	if (cap > BFT_SSO_CAP) {
		char* data = alloc(dst, cap);
		data[0] = 0;
	}
}


void
bft_strcopy (Buffet* dst, const char* src, size_t len)
{
	*dst = ZERO;
	
	if (len <= BFT_SSO_CAP) {

		memcpy(dst->sso, src, len);
		dst->sso[len] = 0; //?
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
bft_slice (const Buffet *src, size_t off, size_t len)
{
	Buffet ret;
	char* data = getdata(src);
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
			const char* data = getdata(src); 
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
bft_append (Buffet *buf, const char *src, size_t srclen) 
{
	const Type type = buf->type;
	const size_t curlen = getlen(buf);
	size_t newlen = curlen + srclen;

	if (type==SSO) {

		if (newlen <= BFT_SSO_CAP) {
			memcpy (buf->sso+curlen, src, srclen);
			buf->sso[newlen] = 0;
			buf->ssolen = newlen;
		} else {
			char *data = grow_sso (buf, newlen);
			memcpy (data+curlen, src, srclen);
			data[newlen] = 0;
			buf->len = newlen;
		}

	} else if (type==OWN) {

		char *data;

		if (newlen <= buf->cap) {
			data = DATA(buf);
			memcpy (data+curlen, src, srclen);
		} else {
			data = grow_own (buf, newlen);
			// if (!data) {ERR("grow fail"); return 0;}
			memcpy (data+curlen, src, srclen);
		}

		data[newlen] = 0;
		buf->len = newlen;

	} else {

		const char *curdata = getdata(buf);
		Buffet *ref = SRC(buf);

		bft_strcopy (buf, curdata, curlen); 
		if (type == REF) --ref->refcnt;
		bft_append (buf, src, srclen);
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
	// const Type type = buf->type;
	char* data = getdata(buf);
	const size_t len = getlen(buf);
	char* ret = malloc(len+1);
	
	memcpy (ret, data, len);
	ret[len] = 0;

	return ret;
}

const char*
bft_data (const Buffet* buf) {
	return getdata(buf);
}

size_t
bft_cap (const Buffet* buf) {
	return getcap(buf, buf->type);
}

size_t
bft_len (const Buffet* buf) {
	return getlen(buf);
}

void 
bft_dbg (Buffet* buf) {
	printf ("type %d cap %zu len %zu data '%s'\n", 
		buf->type, bft_cap(buf), bft_len(buf), bft_data(buf));
}

void
bft_print (const Buffet *buf) {
	const char *data = getdata(buf);
	int len = getlen(buf);
	printf("%.*s\n", len, data);
}