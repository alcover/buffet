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

{
	int cnt;
	Buffet *parts = buffet_split("Split me", 8, " ", 1, &cnt);
	for (int i=0; i<cnt; ++i) {
	    buffet_debug(&parts[i]);
	}	
}
{
	int cnt;
	Buffet *parts = buffet_split("Split me", 8, " ", 1, &cnt);
	Buffet back = buffet_join(parts, cnt, " ", 1);
	buffet_debug(&back);	
}


	// Buffet own = pass_owner();
	// Buffet ref = buffet_view (&own, 10, 4); // "abcd"
	// buffet_free(&own);
	// buffet_free(&ref);

// {	// aliasing then free
// 	Buffet own;
// 	buffet_strcopy(&own, alpha, 40);
// 	Buffet  cpy_alias = own;
// 	Buffet *ptr_alias = &own;

// 	buffet_free(&own);

// 	buffet_free(&cpy_alias);
// 	buffet_debug (&cpy_alias);

// 	buffet_free(ptr_alias);
// 	buffet_debug (ptr_alias);
// }

// {
// 	int nparts;
// 	Buffet *parts = buffet_split ("a b", 3, " ", 1, &nparts);
// 	for (int i = 0; i < nparts; ++i) {
// 		buffet_print(&parts[i]);
// 	}
// }

	return 0;
}