#include "../buffet.h"

int main() {

	char text[] = "Bonjour monsieur Buddy. Already speaks french!";

	// view own
	Buffet own = buffet_memcopy(text, sizeof(text));
	Buffet Bonjour = buffet_view(&own, 0, 7);
	buffet_debug(&Bonjour); // REF cstr:'Bonjour'

	// view ref
	Buffet Bon = buffet_view(&Bonjour, 0, 3);
	buffet_debug(&Bon); // REF cstr:'Bon'

	// detach views
	buffet_append(&Bonjour, "!", 1);
	buffet_free(&Bon); 
	buffet_free(&own); // OK

	// view vue
	Buffet vue = buffet_memview("Good day", 4); // "Good"
	Buffet Goo = buffet_view(&vue, 0, 3);
	buffet_debug(&Goo); // VUE cstr:'Goo'

	// view sso
	Buffet sso = buffet_memcopy("Hello", 5);
	Buffet Hell = buffet_view(&sso, 0, 4);
	buffet_debug(&Hell); // VUE cstr:'Hell'
	buffet_free(&Hell);	 // OK
	buffet_free(&sso); 	 // OK

	return 0;
}