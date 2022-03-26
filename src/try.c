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
	// view own
	Buffet own;
	buffet_strcopy(&own, "Bonjour monsieur", 16);
	Buffet ref1 = buffet_view(&own, 0, 7);
	buffet_debug(&ref1); // tag:REF len:7 cstr:'Bonjour'

	// view ref
	Buffet ref2 = buffet_view(&ref1, 0, 3);
	buffet_debug(&ref2); // tag:REF len:3 cstr:'Bon'

	// detach views
	buffet_append(&ref2, "net", 3);
	buffet_debug(&ref2); // tag:SSO len:6 cstr:'Bonnet'
	buffet_append(&ref1, "!", 1);
	buffet_debug(&ref1); // tag:SSO len:8 cstr:'Bonjour!'
	buffet_free(&own);   // OK

	// view vue
	Buffet vue;
	buffet_strview(&vue, "Good day", 4);
	Buffet vue2 = buffet_view(&vue, 0, 3);
	buffet_debug(&vue2); // tag:VUE len:3 cstr:'Goo'

	// view sso
	Buffet sso;
	buffet_strcopy(&sso, "Bonjour", 7);
	Buffet vue3 = buffet_view(&sso, 0, 3);
	buffet_debug(&vue3); // tag:VUE len:3 cstr:'Bon'
}

// {
//     char text[] = "The train goes.";

// 	Buffet own;
// 	buffet_strcopy (&own, text, sizeof(text));

//     Buffet ref = buffet_view (&own, 4, 5);
//     buffet_print(&ref); // "train"

//     buffet_append (&ref, "ing", 3);
//     buffet_print(&ref); // "training"
    
//     Buffet vue;
//     buffet_strview (&vue, text+4, 5);
//     buffet_print(&vue); // "train"

//     text[4] = 'b';
//     buffet_print(&vue); // "brain"
// }   

// {
// 	int cnt;
// 	Buffet *parts = buffet_splitstr("Split me", 8, " ", 1, &cnt);
// 	for (int i=0; i<cnt; ++i) {
// 	    buffet_debug(&parts[i]);
// 	}
// 	buffet_list_free(parts,cnt);	
// } 
// {
// 	int cnt;
// 	Buffet *parts = buffet_splitstr("Split me", 8, " ", 1, &cnt);
// 	Buffet back = buffet_join(parts, cnt, " ", 1);
// 	buffet_debug(&back);
// 	buffet_list_free(parts,cnt);
// 	buffet_free(&back);
// }


	// Buffet own = pass_owner();
	// Buffet ref = buffet_view (&own, 10, 4); // "abcd"
	// buffet_free(&own);
	// buffet_free(&ref);

// puts("");
// {	// aliasing then free
// 	Buffet own;
// 	buffet_strcopy(&own, alpha, 40);
// 	Buffet  cpy_alias = own;
// 	Buffet *ptr_alias = &own;
// 	Buffet ref = buffet_view(&own,0,4);

// 	buffet_free(&own);

// 	buffet_free(&cpy_alias);
// 	buffet_debug (&cpy_alias);

// 	buffet_free(ptr_alias);
// 	buffet_debug (ptr_alias);
// }

// {
// 	int nparts;
// 	Buffet *parts = buffet_splitstr ("a b", 3, " ", 1, &nparts);
// 	for (int i = 0; i < nparts; ++i) {
// 		buffet_print(&parts[i]);
// 	}
// }

	return 0;
}