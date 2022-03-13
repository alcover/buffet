// Playground

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buffet.h"
#include "log.h"

const char 
alpha[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+=";
const size_t alphalen = 64;

int main() {

{
	Buffet own;
	bft_strcopy (&own, alpha, 40);
	Buffet ref = bft_view (&own, 10, 4); // "abcd"
	bft_free(&own);
	bft_free(&ref);
}


return 0;}