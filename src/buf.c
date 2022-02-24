#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "buf.h"
#include "util.c"
#include "log.h"

static_assert (sizeof(Buf)==BUF_SIZE, "Buf size");

typedef enum {
	SSO = 0,
	PTR,
	REF,
	STRVIEW
} Type;


#define ASDATA(data) ((intptr_t)(data) >> TYPE_BITS)
#define DATA(buf) ((intptr_t)((buf)->data) << TYPE_BITS)
#define SRC(view) ((Buf*)(DATA(view)))
#define assert_aligned(p) assert(0 == (intptr_t)(p) % (1ull<<TYPE_BITS));

static const Buf zero = (Buf){.fill={0}};

//==============================================================================

static char*
getdata (const Buf *b, const Type t) 
{
	switch(t) {
		case SSO:
			return (char*)b->sso;
		case PTR:
			return (char*)DATA(b);
		case STRVIEW:
			return (char*)(DATA(b) + b->off);
		case REF: {
			Buf* ref = SRC(b);
			return (char*)DATA(ref) + b->off;
		}
	}
	return NULL;
}


static size_t
getcap (const Buf *b, Type t) {
	return 
		t == PTR ? (1ull << b->cap)
	:	t == SSO ? SSO_CAP
	: 	0;
}


// optim (!t)*ssolen + (!!t)*len
static size_t
getlen (const Buf *b, Type t) {
	return (t == SSO) ? b->ssolen : b->len;
}


static void 
setlen (Buf* b, Type t, size_t len) {
	if (t==SSO) b->ssolen=len; else b->len=len;
}


static char*
reserve (Buf* dst, size_t cap)
{
	const uint8_t caplog = nextlog2(cap+1);
	const size_t mem = 1ull << caplog;
	char* data = malloc(mem); 
	
	data[0] = 0;
	data[mem-1] = 0;
	
	assert_aligned(data);
	*dst = (Buf){
		.data = ASDATA(data),
		.cap = caplog,
		.len = 0,
		.type = PTR
	};

	return data;
}

//==============================================================================

void
buf_new (Buf* dst, size_t cap)
{
	if (cap <= SSO_CAP) {
		*dst = zero; 
	} else {
		reserve(dst, cap);
	}
}


void
buf_copy_str (Buf* dst, const char* src, size_t srclen)
{
	if (srclen <= SSO_CAP) {

		*dst = zero;
		memcpy(dst->sso, src, srclen);
		dst->ssolen = srclen;
	
	} else {
	
		char* data = reserve(dst, srclen);
		memcpy(data, src, srclen);
		data[srclen] = 0;
		dst->len = srclen;
	}
}


void
buf_view_str (Buf* dst, const char* src, size_t len)
{
	// in case src not aligned to TYPE_BITS,
	// save remainder to be re-added when accessing view.data
	uint8_t off = (intptr_t)src % (1 << TYPE_BITS);

	*dst = zero;
	dst->data = ASDATA(src);
	dst->off = off;
	dst->len = len;
	dst->type = STRVIEW;
}


// COPY
Buf
buf_slice (Buf *src, size_t off, size_t len)
{
	Buf ret;
	const char* data = getdata(src, src->type);
	buf_copy_str (&ret, data+off, len);
	return ret;
}


Buf
buf_view (Buf *src, size_t off, size_t len)
{
	const Type type = src->type;
	Buf ret = zero;
	
	switch(type) {
		
		case SSO:
		case STRVIEW: {
			char* data = getdata(src, type); 
			buf_view_str (&ret, data + off, len); }
			break;
		
		case PTR:
			assert_aligned(src);
			ret.data = ASDATA(src);
			ret.off = off;
			ret.len = len;
			ret.type = REF;		
			src->refcnt += 1;
			break;
		
		case REF: {
			Buf* ref = SRC(src);
			ret = buf_view (ref, src->off + off, len); }
			break;
	}

	return ret;
}


void
buf_free (Buf* buf)
{
	const Type type = buf->type;

	if (type == PTR && !buf->refcnt) {

		free((char*)DATA(buf));
	
	} else if (type == REF) {
	
		Buf* ref = SRC(buf);
		--ref->refcnt;
	}

	*buf = zero;
}


size_t 
buf_append (Buf *buf, const char* src, size_t srclen) 
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

		if (type == PTR) {
			newdata = realloc(data, newcap);
		} else {
			newdata = malloc(newcap);
			memcpy (newdata, data, len);
			
			if (type == REF) {
				Buf* ref = SRC(buf);
				--ref->refcnt;
			}

			buf->type = PTR;
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
buf_cstr (const Buf* buf) 
{
	const Type type = buf->type;

	switch(type) {
		
		case SSO:
			return (const char*)buf->sso;
		
		case PTR:
			return (const char*)DATA(buf);
		
		case REF: {
			const size_t len = buf->len;
			Buf *ref = SRC(buf);
			char* cpy = malloc(len+1);
			memcpy(cpy, (char*)DATA(ref)+buf->off, len);
			cpy[len] = 0;
			return (const char*)cpy; }
		
		case STRVIEW:
			return NULL;
	}

	return NULL;
}


char*
buf_export (const Buf* buf) 
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
buf_data(const Buf* buf) {
	return getdata(buf, buf->type);
}

size_t
buf_cap(const Buf* buf) {
	return getcap(buf, buf->type);
}

size_t
buf_len(const Buf* buf) {
	return getlen(buf, buf->type);
}

void buf_dbg(Buf* b)
{
    printf ("type %d cap %zu len %zu data '%s'\n", 
        b->type, buf_cap(b), buf_len(b), buf_data(b));
}