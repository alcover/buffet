#include "../buffet.h"

int main() {

    char text[] = "Le grand orchestre de Patato Valdez";

    Buffet own = bft_memcopy(text, sizeof(text));
    Buffet ref = bft_view(&own, 9, 9); // "orchestre"
    bft_free(&own); // A bit soon but ok, --refcnt
    bft_dbg(&own);  // SSO 0 ""
    bft_free(&ref); // Was last co-owner, store is released

    Buffet sso = bft_memcopy(text, 8); // "Le grand"
    Buffet ref2 = bft_view(&sso, 3, 5); // "grand"
    bft_free(&sso); // WARN line:328 bft_free: SSO has views on it
    bft_free(&ref2);
    bft_free(&sso); // OK now
    bft_dbg(&sso);  // SSO 0 ""

    return 0;
}
 