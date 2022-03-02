#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffet.h"
#include "log.h"

int main()
{
	char text[] = "The train is fast";
	Buffet view;

	bft_strview (&view, text+4, 5);
	bft_print(&view);

	text[4] = 'b';
	bft_print(&view);

	Buffet view2 = bft_view (&view, 1, 4);
	bft_print(&view2);

	char wet[] = " is wet";
	bft_append (&view2, wet, strlen(wet));
	bft_print(&view2);

	return 0;
}