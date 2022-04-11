// Playground

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buffet.h"
#include "log.h"

int main() { 

    Buffet own; 
    buffet_memcopy (&own, "aaaaaaaaaaaaaaaaaaaaaaaaaaaa", 20); 
    Buffet ref = buffet_view (&own, 0, 8);
    Buffet alias = ref;
    buffet_free(&ref);
    buffet_free(&own);

    buffet_print(&alias);
    buffet_append(&alias, "gogo", 4);


return 0;}