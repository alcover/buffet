#include "../buffet.h"

int main() {

char text[] = "Le grand orchestre";

Buffet own = bft_memcopy(text, sizeof(text));
Buffet ref = bft_view(&own, 9, 9); // 'orchestre'

// Too soon but marked for release
bft_free(&own);

// Was last ref, data gets actually released
bft_free(&ref);

return 0;}