#ifndef _MPTOK_H
#define _MPTOK_H
#include <stdio.h>

#define MPTOK_VERSION 9				// The latest version
#define MAX_PAIR_LEN 100			// Maximum length of string in pair file

class MPtok
{
public:

	MPtok(char *cnam = "");
	~MPtok();

	void init();
	void init(char *idir);			// Initialize with default pair file

	char option_pretag[1000];		// The tag to use on tokens
	int option_token;			// Output tokenized text
	int option_segment;			// Segment into sentences
	int option_hyphen;			// Hyphens are separate tokens
	int option_comma;			// Commas are always tokenized
	int option_pretok;			// The text is pre-tokenized
	int option_new;				// Use new algorithms, used in development only
	int option_doteos;			// If " . " occurs, it's an end EOS (new >= 5)

	void set_segment(int i);		// set the segment option
	void set_token(int i);			// set the token option
	void set_hyphen(int i);			// set the hyphen option
	void set_comma(int i);			// set the comma option
	void set_pretag(char *a);		// set the pretag option
	void set_pretok(int i);			// set the pretok option
	void set_new(int i);			// set the new option
	void set_doteos(int i);			// set the doteos option

	void init_common_pairs(char *file_name, int max_pairs);	// read a file of common pairs

	char *tokenize(char *text);		// return space delimited tokenized text, must delete
	char *tokenize_pre(char *text);		// return space delimited pre-tokenized text, must delete
	void tokenize_save(char *);		// save the tokens
	void tokenize_presave(char *);		// save the pretokenized tokens
	void segment_save(char *);		// save the sentences
	void save_string(char *);		// save a buffer: WARNING, the string must be new and it is consumed

	char	*word(int i);			// Return the ith token
	char	*sent(int i);			// Return the ith sentence

	int	num_words;			// Number of words
	int	num_sents;			// Number of sentences

	char	*word_str;			// A single allocated string of all tokens
	char	**word_ptr;			// Pointers to the tokens in word_str

	char	*sent_str;			// A single allocated string of all sentences
	char	**sent_ptr;			// Pointers to the sentences in sent_str

	char	*suf;				// A suffix, for opening variant support files

private:

	void set_tokflag(const char*, int*);
	void set_endflag(const char*, int*, int*);
	void set_endflag_01(const char*, int*, int*);
	int size_buff(const char*, int*, int*);

	void tok_0(const char *, int *);
	void tok_1(const char *, int *);
	void tok_2(const char *, int *);
	void tok_3(const char *, int *);
	void tok_5_6_7(const char *, int *);
	void tok_8_9(const char *, int *);
	void tok_10(const char *, int *);
	void tok_11(const char *, int *);
	void tok_12(const char *, int *);
	void tok_13(const char *, int *);
	void tok_14(const char *, int *);
	void tok_15(const char *, int *);
	void tok_15_1(const char *, int *);
	void tok_16(const char *, int *);
	void tok_16_1(const char *, int *);
	void tok_17(const char *, int *);
	void tok_20(const char *, int *);
	void tok_20_1(const char *, int *);
	void tok_20_2(const char *, int *);
	void tok_21(const char *, int *);
	void tok_21a(const char *, int *);
	void tok_22(const char *, int *);
	void tok_23(const char *, int *);
	void tok_24(const char *, int *);
	void tok_25(const char *, int *);
	void tok_26(const char *, int *);
	void tok_27(const char *, int *);
	void tok_28(const char *, int *);
	void tok_29(const char *, int *);
	void tok_29a(const char *, int *);
	void tok_30(const char *, int *);
	void tok_31(const char *, int *);
	void tok_32(const char *, int *);
	void tok_33(const char *, int *);

	char* save_tok(char*, int&, char*, int);
	char* save_sents(const char*, int*, int*);

	char **common_pair;
	int num_common_pairs;

	int find_common_pair(const char*, int, int*);
};

#endif

