#include "../buffet.h"

int main() {
    
//==== SHARED OWN =================

    char large[] = "DATA STORE IS HEAP ALLOCATION.";

    Buffet own1 = bft_memcopy(large, sizeof(large)-1);
    // Now own1 owns a store housing a copy of `large`
    bft_dbg(&own1); 
    //-> OWN 30 "DATA STORE ..."
    // View "STORE" in own1 :
    Buffet own2 = bft_view(&own1, 5, 5);
    // Now own1 and own2 share the store, whose refcount is 2
    bft_dbg(&own2); 
    //-> OWN 5 "STORE"

//==== SSO & SSV =================

    char small[] = "SMALL STRING";

    Buffet sso1 = bft_memcopy(small, sizeof(small)-1);
    bft_dbg(&sso1); 
    //-> SSO 12 "SMALL STRING"
    // View "STRING" in sso1 :
    Buffet ssv1 = bft_view(&sso1, 6, 6);
    bft_dbg(&ssv1); 
    //-> SSV 6 "STRING"

//==== VUE =======================

    char any[] = "SOME BYTES";

    // View "BYTES" in `any` :
    Buffet vue1 = bft_memview(any+5, 5);
    bft_dbg(&vue1); 
    //-> VUE 5 "BYTES"

    return 0;
}
