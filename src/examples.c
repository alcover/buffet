// README code examples

#include <stdlib.h>
#include "buffet.h"

int main() { 

{/*example*/
char text[] = "The train goes";

Buffet vue;
buffet_memview (&vue, text+4, 5);
buffet_print(&vue); // "train"

text[4] = 'b';
buffet_print(&vue); // "brain"

Buffet ref = buffet_view (&vue, 1, 4);
buffet_print(&ref); // "rain"

char tail[] = "ing";
buffet_append (&ref, tail, sizeof(tail));
buffet_print(&ref); // "raining"
}	

{/*buffet_new*/
Buffet buf;
buffet_new(&buf, 20);
buffet_debug(&buf); 
// tag:OWN cap:32 len:0 cstr:''
}

{/*buffet_memcopy*/
Buffet copy;
buffet_memcopy(&copy, "Bonjour", 3);
buffet_debug(&copy); 
// tag:SSO cap:14 len:3 cstr:'Bon'
}


{/*buffet_memview*/
char src[] = "Eat Buffet!";
Buffet view;
buffet_memview(&view, src+4, 6);
buffet_debug(&view);
// tag:VUE cap:0 len:6 cstr:'Buffet'
}


{/*buffet_view*/
// view own
Buffet own;
buffet_memcopy(&own, "Bonjour monsieur", 16);
Buffet ref1 = buffet_view(&own, 0, 7);
buffet_debug(&ref1); // tag:REF cstr:'Bonjour'

// view ref
Buffet ref2 = buffet_view(&ref1, 0, 3);
buffet_debug(&ref2); // tag:REF cstr:'Bon'

// detach views
buffet_append(&ref1, "!", 1);   // "Bonjour!"
buffet_append(&ref2, "net", 3); // "Bonnet"
buffet_free(&own); // OK

// view vue
Buffet vue;
buffet_memview(&vue, "Good day", 4); // "Good"
Buffet vue2 = buffet_view(&vue, 0, 3);
buffet_debug(&vue2); // tag:VUE cstr:'Goo'

// view sso
Buffet sso;
buffet_memcopy(&sso, "Bonjour", 7);
Buffet vue3 = buffet_view(&sso, 0, 3);
buffet_debug(&vue3); // tag:VUE cstr:'Bon'
}


{/*buffet_new*/
Buffet buf;
buffet_new(&buf, 20);
buffet_debug(&buf); 
// tag:OWN cap:32 len:0 cstr:''
}


{/*buffet_free*/
char text[] = "Le grand orchestre de Patato Valdez";

Buffet own;
buffet_memcopy(&own, text, sizeof(text));
// 'Le grand orchestre de Patato Valdez'

Buffet ref = buffet_view(&own, 22, 13);
// 'Patato Valdez'

// Too soon but data marked for release
buffet_free(&own);
buffet_debug(&own);
// tag:SSO cstr:''

// Release last ref, so data gets released
buffet_free(&ref);
}


{/*buffet_append*/
Buffet buf;
buffet_memcopy(&buf, "abc", 3); 
buffet_append(&buf, "def", 3); // newlen == 6 
buffet_debug(&buf);
// tag:SSO cap:14 len:6 cstr:'abcdef'
}


{/*buffet_split*/
int cnt;
Buffet *parts = buffet_split("Split me", 8, " ", 1, &cnt);
for (int i=0; i<cnt; ++i)
    buffet_debug(&parts[i]);
free(parts);
// tag:VUE cap:0 len:5 cstr:'Split'
// tag:VUE cap:0 len:2 cstr:'me'
}



{/*buffet_join*/
int cnt;
Buffet *parts = buffet_splitstr("Split me", " ", &cnt);
Buffet back = buffet_join(parts, cnt, " ", 1);
buffet_debug(&back);
// tag:SSO cap:14 len:8 cstr:'Split me'
}



return 0;}