#include "../buffet.h"

int main() {

// SHARED OWN ===================================

    char large[] = "DATA STORE IS HEAP ALLOCATION.";

    // own1 will own a store holding a copy of `large`
    Buffet own1 = bft_memcopy(large, sizeof(large)-1);
    bft_dbg(&own1);
    // View "STORE" in own1
    Buffet own2 = bft_view(&own1, 5, 5);
    // Now own1 and own2 share the store, whose refcount is 2
    bft_dbg(&own2);

// SSO & SSV ===================================

    char small[] = "SMALL STRING";

    Buffet sso1 = bft_memcopy(small, sizeof(small)-1);
    bft_dbg(&sso1);
    // View "STRING" in sso1
    Buffet ssv1 = bft_view(&sso1, 6, 6);
    bft_dbg(&ssv1);

// VUE =========================================

    char any[] = "SOME BYTES";

    // View "BYTES" in `any`
    Buffet vue1 = bft_memview(any+5, 5);
    bft_dbg(&vue1);

    return 0;
}