// Playground

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buffet.h"
#include "log.h"

const char 
alpha[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+=";
const size_t alphalen = 64;

Buffet pass_owner(){
	Buffet local;
	bft_strcopy (&local, alpha, 40);
	return local;	
}

int main() {

	#define MUL 1.618033988749
	size_t a=2;
	for (int i = 0; i < 10; ++i)
	{
		a *= MUL;
		LOGVI(a);
	}

// {
// 	Buffet own;
// 	bft_strcopy (&own, alpha, 40);
// 	Buffet ref = bft_view (&own, 10, 4); // "abcd"
// 	bft_free(&own);
// 	bft_free(&ref);
// }

Buffet own = pass_owner();
Buffet ref = bft_view (&own, 10, 4); // "abcd"
bft_free(&own);
bft_free(&ref);

return 0;}