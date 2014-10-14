// This gets the sentences (forcing option_token = 0 and option_segment = 1)
// and stores local copies that can be retreived with sent(i).

#include <stdio.h>
#include "MPtok.h"

MPtok mptok;

main()
{
	char cnam[1000] = "Is this not a simple-short string? It is not, considering that it has two sentences.";

	mptok.segment_save(cnam);
	for (int i = 0; i < mptok.num_sents; i++)
		printf("sent %d: %s\n", i, mptok.sent(i));
}
