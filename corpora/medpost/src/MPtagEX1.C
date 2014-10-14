// Tag text from a string

#include <stdio.h>
#include "MPtag.h"

MPtag mptag;

main(int argc, char **argv)
{
	char cnam[1000] = "Is this not a simple-short string? It is not, considering that it has two sentences.\n";

	mptag.init();
	mptag.viterbi(cnam);
	mptag.split_idioms();		// Idioms are merged by the tagger, they can then be split up if desired

	printf("\nresult:\n");
	for (int i = 0; i < mptag.num_words; i++)
		printf("%s_%s\n", mptag.word(i), mptag.tag(i));
}
