#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include <Btree.h>

#include "MPtag.h"
#include "MPlex.h"

#define OPTION_USE_CODES 1		// Whether to use code suffixes in the lexicon
static int option_verbose = 1;		// Echo commands if option_verbose == 1

static MPtag *mptag = NULL;		// The actual tagger object

// If sent is given, then go ahead and sent the file stream
// otherwise return 0 at end of file pointer

static FILE *file_stack[MAX_FILES];
static int num_files = 0;

static int command_input(char *file)
{
	if (option_verbose) printf("input %s\n", file);

	file_stack[num_files] = fopen(file, "r");
	if (file_stack[num_files] == NULL)
	{
		printf("Could not open %s for input\n", file);
		return 0;
	}
	++num_files;
	return 1;
}

static void chomp(char *line)
{
	int	i;

	i = strlen(line) - 1;
	while (i >= 0 && line[i] == '\n' || line[i] == '\r')
		line[i--] = '\0';
}


static int read_line(char *line, int sent, int blank_ok)
{
	do
	{
		if (num_files <= 0) return 0;
		strcpy(line, "");
		if (! fgets(line,MAX_LLEN,file_stack[num_files-1]))
		{
			if (num_files == 1) return 0;
			fclose(file_stack[num_files-1]);
			--num_files;
			if (sent == 1) return 0;
		}
		if (sent == 0 && line[0] == '$') printf("%s", line);
	} while ((blank_ok == 0 && strlen(line) <= 1) || (sent == 0 && line[0] == '#'));

	chomp(line);

	return 1;
}

#define GET_ARG(what) for (s = what; *line && isspace(*line) == 0; s++, line++) *s = *line; *s = '\0';
#define GET_REST(what) for (s = what; *line; s++, line++) *s = *line; *s = '\0';
#define SKIP_SPACE() while (isspace(*line)) ++line;

static void get_command(char *line, char *com, char *arg1, char *arg2)
{
	char	*s;
	*com = *arg1 = *arg2 = '\0';

	SKIP_SPACE();
	GET_ARG(com);
	SKIP_SPACE();

	GET_ARG(arg1);
	SKIP_SPACE();
	GET_ARG(arg2);
}

static int cmp(const void *a, const void *b)
{
	return (* (double *) a < * (double *) b) ? 1 : -1;
}

static void command_sentence(char *file)
{
	char	line[MAX_LLEN];

	if (strlen(file) > 0)
	{
		command_input(file);
		strcpy(line, "");

		// Gather lines together

		while (strlen(line) < 9000 && read_line(line+strlen(line), 1, 0))
		{
			strcat(line, " ");
		}

		mptag->load(line);
	} else
	{
		// Read a single line from the current input file to get a sentence

		if (read_line(line, 1, 0))
		{
			mptag->load(line);
		}
	}

}

main(int argc, char **argv)
{
	int	i, j, t;
	double	m;
	char	line[MAX_LLEN], com[MAX_LLEN], arg1[MAX_LLEN], arg2[MAX_LLEN];
	int	used = 0;
	char	*entry;

	srandom(time(NULL));

	file_stack[0] = stdin;
	num_files = 1;

	mptag = new MPtag;

// #define INTERNAL_TOKENIZE

	mptag->set_untag("UNTAGGED");

#ifdef INTERNAL_TOKENIZE

	extern int process_tokenizer_args(int argc, char **argv);
	extern int run_tokenizer(FILE *ofp);

	process_tokenizer_args(argc, argv);
	int fd[2];
	pipe(fd);
	FILE *ifp = fdopen(fd[0], "r");
	FILE *ofp = fdopen(fd[1], "w");
	run_tokenizer(ofp);
	fclose(ofp);
	file_stack[0] = ifp;
	num_files = 1;
#else

	for (i = 1; i < argc; ++i)
	{
		if (argv[i][0] != '-' && num_files == 1)
		{
			num_files = 0;
			command_input(argv[i]);
		}
	}
#endif

	// Read the input file

	while (read_line(line, 0, 0))
	{
		get_command(line, com, arg1, arg2);
		fflush(stdout);

		// Options affecting the program

		if (strcmp(com, "input") == 0)
		{
			command_input(arg1);
		} else if (strcmp(com, "echo") == 0)
		{
			printf("%s\n", line + 5);
		} else if (strcmp(com, "verbose") == 0)
		{
			option_verbose = atoi(arg1);
			if (option_verbose) printf("verbose %s\n", arg1);
		} else if (strcmp(com, "exit") == 0)
		{
			exit(0);

		// Set options within the tagger

		} else if (strcmp(com, "adhoc") == 0)
		{
			if (option_verbose) printf("adhoc %s\n", arg1);
			if (strcmp(arg1, "none") == 0) mptag->set_adhoc_none();
			else if (strcmp(arg1, "medpost") == 0) mptag->set_adhoc_medpost();
			else if (strcmp(arg1, "penn") == 0) mptag->set_adhoc_penn();
		} else if (strcmp(com, "untag") == 0)
		{
			mptag->set_untag(arg1);

		// Initialize state transition probabilities (ngrams)

		} else if (strcmp(com, "ngrams") == 0)
		{
			mptag->read_ngrams(arg1);
		} else if (strcmp(com, "init") == 0)
		{
			if (option_verbose) printf("init\n");
			mptag->norm_ngrams();
		} else if (strcmp(com, "smooth") == 0)
		{
			if (option_verbose) printf("smooth\n");
			mptag->smooth_ngrams();

		// Initialize the lexicon

		} else if (strcmp(com, "lex") == 0)
		{
			if (option_verbose) printf("lex %s %s\n", arg1, arg2);
			if (mptag->lex) delete mptag->lex;
			mptag->lex = new MPlex(mptag->num_tags, atoi(arg1), arg2, OPTION_USE_CODES);
			mptag->backoff(NULL);
		} else if (strcmp(com, "addlex") == 0)
		{
			if (option_verbose) printf("addlex %s\n", arg1);
			if (mptag->lex) mptag->lex->addfile(arg1);
		} else if (strcmp(com, "rmlex") == 0)
		{
			if (option_verbose) printf("rmlex %s\n", arg1);
			if (mptag->lex) mptag->lex->rmfile(arg1);
		} else if (strcmp(com, "addsmoothing") == 0)
		{
			if (option_verbose) printf("add %g\n", mptag->add_smoothing);
			mptag->add_smoothing = atof(arg1);
		} else if (strcmp(com, "backoff") == 0)
		{
			if (option_verbose) printf("backoff %s\n", arg1);
			mptag->add_smoothing = 0.0;
			mptag->backoff(arg1);

		// Load a sentence

		} else if (strcmp(com, "sentence") == 0)
		{
			command_sentence(arg1);		// this is mptag->load(tokenized-text)

		// Perform tagging

		} else if (strcmp(com, "compute") == 0)
		{
			mptag->compute();
		} else if (strcmp(com, "viterbi") == 0)
		{
			mptag->viterbi();
		} else if (strcmp(com, "baseline") == 0)
		{
			mptag->baseline();

		// Invoke the printing functions of the tagger

		} else if (strcmp(com, "print") == 0)
		{
			mptag->print(0);
		} else if (strcmp(com, "printfull") == 0)
		{
			mptag->print(1);
		} else if (strcmp(com, "printsent") == 0)
		{
			if (option_verbose) printf("printsent\n");
			mptag->print(2);
		}
	}
}

