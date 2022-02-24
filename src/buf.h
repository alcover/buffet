#include <stddef.h>
#include <stdint.h>

#define BUF_SIZE 16
#define SSO_CAP (BUF_SIZE-2) // leaving room for NUL & flags
#define TYPE_BITS 2


typedef union Buf {

    char fill[BUF_SIZE];
    
    struct {
    	char	sso[BUF_SIZE-1];
  		uint8_t	ssolen:4, :4;
    };
	
	struct { 
		uint32_t len; 
	    union {
		    uint32_t off;
		    struct {
			    uint16_t refcnt;
			    uint8_t  cap;
			    uint8_t  :8;
		    };
	    };
		intptr_t data : 8*sizeof(intptr_t)-TYPE_BITS;
		uint8_t  type : TYPE_BITS;
	};

} __attribute__((aligned(BUF_SIZE))) Buf;


void 	buf_new		(Buf *dst, size_t cap);
void 	buf_copy_str (Buf *dst, const char* src, size_t len);
void	buf_view_str (Buf* dst, const char* src, size_t len);
Buf 	buf_slice 	(Buf *src, size_t off, size_t len);
Buf		buf_view 	(Buf *src, size_t off, size_t len);
void	buf_free 	(Buf *buf);
size_t 	buf_append 	(Buf *b, const char* src, size_t srclen);

size_t	buf_cap (const Buf* buf);
size_t	buf_len (const Buf* buf);
char*	buf_data (const Buf* buf);
const
char*	buf_cstr (const Buf* buf);
char*	buf_export (const Buf* buf) ;

void 	buf_dbg (Buf* b);