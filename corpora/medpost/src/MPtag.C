#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include <Btree.h>

#include "MPtag.h"

#define c0 count0[0]
#define c1(i) count1[(i-1) % (num_tags)]
#define c2(i) count2[(i-1) % (num_tags*num_tags)]
#define c3(i) count3[(i-1) % (num_tags*num_tags*num_tags)]
#if NGRAMS>=4
#define c4(i) count4[(i-1) % (num_tags*num_tags*num_tags*num_tags)]
#endif

// Count the transitions from i to j by reducing to n-grams

#define t2(i,j) c2((i-1)*num_tags+((j-1)%num_tags)+1)
#define t3(i,j) c3((i-1)*num_tags+((j-1)%num_tags)+1)
#define t4(i,j) c4((i-1)*num_tags+((j-1)%num_tags)+1)

enum { ADHOC_NONE = 0, ADHOC_MEDPOST, ADHOC_PENN };

void MPtag::set_untag(char *u) { strcpy(option_untag, u); }
void MPtag::set_adhoc_none() { option_adhoc = ADHOC_NONE; }
void MPtag::set_adhoc_medpost() { option_adhoc = ADHOC_MEDPOST; }
void MPtag::set_adhoc_penn() { option_adhoc = ADHOC_PENN; }

static void chomp(char *line)
{
	int     i;

	i = strlen(line) - 1;
	while (i >= 0 && line[i] == '\n' || line[i] == '\r')
		line[i--] = '\0';
}


// Given a state i, return the tag at the position in the n-gram

int MPtag::tag_at(int i, int pos)
{
	i -= 1;
	while (++pos <= 0) i /= num_tags;
	return (i%num_tags);
}

char *MPtag::tag_to_str(int i)
{
	return tag_str[i];
}

int MPtag::str_to_tag(char *str)
{
	int	i;

	for (i = 0; i < num_tags; ++i)
		if (strcmp(str, tag_str[i]) == 0)
			return i;
	return -1;
}

int MPtag::state_next(int i, int s)
{
#if NGRAMS==2
	return s+1;
#elif NGRAMS==3
	return tag_at(i,0) * num_tags + s+1;
#elif NGRAMS>=4
	return tag_at(i,-1) * num_tags*num_tags + tag_at(i,0)*num_tags + s+1;
#endif
}

int MPtag::state_prev(int i, int s)
{
#if NGRAMS==2
	return s+1;
#elif NGRAMS==3
	return s*num_tags + tag_at(i,-1) + 1;
#elif NGRAMS>=4
	return s*num_tags*num_tags + tag_at(i,-2)*num_tags + tag_at(i,-1) + 1;
#endif
}

void MPtag::read_ngrams(char *file)
{
	FILE	*fp;
	char	line[BUF_SIZE];
	int	i, j, k;
	double	v;

	fp = fopen(file, "r");
	if (fp == NULL)
	{
		printf("could not open %s\n", file);
		exit(1);
	}

	fgets(line, BUF_SIZE, fp);
	num_tags = atoi(line);

	k = -1;
	i = j = 0;
	while (fgets(line, BUF_SIZE, fp))
	{
		chomp(line);

		v = atof(line);

		if (k == -1)
		{
			strcpy(tag_to_str(i), line);
			if (i-j >= num_tags-1)
			{
				j = i + 1;
				++k;
			}
		} else if (k == 0)
		{
			count0[0] = v;
			j = i+1;
			k = 1;
		} else if (k == 1)
		{
			count1[i-j] = v;
			if (v <= 0.0)
			{
				printf("Tag %s has probability 0.\n", tag_to_str(i-j));
				// exit(1);
			}
			if (i-j >= num_tags-1)
			{
				j = i+1;
				++k;
			}
		} else if (k == 2)
		{
#if 0
			printf("scangram(%s,%s) = count2[%d] = %g\n",
				tag_to_str(tag_at(i-j+1,-1)),
				tag_to_str(tag_at(i-j+1,0)),
				i-j, v);
#endif
			count2[i-j] = v;
			if (i-j >= num_tags*num_tags-1)
			{
				j = i+1;
				++k;
			}
		} else if (k == 3)
		{
			count3[i-j] = v;
			if (i-j >= num_tags*num_tags*num_tags-1)
			{
				j = i+1;
				++k;
			}
#if NGRAMS>=4
		} else if (k == 4)
		{
			count4[i-j] = v;
			if (i-j >= num_tags*num_tags*num_tags*num_tags-1)
			{
				j = i+1;
				++k;
			}
#endif
		}

		++i;
	}

	fclose(fp);

	end_tag = str_to_tag(".");

	for (i = 0; i < num_tags; ++i)
		pr_tag[i] = count1[i] / count0[0];
}

void MPtag::norm_ngrams()
{
	int	j;
	int	i = NGRAMS;

	if (num_tags == 0)
	{
		printf("ngrams must be read prior to init\n");
		exit(0);
	}

	if (i != NGRAMS)
	{
		printf("This version of the tagger was compiled for ``init %d''.\n", NGRAMS);
		i = NGRAMS;
	}

	if (i == 2) num_states = num_tags;
	else if (i == 3) num_states = num_tags * num_tags;
	else if (i == 4) num_states = num_tags * num_tags * num_tags;

	for (i = 1; i <= num_states; ++i)
	{
#if NGRAMS==2
		pr_state[i-1] = (c0 > 0.0) ? c1(i) / c0 : 1.0 / (double) num_states;
#elif NGRAMS==3
		pr_state[i-1] = (c0 > 0.0) ? c2(i) / c0 : 1.0 / (double) num_states;
#elif NGRAMS==4
		pr_state[i-1] = (c0 > 0.0) ? c3(i) / c0 : 1.0 / (double) num_states;
#endif

		for (j = 1; j <= num_states; ++j)
		{
			pr_trans[i-1][j-1] = 0.0;
#if NGRAMS==2
			pr_trans[i-1][j-1] = (c1(i) > 0.0) ? t2(i,j) / c1(i) : 1.0 / (double) num_tags;
#elif NGRAMS==3
			if (tag_at(i,0) == tag_at(j,-1))
				pr_trans[i-1][j-1] = (c2(i) > 0.0) ? t3(i,j) / c2(i) : 1.0 / (double) num_tags;
#elif NGRAMS==4
			if (tag_at(i,0) == tag_at(j,-1) && tag_at(i,-1) == tag_at(j,-2))
				pr_trans[i-1][j-1] = (c3(i) > 0.0) ? t4(i,j) / c3(i) : 1.0 / (double) num_tags;
#endif
		}
	}
}


// Smooth the ngrams

void MPtag::smooth_ngrams()
{

	// Compute discounted probabilities, then backoff

	double *p1 = new double[MAX_TAGS];
	double *p2 = new double[MAX_TAGS*MAX_TAGS];
	double *p3 = new double[MAX_TAGS*MAX_TAGS*MAX_TAGS];

	double	a, b, d;
	double	N, T, Z;

	int	i, j, k, w;

	double p, m;

	N = T = Z = 0;
	for (i = 0; i < num_tags; ++i)
	{
		N += count1[i];
		if (count1[i] > 0) T++; else Z++;
	}
	d = N / (N + T);

	// The discounted probability for the 1-grams is not backed-off

	for (i = 0; i < num_tags; ++i)
		p1[i] = (count1[i] > 0) ? d * count1[i] / count0[0] : (1 - d) / Z;


	// The 2-gram probabilities (i -> j)

	for (i = 0; i < num_tags; ++i)
	{

		N = T = Z = 0;
		for (j = 0; j < num_tags; ++j)
		{
			k = i * num_tags + j;
			N += count2[k];
			if (count2[k] > 0) T++; else Z++;
		}
		d = N > 0 ? N / (N + T) : 0.0;

#if 0
		printf("Discount factor for %s = %g\n", tag_to_str(i), d);
#endif

		a = b = 0.0;
		for (j = 0; j < num_tags; ++j)
		{
			k = i * num_tags + j;

			// Use the discounted probability, if possible, otherwise use
			// the backoff probability. This will be adjusted afterwards to
			// get a probability

			p2[k] = (count2[k] > 0) ? d * count2[k] / count1[i] : p1[j];

			if (count2[k] > 0) a += p2[k]; else b += p2[k];

#if 0
			printf("p2(%s->%s) = %g, count2 = %g\n", tag_to_str(i), tag_to_str(j), p2[k], count2[k]);
#endif
		}

		a = (1.0 - a) / b;
		for (j = 0; j < num_tags; ++j)
		{
			k = i * num_tags + j;

			if (count2[k] == 0) p2[k] *= a;

#if 0
			printf("p2(%s->%s) = %g, count2 = %g\n", tag_to_str(i), tag_to_str(j), p2[k], count2[k]);
#endif
		}
	}

#if NGRAMS > 2
	// The 3-gram probabilities

	for (i = 0; i < num_tags*num_tags; ++i)
	{

		w = i % num_tags;

		N = T = Z = 0;
		for (j = 0; j < num_tags; ++j)
		{
			k = i * num_tags + j;
			N += count3[k];
			if (count3[k] > 0) T++; else Z++;
		}
		d = N > 0 ? N / (N + T) : 0.0;

		a = b = 0.0;
		for (j = 0; j < num_tags; ++j)
		{
			k = i * num_tags + j;

			// Use the discounted probability, if possible, otherwise use
			// the backoff probability. This will be adjusted afterwards to
			// get a probability

			p3[k] = (count3[k] > 0) ? d * count3[k] / count2[i] : p2[w * num_tags + j];

			if (count3[k] > 0) a += p3[k]; else b += p3[k];
		}

		a = (1.0 - a) / b;
		for (j = 0; j < num_tags; ++j)
		{
			k = i * num_tags + j;

			if (count3[k] == 0) p3[k] *= a;
		}
	}
#endif

	m = 0.0;
	for (i = 0; i < num_states; ++i)
	{

#if NGRAMS==2
		p = p1[i];
#elif NGRAMS==3
		if (end_tag == tag_at(i+1,-1))
			p = p2[end_tag * num_tags + tag_at(i+1,0)];
#endif

		if (fabs(p - pr_state[i]) > m)
			m = fabs(p - pr_state[i]);

		pr_state[i] = p;

		for (j = 0; j < num_states; ++j)
		{

#if NGRAMS==2
			p = p2[i * num_tags + j];
#elif NGRAMS==3
			if (tag_at(i+1,0) == tag_at(j+1,-1))
				p = p3[i * num_tags + (j % num_tags)];
			else
				p = 0.0;
#endif

#if 0
			if (p != pr_trans[i][j])
			{
				printf("pr(%s->%s) was %g now %g\n", tag_to_str(i), tag_to_str(j), pr_trans[i][j], p);
			} else
			{
				printf("pr(%s->%s) = %g unch\n", tag_to_str(i), tag_to_str(j), pr_trans[i][j]);
			}
#endif

			if (fabs(p - pr_trans[i][j]) > m)
				m = fabs(p - pr_trans[i][j]);

			pr_trans[i][j] = p;
		}
	}

#if 0
	printf("Maximum absolute difference %g\n", m);
#endif

	delete [] p1;
	delete [] p2;
	delete [] p3;
}


// This is the tagger code

MPtag::MPtag(char *cnam) : MPtok(cnam)
{
	alpha_array = NULL;
	beta_array = NULL;
	delta_array = NULL;
	psi_array = NULL;
	num_words = 0;
	num_tags = 0;

	add_smoothing = 0.0;
	for (int i = 0; i < MAX_TAGS; ++i)
		lex_backoff[i] = 1.0;

	zero = 0.0;
	z = 0;

	count0 = new double;
	count1 = new double[MAX_TAGS];
	count2 = new double[MAX_TAGS * MAX_TAGS];
	count3 = new double[MAX_TAGS * MAX_TAGS * MAX_TAGS];
#if NGRAMS>=4
	count4 = new double[MAX_TAGS * MAX_TAGS * MAX_TAGS * MAX_TAGS];
#endif

	num_states = 0;
	end_tag = 0;

	option_adhoc = ADHOC_MEDPOST;
}

void MPtag::init(void)
{
	char	*idir = getenv("MEDPOST_HOME");
	char	buff[1000];

	if (idir)
	{
		idir = strchr(idir, '=');
		if (idir) idir++;
	}

	if (idir == NULL || strlen(idir) == 0)
	{
		FILE *fp = fopen("path_medpost", "r");
		if (fp)
		{
			if (fgets(buff, 1000, fp))
			{
				chomp(buff);
				idir = &buff[0];
			}
			fclose(fp);
		}
	}

	if (idir == NULL || strlen(idir) == 0)
		idir = "/net/coleman/vol/export3/IRET/CPP/lib/FIXED_DATA/";

	init(idir);
}

void MPtag::init(char *idir)
{
	char	fname[1000];

	sprintf(fname, "%s/medpost%s.ngrams", idir, suf);
	read_ngrams(fname);
	norm_ngrams();
	smooth_ngrams();

	sprintf(fname, "%s/medpost%s.lex", idir, suf);
	lex = new MPlex(num_tags, 30, fname, 1);
	backoff("");

	MPtok::init(idir);
}

void MPtag::tag_ok(char *word, double *dw, char *tag, int cond)
{
	int	i;
	double	m;

	if (str_to_tag(tag) < 0) return;

	if (! cond)
	{
		dw[str_to_tag(tag)] = 0.0;
		m = 0.0;
		for (i = 0; i < num_tags; ++i)
			m += dw[i];

		// m could be 0 here

		for (i = 0; i < num_tags; ++i)
			dw[i] /= m;
	}
}

// Reset the tag and normalize

void MPtag::tag_set(char *word, double *dw, char *tag, int cond)
{
	int	i;
	double	m;

	if (str_to_tag(tag) < 0) return;

	if (cond)
	{
		// printf("Forcing %s to %s\n", word, tag);
		for (i = 0; i < num_tags; ++i)
			dw[i] = 0.0;
		dw[str_to_tag(tag)] = 1.0;
	} else
	{
		dw[str_to_tag(tag)] = 0.0;
		m = 0.0;
		for (i = 0; i < num_tags; ++i)
			m += dw[i];

		// m could be 0 here

		for (i = 0; i < num_tags; ++i)
			dw[i] /= m;
	}
}

static char *spelled_numbers[] = {
"first", "second", "third", "fourth", "fifth", "sixth", "seventh", "eighth", "ninth", "tenth",
"one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten",
"twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety", "hundred",
"1st", "2nd", "3rd", "4th", "5th", "6th", "7th", "8th", "9th", "0th",
NULL };

enum {NUMBER_NOTOK=0, NUMBER_OK, NUMBER_DEFINITE, NUMBER_HYPHEN};

int MPtag::numstringok(char *str)
{
	int	i, n, d, h;
	char	buff[MAX_LLEN];

	strcpy(buff, str);
	for (i = 0; i < strlen(buff); ++i) buff[i] = tolower(buff[i]);

	n = d = h = 0;
	for (i = 0; i < strlen(buff); ++i)
	{
		if (strchr("0123456789", buff[i]))
			++d;
		else if ((i == 0 && strchr("=+-", buff[i])) || strchr(",.:", buff[i]))
			++n;
		else if (i > 0 && (buff[i] == '-' || buff[i] == '+'))
			++h;
	}

	// This is a number

	if (d > 0 && d + n == strlen(buff))
		return NUMBER_DEFINITE;

	// This is a hyphenated number

	if (d > 0 && d + n + h == strlen(buff))
		return NUMBER_HYPHEN;

	for (i = 0; spelled_numbers[i]; ++i)
	{
		if (strncmp(buff, spelled_numbers[i], strlen(spelled_numbers[i])) == 0)
		// || (strlen(spelled_numbers[i]) >= strlen(buff) && strcmp(buff - strlen(spelled_numbers[i]), spelled_numbers[i]) == 0))
		{
			// printf("%s could be a spelled number (%d=%s, length %d)\n", buff, i, spelled_numbers[i], strlen(spelled_numbers[i]));
			return NUMBER_OK;
		}
	}

	return NUMBER_NOTOK;
}

// Check the current word store for idioms in the lexicon

#define MAX_IDIOM 4

void MPtag::merge_idioms()
{
	int	n;
	char	*ptr;

	// Next idiom

	for (int i = 0; i < num_words - 1; i++)
	{
		idiom_flag[i] = 0;

		// Start by putting words together

		for (n = 0; n < MAX_IDIOM && i + n + 1 < num_words; n++)
		{
			ptr = word(i) + strlen(word(i));
			*ptr = ' ';
		}

		if (n < 1) break;

		while (ptr = strrchr(word(i), ' '))
		{
			if (lex->exists(word(i)))
			{
				// printf("idiom: '%s'\n", word(i));
				// Fixup the remaining array

				for (int k = i + 1; k + n < num_words; k++)
					word_ptr[k] = word_ptr[k+n];

				num_words -= n;
				break;
			}
			*ptr = '\0';
			--n;

		}
	}
}

// Put back idioms

void MPtag::split_idioms()
{
	int	i, j;
	char	*p;

	for (i = 0; i < num_words; i++)
	{
		// If there is an idiom

		if (p = strchr(word(i), ' '))
		{
			// Move all the words and tags down

			for (j = num_words - 1; j >= i; j--)
			{
				word_ptr[j+1] = word_ptr[j];
				comp_tag[j+1] = comp_tag[j];
			}
			num_words++;

			// Null terminate this word

			*p = '\0';

			// Point to the next word

			word_ptr[i+1] = p + 1;

			// Set the idiom flag

			idiom_flag[i] = 1;
		}
	}
}

void MPtag::load(char *str)
{
	tokenize_presave(str);
	load();
}

void MPtag::load()
{
	merge_idioms();

	int	i, j, t;
	char	*s;
	char	*entry;
	char	buff[MAX_LLEN+1];

	// Initialize memory

	if (alpha_array) { delete[] alpha_array; } alpha_array = new RTYPE[(num_words+1)*MAX_STATES];
	if (beta_array) { delete[] beta_array; } beta_array = new RTYPE[(num_words+1)*MAX_STATES];
	if (delta_array) { delete[] delta_array; } delta_array = new double[(num_words+1)*MAX_STATES];
	if (psi_array) { delete[] psi_array; } psi_array = new int[(num_words+1)*MAX_STATES];

	if (alpha_array == NULL || beta_array == NULL
	|| delta_array == NULL || psi_array == NULL)
	{
		printf("Could not allocate sentence (%d)\n", num_words);
		exit(1);
	}

	// Initialize tags and flags

	for (i = 0; i <= num_words; i++)
	{
		comp_tag[i] = -1;
		idiom_flag[i] = 0;
	}

	// Set the prior probabilities from the lexicon for each word

	for (i = 0; i < num_words; i++)
	{
		strncpy(buff, word(i), MAX_LLEN);
		buff[MAX_LLEN] = '\0';

		// Lower case the first letter only if the remaining letters are all lower case

		if (i == 0)
		{
			for (j = 1; j < strlen(buff); ++j)
				if (! islower(buff[j]))
					break;
			if (j == strlen(buff))
				buff[0] = tolower(buff[0]);
		}

		s = &buff[0];

		if (s == NULL)
		{
			strcpy(buff, "");
			s = &buff[0];
		}

		entry = lex->get(s);
		count[i] = lex->count(s);

		for (t = 0; t < num_tags; ++t)
		{
			dw[i][t] = lex->scan_lex(entry, tag_to_str(t));
#if 0
			printf("%s (%s) %s%s:%g\n", word(i), s, sTAGSEP, tag_to_str(t), dw[i][t]);
#endif
		}
	}

	// Normalize the probabilities

	normalize();

	// Check post-lexicon constraints

	if (option_adhoc != ADHOC_NONE)
	{
		char	essential[MAX_LLEN];
		int	ok, flag;

		for (i = 0; i < num_words; i++)
		{
			strncpy(buff, word(i), MAX_LLEN);
			buff[MAX_LLEN] = '\0';

			if (count[i] > 999.0) continue;	// This means the lexicon entry is definite.

			tag_set(buff, dw[i], "$", strcmp(buff, "$") == 0);
			tag_set(buff, dw[i], "''", strcmp(buff, "'") == 0 || strcmp(buff, "''") == 0);
			tag_set(buff, dw[i], "(", strcmp(buff, "(") == 0 || strcmp(buff, "[") == 0 || strcmp(buff, "{") == 0);
			tag_set(buff, dw[i], ")", strcmp(buff, ")") == 0 || strcmp(buff, "]") == 0 || strcmp(buff, "}") == 0);
			tag_set(buff, dw[i], ",", strcmp(buff, ",") == 0);
			tag_set(buff, dw[i], ".", strcmp(buff, ".") == 0 || strcmp(buff, "!") == 0 || strcmp(buff, "?") == 0);
			tag_set(buff, dw[i], ":", strcmp(buff, "-") == 0 || strcmp(buff, "--") == 0 || strcmp(buff, ":") == 0 || strcmp(buff, ";") == 0);
			tag_set(buff, dw[i], "``", strcmp(buff, "`") == 0 || strcmp(buff, "``") == 0);


			// Numbers are easily recognized,
			// Verbs cannot be hyphenated, this takes care of hyphenated participles
			// which must be tagged as JJ
			// These are the only tags with constraints (so far) that are different
			// in the MedPost and Penn treebank tag set

			if (option_adhoc == ADHOC_MEDPOST)
			{
				switch (numstringok(buff)) {
				case NUMBER_DEFINITE: tag_set(buff, dw[i], "MC", 1); break;
				case NUMBER_OK: tag_ok(buff, dw[i], "MC", 1); break;
				case NUMBER_HYPHEN: tag_set(buff, dw[i], "MC", 1); break;
				default: tag_ok(buff, dw[i], "MC", 0); break;
				}

				tag_ok(buff, dw[i], "VVB", strchr(buff, '-') == NULL);
				tag_ok(buff, dw[i], "VVD", strchr(buff, '-') == NULL);
				tag_ok(buff, dw[i], "VVG", strchr(buff, '-') == NULL);
				tag_ok(buff, dw[i], "VVI", strchr(buff, '-') == NULL);
				tag_ok(buff, dw[i], "VVN", strchr(buff, '-') == NULL);
				tag_ok(buff, dw[i], "VVZ", strchr(buff, '-') == NULL);
			} else if (option_adhoc == ADHOC_PENN)
			{
				switch (numstringok(buff)) {
				case NUMBER_DEFINITE: tag_set(buff, dw[i], "CD", 1); break;
				case NUMBER_OK: tag_ok(buff, dw[i], "CD", 1); break;
				case NUMBER_HYPHEN: tag_set(buff, dw[i], "CD", 1); break;
				default: tag_ok(buff, dw[i], "CD", 0); break;
				}

				tag_ok(buff, dw[i], "VBP", strchr(buff, '-') == NULL);
				tag_ok(buff, dw[i], "VBD", strchr(buff, '-') == NULL);
				tag_ok(buff, dw[i], "VBG", strchr(buff, '-') == NULL);
				tag_ok(buff, dw[i], "VB", strchr(buff, '-') == NULL);
				tag_ok(buff, dw[i], "VBN", strchr(buff, '-') == NULL);
				tag_ok(buff, dw[i], "VBZ", strchr(buff, '-') == NULL);
			}

			// If the word is hyphenated, look at the last word, call this
			// the essential word

			s = strrchr(buff, '-');
			if (s == NULL || strlen(s) < 3) s = &buff[0]; else ++s;
			strcpy(essential, s);

			// If the essential word has at least one letter, and has an embedded cap
			// or number, make it an NN or NNS. NNP is possible but we accept this error.

			ok = 0;
			flag = 0;
			for (j = 0; j < strlen(essential); ++j)
			{
				if ((j > 0 && isupper(essential[j])) || isdigit(essential[j])) flag = 1;
				if (isalpha(essential[j])) ok = 1;
			}
			if (flag == 1 && j > 0 && essential[j - 1] == 's') flag = 2;
			if (ok == 1 && flag == 1) tag_set(buff, dw[i], "NN", 1);
			if (ok == 1 && flag == 2) tag_set(buff, dw[i], "NNS", 1);

			// Require any NNP to be capitalized

			tag_ok(buff, dw[i], "NNP", isupper(essential[0]));

#if 0
			for (t = 0; t < num_tags; ++t)
				printf("%s%s%s:%g after constraints\n", buff, sTAGSEP, tag_to_str(t), dw[i][t]);
#endif

		}
	}

	// Initialize the tag probabilities

	for (i = 0; i < num_words; i++)
		for (t = 0; t < num_tags; ++t)
			pr[i][t] = dw[i][t];

}

MPtag::~MPtag()
{
	if (alpha_array) delete[] alpha_array;
	if (beta_array) delete[] beta_array;
	if (delta_array) delete[] delta_array;
	if (psi_array) delete[] psi_array;
}

void MPtag::backoff(char *e)
{
	if (e) e = lex->get(e);
	for (int t = 0; t < num_tags; ++t)
		lex_backoff[t] = e ? lex->scan_lex(e, tag_to_str(t)) : 1.0;
}

RTYPE &MPtag::alpha(int t, int i)
{
	if (i < 1 || i > num_states) return (RTYPE&) zero = 0.0;
	return alpha_array[(t-1) * num_states + (i-1)];
}

RTYPE &MPtag::beta(int t, int i)
{
	if (i < 1 || i > num_states) return (RTYPE&) zero = 0.0;
	return beta_array[(t-1) * num_states + (i-1)];
}

double &MPtag::delta(int t, int i)
{
	if (i < 1 || i > num_states) return zero = 0.0;
	return delta_array[(t-1) * num_states + (i-1)];
}

int &MPtag::psi(int t, int i)
{
	if (i < 1 || i > num_states) return z = 0;
	return psi_array[(t-1) * num_states + (i-1)];
}

static int count_words(char *s)
{
	int	i;

	i = 1;
	for (; *s; ++s)
	{
		if (*s == ' ') ++i;
	}
	return i;
}

static void print_word(char *s, int i)
{
	for (; i > 0 && *s; ++s) { if (*s == ' ') --i; }
	while (*s && *s != ' ') { printf("%c", *s); ++s; }
}

void MPtag::print(int how)
{
	int	i, j, w;
	double	v;

	if (how == 1)
		printf("sentence log10 probability %g\n", p_sent);
	for (i = 0; i < num_words; ++i)
	{
		// Get the words from an idiom

		for (w = 0; w < count_words(word(i)); ++w)
		{
			if (how == 2 && i + w > 0)
				printf(" ");

			print_word(word(i), w);

			if (how == 0)
			{
				if (comp_tag[i] < 0)
					printf(" untagged", tag_to_str(comp_tag[i]));
				else
				{
					printf(" tagged %s", tag_to_str(comp_tag[i]));
					if (w < count_words(word(i)) - 1) printf("+");
				}
				printf("\n");
			} else if (how == 1)
			{
				for (j = 0; j < num_tags; ++j)
				{
					v = floor(0.5 + 100.0 * pr[i][j]) / 100.0;
					if (v > 0)
					{
						printf("%s%s", sTAGSEP, tag_to_str(j));
						if (w < count_words(word(i)) - 1) printf("+");
						printf(":%g", v);
					}
				}
				printf("\n");
			} else if (how == 2)
			{
				if (comp_tag[i] < 0)
					printf("%s%s", sTAGSEP, option_untag);
				else
				{
					printf("%s%s", sTAGSEP, tag_to_str(comp_tag[i]));
					if (w < count_words(word(i)) - 1) printf("+");
				}
			}
		}
	}
	if (how == 2)
		printf("\n");
}

// This is where smoothing should be performed!
// To this end, the sentence should retrieve default
// back-off probabilities

void MPtag::normalize()
{
	int	i, t;
	double	T, N, Z, d, sum;
	int	known;

	for (i = 0; i < num_words; ++i)
	{
		N = T = Z = 0.0;
		for (t = 0; t < num_tags; ++t)
		{
			N += dw[i][t];
			if (dw[i][t] > 0.0) T++; else Z += lex_backoff[t];
		}

		d = N / (N + T);

		// If the word was seen enough (or if it was manually tagged)
		// then don't back off, but average with an insignificant amount
		// of the backoff in case it is needed to break ties.

#define INSIGNIF (10e-10)

		known = (count[i] > 999.0 || N > 999.0);

		if (known) d = 1.0 - INSIGNIF;

		// Witten-Bell smoothing with uniform backoff priors

		sum = 0.0;
		for (t = 0; t < num_tags; ++t)
		{
#if 0
			if (dw[i][t] > 0) printf("%s %s%s:%g before\n", word(i), sTAGSEP, tag_to_str(t), dw[i][t]);
#endif

			if (add_smoothing != 0.0)
			{
				if (add_smoothing > 0.0)
					dw[i][t] = (dw[i][t] + add_smoothing) / (N + add_smoothing * (double) num_tags);
			} else if (known)
			{
				if (N > 0.0 && dw[i][t] > 0.0)
					dw[i][t] = d * dw[i][t] / N;
				else
					dw[i][t] = 0.0;
				if (Z > 0.0 && dw[i][t] > 0.0)
					dw[i][t] += (1.0 - d) * lex_backoff[t] / Z;
			} else
			{
				if (N > 0.0 && dw[i][t] > 0.0) dw[i][t] = d * dw[i][t] / N;
				else if (Z > 0.0) dw[i][t] = (1.0 - d) * lex_backoff[t] / Z;
				else dw[i][t] = 0.0;
			}
			sum += dw[i][t];
#if 0
			if (dw[i][t] > 0) printf("%s %s%s:%g after\n", word(i), sTAGSEP, tag_to_str(t), dw[i][t]);
#endif
		}

		if (sum == 0.0)
		{
			printf("Cannot continue because word '%s' has no possible tags.\n", word(i));
			exit(1);
		}
	}
}

void MPtag::compute(char *str)
{
	tokenize_save(str);
	load();
	compute();
}

void MPtag::compute()
{
	int	r, i, j, k;
	double	m, v;
	double	scale;

	// Compute alpha

	p_sent = 0.0;
	for (r = 1; r <= num_words; ++r)
	{
		// printf("computing alpha(%d/%d)\n", r, num_words);
		scale = 0.0;

		for (i = 1; i <= num_states; ++i)
		{
			alpha(r, i) = 0.0;

			if (dw[r-1][tag_at(i,0)] <= 0.0) continue;

			if (r == 1)
			{
				// if (tag_at(i,-1) == end_tag)
					alpha(r, i) = pr_state[i-1] * dw[r-1][tag_at(i,0)] / pr_tag[tag_at(i,0)];
			} else
			{
				for (k = 0; k < num_tags; ++k)
				{
					j = state_prev(i, k);
					alpha(r,i) += alpha(r-1,j)
						* (RTYPE) (pr_trans[j-1][i-1] * dw[r-1][tag_at(i,0)] / pr_tag[tag_at(i,0)]);
#if 0
// if (alpha(r-1,j) > 0.0)
	printf("(%s %s) -> (%s %s), a=%g, dw=%g\n",
		tag_to_str(tag_at(j,-1)),
		tag_to_str(tag_at(j,0)),
		tag_to_str(tag_at(i,-1)),
		tag_to_str(tag_at(i,0)),
		pr_trans[j-1][i-1], dw[r-1][tag_at(i,0)]);
#endif
				}
			}
#if 0
// if (alpha(r,i) > 0.0)
	printf("alpha(%d,%d) = %g (%s%s%s)\n", r, i, log10(alpha(r,i)), word(r-1), sTAGSEP, tag_to_str(tag_at(i,0)));
#endif

			scale += alpha(r, i);
		}

		for (i = 1; i <= num_states; i++)
			alpha(r, i) /= scale;

		p_sent += log10(scale);
	}

#if 0
printf("p_sent = %g, num_states = %d\n", p_sent, num_states);
#endif

	// Compute beta

	for (r = num_words; r >= 1; --r)
	{
		scale = 0.0;
		for (i = 1; i <= num_states; ++i)
		{
			beta(r, i) = 0.0;

			if (r == num_words)
				beta(r, i) = 1.0;
			else
			{
				for (k = 0; k < num_tags; ++k)
				{
					j = state_next(i, k);
					beta(r,i) += ((RTYPE) (pr_trans[i-1][j-1] * dw[r][tag_at(j,0)] / pr_tag[tag_at(j,0)]))
						* beta(r+1, j);
				}
			}

			scale += beta(r, i);
		}

		for (i = 1; i <= num_states; i++)
			beta(r, i) /= scale;
	}

	// Compute maximum likelihood for each tag, and normalize!

	for (r = 1; r <= num_words; ++r)
	{
		for (i = 0; i < num_tags; ++i)
			pr[r-1][i] = 0.0;

		v = 0.0;
		for (i = 1; i <= num_states; ++i)
		{
			pr[r-1][tag_at(i,0)] += alpha(r,i)*beta(r,i);
			v += pr[r-1][tag_at(i,0)];
		}

		for (i = 0; i < num_tags; ++i)
			pr[r-1][i] /= v;
	}

	maxprob();

#if 0
	printf("Log probability of sentence: %g (prob %g)\n", log10(p_sent), p_sent);
	for (r = 0; r < num_words; ++r)
		printf("%s%s%s\n", word(r), sTAGSEP, tag_to_str(comp_tag[r]));
#endif
}

// #define DEBUG

#define VSCALE

void MPtag::viterbi(char *str)
{
	tokenize_save(str);
	load();
	viterbi();
}

void MPtag::viterbi()
{
	int	i, j, k, r, p;
	double	m, max_delta;
#ifdef VSCALE
	double scale = 0.0;
#endif

	r = 1;
	for (i = 1; i <= num_states; ++i)
	{
#ifdef VSCALE
		delta(r, i) = pr_state[i-1] * dw[r-1][tag_at(i,0)] / pr_tag[tag_at(i,0)];
		if (delta(r, i) > scale)
		{
			scale = delta(r, i);
		}
#else
		delta(r, i) = log10(pr_state[i-1] * dw[r-1][tag_at(i,0)] / pr_tag[tag_at(i,0)]);
#endif
		psi(r, i) = 0;

#if 0
		printf("delta(%d,%s) = %g\n", r, tag_to_str(tag_at(i,0)), delta(r,i));
		printf("pr_state(%s) = %g\n", tag_to_str(tag_at(i,0)), pr_state[i-1]);
		printf("dw[%s][%s] = %g\n", word(r-1), tag_to_str(tag_at(i,0)), dw[r-1][tag_at(i,0)]);
		printf("count1[%s] = %g\n", tag_to_str(tag_at(i,0)), count1[tag_at(i,0)]);
#endif
	}

#ifdef VSCALE
	if (scale > 0.0)
		for (i = 1; i <= num_states; ++i)
			delta(r, i) /= scale;
#endif

	for (r = 2; r <= num_words; ++r)
	{
		// printf("viterbi(delta(%d,*))\n", r);

#ifdef VSCALE
		scale = 0.0;
#endif

		for (j = 1; j <= num_states; ++j)
		{
#ifdef VSCALE
			max_delta = 0.0;
#else
			max_delta = -1.0e100;
#endif
			p = -1;

			for (k = 0; k < num_tags; ++k)
			{
				i = state_prev(j, k);

#ifdef VSCALE
				m = delta(r-1,i) * pr_trans[i-1][j-1] * dw[r-1][tag_at(j,0)] / pr_tag[tag_at(j,0)];
#else
				m = delta(r-1,i) + log10(pr_trans[i-1][j-1] * dw[r-1][tag_at(j,0)] / pr_tag[tag_at(j,0)]);
#endif
				if (m > max_delta)
				{
					max_delta = m;
					p = i;
				}
			}
			if (p < 1) p = 1;

			delta(r, j) = max_delta;
			psi(r, j) = p;

#ifdef VSCALE
			if (delta(r, j) > scale)
				scale = delta(r, j);
#endif
		}

#ifdef VSCALE
		if (scale > 0.0)
			for (j = 1; j <= num_states; ++j)
				delta(r, j) /= scale;
#endif
	}

	max_delta = delta(num_words, 1);
	p = 1;
	for (i = 1; i <= num_states; ++i)
	{
		m = delta(num_words, i);
		if (m > max_delta)
		{
			max_delta = m;
			p = i;
		}
	}
	comp_tag[num_words-1] = tag_at(p, 0);
	for (r = num_words-1; r > 0; --r)
	{
		p = psi(r+1, p);
		comp_tag[r-1] = tag_at(p, 0);
	}

#if 0
	for (r = 0; r < num_words; ++r)
		printf("%s%s%s\n", word(r), sTAGSEP, tag_to_str(comp_tag[r]));
#endif
}

void MPtag::baseline(char *str)
{
	tokenize_save(str);
	load();
	baseline();
}

void MPtag::baseline()
{
	int	i, j;

	for (i = 0; i < num_words; ++i)
		for (j = 0; j < num_tags; ++j)
			pr[i][j] = dw[i][j];
	maxprob();
}

// Compute the maximum likelihood tag based on whatever is in
// the pr field.

void MPtag::maxprob()
{
	int	i, j;
	double	m;

	for (i = 0; i < num_words; ++i)
	{
		m = 0.0;
		for (j = 0; j < num_tags; ++j)
		{
			if (pr[i][j] > m)
			{
				m = pr[i][j];
				comp_tag[i] = j;
			}
		}
	}
}

char *MPtag::tag(int i)
{
	if (i >= num_words) return "";
	if (comp_tag[i] < 0 || comp_tag[i] >= num_tags) return "";
	strcpy(save_tag, tag_to_str(comp_tag[i]));
	if (idiom_flag[i]) strcat(save_tag, "+");
	return &save_tag[0];
}

