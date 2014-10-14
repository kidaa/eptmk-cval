// Tag sentences seperately

#include <stdio.h>
#include "MPtag.h"

MPtag mptag;

main(int argc, char **argv)
{
	char cnam[1000] = "Is this not a simple-short string? It is not, considering that it has two sentences.";

	mptag.init();

	mptag.segment_save(cnam);

	for (int num = 0; num < mptag.num_sents; num++)
	{
		mptag.viterbi(mptag.sent(num));

		printf("\nresult for sentence %d:\n", num+1);
		for (int i = 0; i < mptag.num_words; i++)
			printf("%s_%s\n", mptag.word(i), mptag.tag(i));
	}
}
