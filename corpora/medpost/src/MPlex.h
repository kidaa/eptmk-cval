#ifndef _MPLEX_H
#define _MPLEX_H

#include <Btree.h>

using namespace std;
using namespace iret;

// Implement a lexicon.

class MPlex
{
public:

	MPlex(int, int, char *, int);
	~MPlex();

	void add(char *entry);
	void addfile(char *file);
	void rm(char *entry);
	void rmfile(char *file);

	// Return the whole record for a word.

	char *get(char *word);

	// Determine if the word (or idiom) is in the lexicon

	int exists(char *word);

	// Return a probability of the tag given the word.
	// This can use any "machine learning" method to give a result

	double get(char *word, char *tag);
	double scan_lex(char *entry, char *tag);
	void search_string(char *buff, char *word);
	double count(char *word);

private:
	int	num_tags;
	Btree	tree;
	int	use_codes;
};

// These are the tag separators

#define sTAGSEP "_"
#define cTAGSEP '_'

#endif
