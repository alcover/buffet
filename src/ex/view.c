#include "../buffet.h"

int main() {

    char text[] = "Bonjour monsieur Buddy. Already speaks french!";

    // view sso
    Buffet sso = bft_memcopy(text, 16); // "Bonjour monsieur"
    Buffet ssv = bft_view(&sso, 0, 7);
    bft_dbg(&ssv);

    // view ssv
    Buffet Bon = bft_view(&ssv, 0, 3);
    bft_dbg(&Bon);

    // view own
    Buffet own = bft_memcopy(text, sizeof(text));
    Buffet ownview = bft_view(&own, 0, 7);
    bft_dbg(&ownview);

    // detach view
    bft_append (&ownview, "!", 1);
    // bft_free(&ownview); 
    bft_free(&own); // Done

    // view vue
    Buffet vue = bft_memview(text+8, 8); // "Good"
    Buffet mon = bft_view(&vue, 0, 3);
    bft_dbg(&mon);

    return 0;
}