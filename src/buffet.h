#include <stddef.h>
#include <stdint.h>

#define BFT_SIZE 16
#define BFT_SSO_CAP (BFT_SIZE-2) // leaving room for NUL & flags
#define BFT_TYPE_BITS 2


typedef union Buffet {

	// init
	char fill[BFT_SIZE];
	
	// embed
	struct {
		char	sso[BFT_SIZE-1];
		uint8_t	ssolen:4, :4;
	};
	
	// reference
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
		intptr_t data : 8*sizeof(intptr_t)-BFT_TYPE_BITS;
		uint8_t  type : BFT_TYPE_BITS;
	};

} __attribute__((aligned(BFT_SIZE))) Buffet;


void	bft_new (Buffet *dst, size_t cap);
void 	bft_strcopy (Buffet *dst, const char *src, size_t len);
void	bft_strview (Buffet *dst, const char *src, size_t len);
size_t 	bft_append  (Buffet *dst, const char *src, size_t len);
Buffet 	bft_slice (Buffet *src, size_t off, size_t len);
Buffet	bft_view  (Buffet *src, size_t off, size_t len);
void	bft_free (Buffet *buf);

size_t	bft_cap (const Buffet* buf);
size_t	bft_len (const Buffet* buf);
char*	bft_data (const Buffet* buf);
const
char*	bft_cstr (const Buffet* buf);
char*	bft_export (const Buffet* buf) ;

void 	bft_dbg (Buffet* b);