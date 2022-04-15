#include "../buffet.h"

int main() {

char text[] = "Le grand orchestre";

Buffet own = buffet_memcopy(text, sizeof(text));
Buffet ref = buffet_view(&own, 9, 9); // 'orchestre'

// Too soon but marked for release
buffet_free(&own);

// Was last ref, data gets actually released
buffet_free(&ref);

return 0;}