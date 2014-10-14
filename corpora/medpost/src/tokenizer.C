/* tokenizer_main.C
**
** This function performs tokenization of raw input to be read by a tagger.
*/

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "MPtok.h"

MPtok *mptok = NULL;

// This is the callable interface

#define MAX_BUFF 10000

// Options passed on to the tagger

static char install_dir[1000];		// The location where to find lex and ngram files
static char option_compute[1000];	// How to perform tagging (viterbi, compute)
static char option_print[1000];		// How to print the result
static int option_silent;		// Whether the tagger should be silent or verbose

// Options relating to the input and what to tag

static char input_file[1000];		// The input file
static char option_input[1000];		// The format of the input file (itame, medpost)
static int option_textid;		// Whether the input file has an id
static int option_titles;		// Whether to tag the titles that are found

// Options related to output

static int option_id;			// Whether to print an id with each sentence
static char option_letter[1000];	// The letter used to prefix the ids
static int option_loc;			// Whether to put the sentence number in the id
static char option_untag[1000];		// A dummy tag to apply to each token, if desired

// Values found in each record that is read from the file

static char input_pmid[1000];		// The pmid
static char input_loc[1000];		// The location (title or abstract)
static char input_id[1000];		// The id (eg option_textid)

// Internal variables

static FILE *input_fp = NULL;
static FILE *option_ofp = NULL;

static int process_tokenizer_args(int argc, char **argv)
{
	strcpy(install_dir, "");
	strcpy(option_compute, "viterbi");
	strcpy(option_print, "printsent");
	option_silent = 1;

	strcpy(input_file, "");
	strcpy(option_input, "itame");
	option_textid = 0;
	option_titles = 0;

	option_id = 0;
	strcpy(option_letter, "P");
	option_loc = 1;
	strcpy(option_untag, "");

	for (int i = 1; i < argc; i++)
	{
		// Process any input file arguments, there can be at most one

		if (strcmp(argv[i], "-") == 0 || argv[i][0] != '-')
		{
			if (strlen(input_file) > 0)
			{
				fprintf(stderr, "Only one input file may be specified.\n");
				return 0;
			}
			strcpy(input_file, argv[i]);
		}

		// Tokenizer options (related to how text is tokenized)

		if (strcmp(argv[i], "-cnam") == 0) mptok = new MPtok(argv[++i]);
		if (strcmp(argv[i], "-nosegment") == 0) mptok->set_segment(0);
		if (strcmp(argv[i], "-notokens") == 0) mptok->set_token(0);
		if (strcmp(argv[i], "-hyphen") == 0) mptok->set_hyphen(1);
		if (strcmp(argv[i], "-pretag") == 0) mptok->set_pretag(argv[++i]);

		// Tagger options (related to how to interpret the input and what to do with it)

		if (strcmp(argv[i], "-untag") == 0) strcpy(option_untag, argv[++i]);
		if (strcmp(argv[i], "-titles") == 0) option_titles = 1;
		if (strcmp(argv[i], "-id") == 0) option_id = 1;
		if (strcmp(argv[i], "-xml") == 0) strcpy(option_input, "xml");
		if (strcmp(argv[i], "-medline") == 0) strcpy(option_input, "medline");
		if (strcmp(argv[i], "-text") == 0) strcpy(option_input, "text");
		if (strcmp(argv[i], "-idtext") == 0) { strcpy(option_input, "text"); option_textid = 1; }
		if (strcmp(argv[i], "-token") == 0) { strcpy(option_input, "token"); mptok->set_segment(0); }
		if (strcmp(argv[i], "-idtoken") == 0) { strcpy(option_input, "token"); option_textid = 1; mptok->set_segment(0); }
		if (strcmp(argv[i], "-input") == 0) strcpy(input_file, argv[++i]);
		if (strcmp(argv[i], "-home") == 0) strcpy(install_dir, argv[++i]);
		if (strcmp(argv[i], "-viterbi") == 0) strcpy(option_compute, "viterbi");
		if (strcmp(argv[i], "-mle") == 0) strcpy(option_compute, "compute");
		if (strcmp(argv[i], "-noop") == 0) strcpy(option_compute, "");
		if (strcmp(argv[i], "-letter") == 0) strcpy(option_letter, argv[++i]);
		if (strcmp(argv[i], "-noloc") == 0) option_loc = 0;

		if (strcmp(argv[i], "-printfull") == 0) strcpy(option_print, "printfull");
		if (strcmp(argv[i], "-verbose") == 0) option_silent = 0;
	}

	// If the install_dir was not specified on the command line,
	// look for path_medpost in the current working directory;
	// if it's not there, then look for an environment variable;
	// finally, fallback to a hard coded path

	if (strlen(install_dir) == 0)
		strcpy(install_dir, "/home/lsmith/medpost");

	return 1;
}

static char *skip(char *text, char *what)
{
	while (*text && strchr(what, *text)) ++text;
	return text;
}

static void make_sent_id(int n)
{
	sprintf(input_id, "%s%08s", option_letter, input_pmid);
	if (option_loc)
	{
		sprintf(input_id + strlen(input_id), "%s", input_loc);
		sprintf(input_id + strlen(input_id), "%02d", n);
	}
	for (char *s = &input_id[0]; *s; s++)
		if (isspace(*s)) *s = '0';
}

// print the sentences after tokenizing
// including sentence id and medpost commands to tag
// this function overwites the data, and it can't be used again

static void print_sentences(char *out)
{
	char	*p;
	int	sent_num = 0;

	// Look for end-of-sentences

	while (out)
	{
		// Skip any leading space

		while (isspace(*out)) ++out;
		if (strlen(out) == 0) break;

		++sent_num;

		p = strchr(out, '\n');

		if (p) *p = '\0';
		if (option_id)
		{
			if (strlen(input_id) == 0) make_sent_id(sent_num);
			fprintf(option_ofp, "echo %s\n", input_id);
			strcpy(input_id, "");
		}
		fprintf(option_ofp, "sentence\n%s\n%s\n%s\n", out, option_compute, option_print);
		out = p ? p + 1 : NULL;
	}
	strcpy(input_id, "");
}

static int run_tokenizer(FILE *ofp)
{
	int	line;
	char	*out = NULL;

	if (strlen(input_file) == 0 || strcmp(input_file, "-") == 0)
	{
		input_fp = stdin;
	} else
	{
		input_fp = fopen(input_file, "r");
		if (input_fp == NULL)
		{
			fprintf(stderr, "Could not open file %s\n", input_file);
			return 0;
		}
	}

	char *save_text = new char[MAX_BUFF + 1];

	option_ofp = ofp;

	// The ``preamble''

	fprintf(option_ofp, "#%s/util/tagger\n", install_dir);

	if (option_silent) fprintf(option_ofp, "verbose 0\n");

	if (strlen(option_untag) > 0) fprintf(option_ofp, "untag %s\n", option_untag);

	fprintf(option_ofp, "ngrams %s/medpost%s.ngrams\n", install_dir, mptok->suf);
	fprintf(option_ofp, "lex 30 %s/medpost%s.lex\n", install_dir, mptok->suf);
	fprintf(option_ofp, "backoff\n");
	fprintf(option_ofp, "init 2\n");
	fprintf(option_ofp, "smooth\n");

	strcpy(input_pmid, "");
	strcpy(input_loc, "");
	strcpy(input_id, "");

	char	*text = new char[MAX_BUFF + 1];

	int	collect_text;

	collect_text = 0;

	// initialize

	mptok->init(install_dir);

	line = 0;
	while (input_fp && fgets(text, MAX_BUFF, input_fp))
	{

		line++;

		// Remove space (including newline) at the end of the string

		for (int i = (int) strlen(text) - 1; i >= 0 && isspace(text[i]); --i)
			text[i] = '\0';

#if 0
		fprintf(option_ofp, "\n%s\n\n", text);
#endif

		if (strcmp(option_input, "itame") == 0)
		{
			if (strncmp(text, ".I", 2) == 0)
			{
				strcpy(input_pmid, text + 2);
			} else if (option_titles && strncmp(text, ".T", 2) == 0)
			{
				strcpy(input_loc, "T");
				out = mptok->tokenize(text + 2);
			} else if (strncmp(text, ".A", 2) == 0)
			{
				strcpy(input_loc, "A");
				out = mptok->tokenize(text + 2);
			}
		} else if (strcmp(option_input, "xml") == 0)
		{
			char *s1;
			char *s2;

			if ((s1 = strstr(text, "<PMID>")) && (s2 = strstr(text, "</PMID>")) && s2 > s1)
			{
				*s2 = '\0';
				strcpy(input_pmid, s1 + 6);
			} else if (option_titles && (s1 = strstr(text, "<ArticleTitle>")) && (s2 = strstr(text, "</ArticleTitle>")) && s2 > s1)
			{
				strcpy(input_loc, "T");
				*s2 = '\0';
				out = mptok->tokenize(s1 + 14);
			} else if ((s1 = strstr(text, "<AbstractText>")) && (s2 = strstr(text, "</AbstractText>")) && s2 > s1)
			{
				strcpy(input_loc, "A");
				*s2 = '\0';
				out = mptok->tokenize(s1 + 14);
			}
		} else if (strcmp(option_input, "medline") == 0)
		{
			if (collect_text)
			{
				if (isspace(*text))
					strcat(save_text, text);
				else
				{
					out = mptok->tokenize(save_text);
					collect_text = 0;
				}
			}

			if (strncmp(text, "PMID", 4) == 0)
			{
				strcpy(input_pmid, skip(text + 4, "- "));
			} else if (option_titles && strncmp(text, "TI", 2) == 0)
			{
				strcpy(input_loc, "T");
				strcpy(save_text, skip(text + 2, "- "));
				collect_text = 1;
			} else if (strncmp(text, "AB", 2) == 0)
			{
				strcpy(input_loc, "A");
				strcpy(save_text, skip(text + 2, "- "));
				collect_text = 1;
			}

		} else if (strcmp(option_input, "text") == 0)
		{
			// If id is specified for text input
			// each line is preceded by an id

			if (option_textid && (line % 2))
			{
				strcpy(input_id, text);
			} else
			{
				out = mptok->tokenize(text);
			}
		} else if (strcmp(option_input, "token") == 0)
		{
			// If id is specified for text input
			// each line is preceded by an id

			if (option_textid && (line % 2))
			{
				strcpy(input_id, text);
			} else
			{
				out = mptok->tokenize_pre(text);
			}
		}

		if (out)
		{
			print_sentences(out);
			delete[] out;
			out = NULL;
		}

	}
	delete[] save_text;
}

main(int argc, char **argv)
{
	mptok = new MPtok;

	process_tokenizer_args(argc, argv);
	run_tokenizer(stdout);
}
