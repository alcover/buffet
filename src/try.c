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
	// reproduce leak - https://news.ycombinator.com/item?id=30601517
	Buffet own;
	bft_strcopy (&own, alpha, 40);
	Buffet ref = bft_view (&own, 10, 4); // "abcd"
	bft_free(&own);
	bft_free(&ref);
	// With new `wantfree` field :
	// $ valgrind --leak-check=full ./bin/try
	// ERROR SUMMARY: 0 errors 
}

{
	// readme::bft_free
	char text[] = "Le grand orchestre de Patato Valdez";
	Buffet own;
	bft_strcopy(&own, text, sizeof(text));
	bft_dbg(&own);

	Buffet ref = bft_view(&own, 22, 13);
	bft_dbg(&ref);

	// Too soon but marked for release
	bft_free(&own);
	bft_dbg(&own); // -> unchanged

	// release last refcnt, hence release owner
	bft_free(&ref);
	bft_dbg(&own);	
}

// {
// Buffet copy;
// bft_strcopy(&copy, "Bonjour", 3);
// bft_dbg(&copy);
// }

// {
// char src[] = "Eat Buffet!";
// bft_dbg(&view);
// // type:VUE cap:0 len:6 data:'Buffet!'
// bft_print(&view);
// // Buf
// }

// {
// Buffet buf;
// bft_strcopy(&buf, "abc", 3); 
// size_t newlen = bft_append(&buf, "def", 3); // newlen == 6 
// bft_dbg(&buf);
// }


// {
// Buffet buf;
// bft_strcopy(&buf, "foo", 3);
// bft_dbg(&buf);
// }

// {
// Buffet buf;
// bft_strcopy(&buf, "Too long for SSO..........", 16);
// bft_dbg(&buf);
// // type:OWN cap:32 len:16 data:'Too long for SSO'
// bft_free(&buf);
// bft_dbg(&buf);	
// }

return 0;}