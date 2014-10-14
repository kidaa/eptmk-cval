#ifndef _MPTAG_H
#define _MPTAG_H

#include "MPlex.h"
#include "MPtok.h"

// Maximum number of tags

#define MAX_TAGS 100

// Maximum length of a tag

#define MAX_TLEN 100

// Maximum number of words in a sentence

#define MAX_WORDS 1000

// Maximum number of files

#define MAX_FILES 100

// Maximum length of a line to read

#define MAX_LLEN 10000

// Number n of ngrams

#define NGRAMS 2

// A large enough size for most string buffers

#define BUF_SIZE 1000

// Type to use for floating point numbers

// #include "huge.h"
// #define RTYPE huge
#define RTYPE double

#if NGRAMS==2
#define MAX_STATES MAX_TAGS
#elif NGRAMS==3
#define MAX_STATES (MAX_TAGS*MAX_TAGS)
#elif NGRAMS==4
#define MAX_STATES (MAX_TAGS*MAX_TAGS*MAX_TAGS)
#endif

class MPtag : public MPtok
{
public:
	MPtag(char *cnam = "");
	~MPtag();

	void init();			// Initialize with defaults
	void init(char *idir);

	void load();			// load a (tagged) sentence
	void load(char *str);		// load a (new) sentence
	void merge_idioms();		// merge idioms in a loaded sentence
	void split_idioms();		// remove merged idioms (and their tags)

	void print(int);		// print the sentence

	void compute();			// forward-backward algorithms
	void compute(char *);		// forward-backward algorithms
	void viterbi();			// viterbi algorithm
	void viterbi(char *);		// viterbi algorithm
	void baseline();		// baseline algorithm
	void baseline(char *);		// baseline algorithm

	void normalize();		// Normalize dw
	void transform(char *);		// Apply Brill transformation to pr

	MPlex *lex;			// The lexicon
	double lex_backoff[MAX_TAGS];	// Backoff smoothing
	void backoff(char *e);		// Backoff to lexicon entry, or NULL
	double add_smoothing;		// Add smoothing

	int	num_tags;		// The number of tags

	char	sentence[MAX_LLEN];	// A buffer to hold the whole sentence

	char	*tag(int i);		// Return the tag

	double	dw[MAX_WORDS][MAX_TAGS];	// The d_w(t) for each word and tag
	double	pr[MAX_WORDS][MAX_TAGS];	// The current tag probability estimates
	double	count[MAX_WORDS];	// Count of occurences (only if whole word was in lexicon)

	int	comp_tag[MAX_WORDS];	// The computed tag
	int	idiom_flag[MAX_WORDS];	// The tag is part of an extended idiom

	RTYPE	p_sent;			// Probability of sentence

	RTYPE &alpha(int t, int i);	// Access routines
	RTYPE &beta(int t, int i);
	double &delta(int t, int i);
	int &psi(int t, int i);

	RTYPE *alpha_array;		// Alpha, beta, etc
	RTYPE *beta_array;
	double *delta_array;
	int *psi_array;

	double	zero;			// Zeros, etc
	int	z;

	double *count0;
	double *count1;
	double *count2;
	double *count3;
#if NGRAMS>=4
	double *count4;
#endif

	int num_states;			// The number of states
	int end_tag;			// The tag for end-of-sentence

	// The tags

	char tag_str[MAX_TAGS][MAX_TLEN];
	char save_tag[MAX_TLEN];	// A place to save a tag string
	double pr_tag[MAX_TAGS];
	double pr_state[MAX_STATES];
	double pr_trans[MAX_STATES][MAX_STATES];

	int numstringok(char *str);
	void tag_set(char *word, double *dw, char *tag, int cond);
	void tag_ok(char *word, double *dw, char *tag, int cond);

	void read_ngrams(char *);
	void norm_ngrams();
	void smooth_ngrams(void);

	char option_untag[1000];
	int option_adhoc;

	void set_adhoc_none();
	void set_adhoc_medpost();
	void set_adhoc_penn();
	void set_untag(char *);

	int tag_at(int, int);
	char *tag_to_str(int);
	int str_to_tag(char *);
	int state_next(int, int);
	int state_prev(int, int);

	void maxprob();			// internal function to find max prob tags
};

#endif

