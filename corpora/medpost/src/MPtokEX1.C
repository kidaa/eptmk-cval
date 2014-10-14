// This is an example of how to call the tokenizer
// it returns an allocated string with tokenized text.
// If tokens are requested, they are separated by spaces.
// If sentences are segmented, they are separated by newlines (\n).
// Be sure and delete this string when you're done with it.
// Note: the last sentence will not end with a newline.

#include <stdio.h>
#include "MPtok.h"

MPtok mptok;

main()
{
	char cnam[1000] = "Is this not a simple-short string? It is not, considering that it has two sentences.\n";

	// mptok.set_hyphen(1);			// Hyphens are tokenized separately (default no)
	// mptok.set_comma(0);			// Commas followed by non-space are tokenized (default yes)
	// mptok.set_pretag("NOTAG");		// Tokenized output is pre-tagged (default no)
	// mptok.set_segment(0);		// Output of tokenize() is segmented into sentences (default yes)
	// mptok.set_token(0);			// Output of tokenize() is separated into tokens (default yes)
	// mptok.set_pretok(1);			// The input is already tokenized, don't redo it

	char	*out = mptok.tokenize(cnam);
	printf("%s\n", out);
	delete[] out;
}
