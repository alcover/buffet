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

	Buffet own = pass_owner();
	Buffet ref = buffet_view (&own, 10, 4); // "abcd"
	buffet_free(&own);
	buffet_free(&ref);

{
	Buffet src;
	buffet_strcopy(&src, "Bonjour", 7);
	Buffet ref = buffet_view(&src, 0, 3);
	buffet_dbg(&ref);
	buffet_print(&ref);
}

	return 0;
}