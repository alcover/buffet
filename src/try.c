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
	buffet_strcopy (&local, alpha, 40);
	return local;	
}

int main() {

	// Buffet own = pass_owner();
	// Buffet ref = buffet_view (&own, 10, 4); // "abcd"
	// buffet_free(&own);
	// buffet_free(&ref);

{	// aliasing then free
	Buffet own;
	buffet_strcopy(&own, alpha, 40);
	Buffet  cpy_alias = own;
	Buffet *ptr_alias = &own;

	buffet_free(&own);

	buffet_free(&cpy_alias);
	buffet_dbg (&cpy_alias);

	buffet_free(ptr_alias);
	buffet_dbg (ptr_alias);
}

	return 0;
}