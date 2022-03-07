#include <stdio.h>
#include "buffet.h"

int main()
{
	char text[] = "The train is fast";
	
	Buffet vue;
	bft_strview (&vue, text+4, 5);
	bft_print(&vue);

	text[4] = 'b';
	bft_print(&vue);

	Buffet ref = bft_view (&vue, 1, 4);
	bft_print(&ref);

	char wet[] = " is wet";
	bft_append (&ref, wet, sizeof(wet));
	bft_print(&ref);

	return 0;
}

/* prints :
train
brain
rain
rain is wet
*/