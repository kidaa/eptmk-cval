// This gets the tokens (forcing option_token = 1 and option_segment = 0)
// and stores local copies that can be retreived with word(i).

#include <stdio.h>
#include "MPtok.h"

MPtok mptok;

main()
{
	char cnam[1000] = "Is this not a simple-short string? It is not, considering that it has two sentences.";

	mptok.tokenize_save(cnam);
	for (int i = 0; i < mptok.num_words; i++)
		printf("word %d: %s\n", i, mptok.word(i));
}
