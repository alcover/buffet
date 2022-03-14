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