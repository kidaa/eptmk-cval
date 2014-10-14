/* Port the parse.perl program */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <pcre.h>

#include "MPtag.h"
#include "MPpar.h"

#define MAXNP 100000

int main(int argc, char **argv)
{
	char	buff[MAXBUFF];
	MPpar	*mppar;
	MPtag	*mptag = new MPtag;
	char	*np[MAXNP];
	int	num_np;

	while (fgets(buff, MAXBUFF, stdin))
	{
		// Tag the text with parts of speech

		mptag->init();
		mptag->viterbi(buff);
		mptag->split_idioms();

		// Parse the tagged text

		mppar = new MPpar(mptag);

		// Options that affect the parser:
		// comment these out or change the numbers if desired

		mppar->option_parens = 0;	// paren options: 0=remove, 1=leave, 2=replace
		mppar->option_conj = 0;		// conj options: 0=ignore, 1=simple, 2=compound
		mppar->option_gerund = 0;	// gerund options: 0=no object, 1=object
		mppar->option_postmod = 0;	// postmode options: 0=no attach, 1=attach

		mppar->parse();

		// Print the NPs found, free memory along the way

		mppar->list_nodes("NP", np, num_np);
		// printf("NPs:\n");
		for (int i = 0; i < num_np; i++)
		{
			printf("%s\n", np[i]);
			delete[] np[i];
		}

		delete mppar;
	}
}

