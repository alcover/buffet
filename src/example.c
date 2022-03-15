#include <stdio.h>
#include "buffet.h"

int main()
{
	char text[] = "The train is fast";
	
	Buffet vue;
	buffet_strview (&vue, text+4, 5);
	buffet_print(&vue);

	text[4] = 'b';
	buffet_print(&vue);

	Buffet ref = buffet_view (&vue, 1, 4);
	buffet_print(&ref);

	char wet[] = " is wet";
	buffet_append (&ref, wet, sizeof(wet));
	buffet_print(&ref);

	return 0;
}

/* prints :
train
brain
rain
rain is wet
*/