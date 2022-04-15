#include "../buffet.h"

int main() {

// view own
Buffet own = buffet_memcopy("Bonjour monsieur buddy", 16);
Buffet Bonjour = buffet_view(&own, 0, 7);
buffet_debug(&Bonjour); // tag:REF cstr:'Bonjour'

// view ref
Buffet Bon = buffet_view(&Bonjour, 0, 3);
buffet_debug(&Bon); // tag:REF cstr:'Bon'

// detach views
buffet_append(&Bonjour, "!", 1); // "Bonjour!"
buffet_free(&Bon); 
buffet_free(&own); // OK

// view vue
Buffet vue = buffet_memview("Good day", 4); // "Good"
Buffet Goo = buffet_view(&vue, 0, 3);
buffet_debug(&Goo); // tag:VUE cstr:'Goo'

// view sso
Buffet sso = buffet_memcopy("Hello", 5);
Buffet Hell = buffet_view(&sso, 0, 4);
buffet_debug(&Hell); // tag:VUE cstr:'Hell'
buffet_free(&Hell); // OK
buffet_free(&sso); // OK

return 0;}