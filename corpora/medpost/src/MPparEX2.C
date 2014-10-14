/* Port the parse.perl program */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <pcre.h>

#include "MPtag.h"
#include "MPpar.h"

#define MAXNP 100000

#define MOD		"(?: NNS| NNP| NN)(?: GE)?"
#define ADJ		"(?: RRR| RRT| RR)?(?: VVNJ| VVGJ| JJR| JJT| JJ)"
#define NUM		"(?: MC SYM| MC)"
#define HEAD		"(?: NNS| NNP| NN| PND| PN| VVGN)"

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

		mppar->match("(?:" MOD "|" ADJ "|" NUM ")*(?:" HEAD "|" NUM ") ", np, num_np);

		// printf("NPs:\n");
		for (int i = 0; i < num_np; i++)
		{
			printf("%s\n", np[i]);
			delete[] np[i];
		}

		delete mppar;
	}
}

