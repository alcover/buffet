#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdalign.h>

#include "buf.h"
#include "log.h"
#include "util.c"
#include "teststr.h"

struct test
{
	const char* p;
};


int main()
{
	// printf("%d\n", __builtin_clzll((unsigned long long)(-1)));
	// printf("%d\n", __builtin_clzll(0ull));
	// printf("%d\n", __builtin_clzll(1ull));
	// printf("%d\n", next_pow2(0ull));
	// printf("%d\n", next_pow2(1ull));
	// printf("%d\n", next_pow2(31ull));
	// printf("%d\n", next_pow2(32ull));
	// printf("%d\n", next_pow2(33ull));

	// printf("Buf align %zu\n", alignof(Buf)); // 8

	// Buf i;
	// memset(&i,0,sizeof(Buf));
	// printf("%p\n", i.sso);

	// test();

	// Buf b;
	// buf_copy_str (&b, w7, strlen(w7));
	// buf_dbg(&b);

	// buf_append (&b, w8, strlen(w8));
	// buf_dbg(&b);


	// Buf d;
	// buf_copy_str(&d, w15, 15);
	// buf_dbg(&d);

	// Buf c;
	// buf_new (&c, 0);
	// buf_append (&c, w15, 15);

	// printf("%d\n", buf_len(&c));
	// printf("%s\n", buf_data(&c));

	// buf_free(&b);
	// buf_free(&b);

	// Buf v = buf_slice(&c, 0, 16);
	// bits_print(v.off);

	char* s = malloc(3);
	strcpy(s,"hi");
	LOGVS(s);

	struct test t;
	t.p = s;
	((char*)(t.p))[1] = 'o';
	LOGVS(t.p);

	return 0;
}