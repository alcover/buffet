// Playground

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buffet.h"
#include "log.h"


int main() {

{
Buffet buf;
bft_new(&buf, 20);
bft_dbg(&buf); 
}

{
Buffet copy;
bft_strcopy(&copy, "Bonjour", 3);
bft_dbg(&copy);
}

{
char src[] = "Eat Buffet!";
Buffet view;
bft_strview(&view, src+4, 3);
bft_dbg(&view);
// type:VUE cap:0 len:6 data:'Buffet!'
bft_print(&view);
// Buf
}

{
Buffet buf;
bft_strcopy(&buf, "abc", 3); 
size_t newlen = bft_append(&buf, "def", 3); // newlen == 6 
bft_dbg(&buf);
}

{
Buffet buf;
bft_strcopy(&buf, "Too long for SSO..........", 16);
bft_dbg(&buf);
bft_free(&buf);
bft_dbg(&buf);
}

{
Buffet buf;
bft_strcopy(&buf, "foo", 3);
bft_dbg(&buf);
}

{
Buffet buf;
bft_strcopy(&buf, "Too long for SSO..........", 16);
bft_dbg(&buf);
// type:OWN cap:32 len:16 data:'Too long for SSO'
bft_free(&buf);
bft_dbg(&buf);	
}

return 0;
}