#include <stdio.h>
#include "buffet.h"

int main()
{
	char text[] = "The train goes";
	
	Buffet vue;
	buffet_memview (&vue, text+4, 5);
	buffet_print(&vue); // "train"

	text[4] = 'b';
	buffet_print(&vue); // "brain"

	Buffet ref = buffet_view (&vue, 1, 4);
	buffet_print(&ref); // "rain"

	char tail[] = "ing";
	buffet_append (&ref, tail, sizeof(tail));
	buffet_print(&ref); // "raining"

	return 0;
}