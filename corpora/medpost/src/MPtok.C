/* tokenizer.C
**
** This function performs tokenization of raw input to be read by a tagger.
*/

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "MPtok.h"

// These options are probably compile time constants

static char option_tagsep = '_';	// The tagsep character
static char option_replacesep = '-';	// Replace tagsep with this

static void chomp(char *line)
{
	int     i;

	i = strlen(line) - 1;
	while (i >= 0 && line[i] == '\n' || line[i] == '\r')
		line[i--] = '\0';
}

// Data structure and algorithm for finding common pairs.

// read a file of pairs into a data structure,
// the file must be sorted first

void MPtok::init_common_pairs(char *file_name, int max_pairs)
{
	FILE *fp;
	char	buff[1000];

	common_pair = NULL;
	num_common_pairs = 0;

	fp = fopen(file_name, "r");
	if (fp == NULL)
	{
		fprintf(stderr, "Could not open file of common pairs: %s\n", file_name);
		return;
	}

	common_pair = new char*[max_pairs];

	while (fgets(buff, 1000, fp))
	{
		if (strlen(buff) <= 1) continue;
		if (buff[strlen(buff)-1] == '\n') buff[strlen(buff)-1] = '\0';
		common_pair[num_common_pairs] = new char[strlen(buff) + 1];
		strcpy(common_pair[num_common_pairs], buff);
		num_common_pairs++;
		if (num_common_pairs >= max_pairs) break;
	}
	fclose(fp);
}

int MPtok::find_common_pair(const char *text, int offs, int *tokflag)
{
	char	buff[MAX_PAIR_LEN];
	int	a, b, c;
	int	i, j, cmp;

	if (common_pair == NULL || num_common_pairs == 0) return 0;

	if (tokflag[offs] == 0 || isalnum(text[offs]) == 0) return 0;

	int num_toks = 0;
	for (i = j = 0; i < MAX_PAIR_LEN && offs + i < strlen(text); i++)
	{
		if (isspace(text[offs + i])) continue;

		if (tokflag[offs + i])
		{
			if (num_toks == 0) num_toks++;
			else if (num_toks == 1 && text[offs + i] != '.') return 0;
			else if (num_toks != 1 && text[offs + i] == '.') return 0;
			else if (num_toks == 1) num_toks++;
			else if (num_toks == 2)
			{
				buff[j++] = ' ';
				num_toks++;
			} else if (num_toks == 3) break;
		}
		buff[j++] = text[offs + i];
	}
	buff[j] = '\0';
	if (num_toks != 3 || strlen(buff) == 0) return 0;

	a = 0;
	b = num_common_pairs - 1;
	i = 1;

	// printf("searching for %s...\n", buff);
	while (a <= b && i <= strlen(buff))
	{
		c = (a + b) / 2;
		// printf("comparing %s at length %d\n", common_pair[c], i);
		cmp = strncmp(common_pair[c], buff, i);
		if (cmp < 0)
		{
			if (a == b) return 0;
			a = c + 1;
		} else if (cmp > 0)
		{
			if (a == b) return 0;
			b = c - 1;
		} else
		{
			if (strlen(common_pair[c]) == i
			&& (strlen(buff) == i || (! isalnum(buff[i]))))
			{
				// printf("Returning match at %d\n", i);
				return i;
			}
			i++;
		}
	}
	// printf("Returning no match\n");
	return 0;
}

static char nextchar(const char *t, int i)
{
	while (isspace(t[i])) i++;
	return t[i];
}

// Look for a token at or prior to the text position

static int lookbehind(const char *t, int i, const char *s, int *tokflag)
{
	int	k = (int) strlen(s) - 1;

	while (i > 0 && isspace(t[i])) i--;

	while (k >= 0 && i >= 0)
	{
		if (k > 0 && tokflag[i]) break;

		if (tolower(s[k]) != tolower(t[i]))
			return -1;
		k--;
		i--;
	}

	return (k < 0 && tokflag[i+1]) ? i + 1 : -1;
}

// Look for a token at or following the text position

static int lookahead(const char *t, int i, const char *s, int *tokflag)
{
	int	k = 0;

	while (isspace(t[i])) i++;

	while (k < strlen(s) && i < strlen(t))
	{
		if (k > 0 && tokflag[i]) break;

		if (tolower(s[k]) != tolower(t[i]))
			return -1;
		k++;
		i++;
	}

	return (k == strlen(s) && tokflag[i]) ? i - (int) strlen(s) : -1;
}

// Set the initial tokens at spaces

void MPtok::tok_0(const char *text, int *tokflag)
{
	int i;

	tokflag[0] = 1;
	for (i = 1; i < strlen(text); i++)
	{
		tokflag[i] = isspace(text[i]) || (i > 0 && isspace(text[i - 1])) ? 1 : 0;
	}
	tokflag[i] = 1;
}

// Get quotes preceded by open parens
//
// A double quote, preceded by a space or open bracket is a separate token
//

void MPtok::tok_1(const char *text, int *tokflag)
{
	for (int i = 1; i < strlen(text); i++)
	{
		if (text[i] == '"' && strchr("([{<", text[i-1]))
		{
			tokflag[i] = 1;
			if (i + 1 < strlen(text)) tokflag[i+1] = 1;
		}
	}
}

// Look for ellipses
//
// Three dots in a row is a separate token

void MPtok::tok_2(const char *text, int *tokflag)
{
	for (int i = 1; i + 2 < strlen(text); i++)
	{
		if (strncmp(&text[i], "...", 3) == 0)
		{
			tokflag[i] = 1;
			if (i + 3 < strlen(text)) tokflag[i+3] = 1;
		}
	}
}

// Non-sentence-ending punctuation
//
// Certain punctuation characters are separate tokens

void MPtok::tok_3(const char *text, int *tokflag)
{
	for (int i = 0; i < strlen(text); i++)
	{
		// If it is a comma and the next char is not a space and option_comma = 0

		if (option_comma == 0 && text[i] == ',' && isspace(text[i + 1]) == 0)
		{
			// do nothing
		} else if (strchr(",;:@#$%&", text[i]))
		{
			tokflag[i] = 1;
			tokflag[i + 1] = 1;
		}
	}
}

// Separate the slashes
//
// Slashes are a separate token
// except for +/-, +/+, -/-, -/+, and and/or.

void MPtok::tok_5_6_7(const char *text, int *tokflag)
{
	for (int i = 0; i < strlen(text); i++)
	{
		if (text[i] == '/')
		{
			tokflag[i] = 1;
			if (i+1 < strlen(text)) tokflag[i+1] = 1;

			// Put back +/-, etc

			if (i - 1 >= 0
			&& i + 1 < strlen(text)
			&& ((option_new < 9
				&& text[i - 1] == '+' || (text[i - 1] == '-' && option_hyphen == 0)
				&& text[i + 1] == '+' || (text[i + 1] == '-' && option_hyphen == 0))
			|| (option_new >= 9
				&& (text[i - 1] == '+' || text[i - 1] == '-')
				&& (text[i + 1] == '+' || text[i + 1] == '-'))))
			{
				tokflag[i - 1] = 1;
				tokflag[i] = tokflag[i+1] = 0;
				tokflag[i + 2] = 1;
			}

			// Put back and/or, etc

			if (option_new <= 7)
			{
				if (i > 5 && strncmp(text + i - 5, " and/or ", 8) == 0)
				{
					for (int j = 1; j < 5; j++)
						tokflag[i - 2 + j] = 0;
				}
			} else
			{
				if (i > 4 && strncmp(text + i - 4, " and/or ", 8) == 0)
				{
					for (int j = 1; j < 6; j++)
						tokflag[i - 3 + j] = 0;
				}
			}
		}
	}
}

// All brackets
//
// Any open or closed bracket is a separate token
//
// Exclamation and question mark
//
// Any question or exclamation mark is a separate token

void MPtok::tok_8_9(const char *text, int *tokflag)
{
	for (int i = 0; i < strlen(text); i++)
	{
		if (strchr("[](){}<>", text[i])
		|| strchr("?!", text[i]))
		{
			tokflag[i] = 1;
			if (i + 1 < strlen(text)) tokflag[i+1] = 1;
		}
	}
}

// Period at the end of a string may be followed by closed-bracket or quote
//
// A period that is preceded by a non-period
// and optionally followed by a close paren
// and any amount of space at the end of the string
// is a separate token.

void MPtok::tok_10(const char *text, int *tokflag)
{
	for (int i = (int) strlen(text) - 1; i >= 0; i--)
	{
		if (isspace(text[i])) continue;
		if (strchr("])}>\"'", text[i])) continue;
		if (text[i] != '.') break;
		if (text[i] == '.' && (i - 1 < 0 || text[i-1] != '.'))
		{
			tokflag[i] = 1;
			if (i + 1 < strlen(text)) tokflag[i+1] = 1;
		}
	}
}

// Period followed by a capitalized word
//
// A period preceded by a character that is not another period and not a space
// and followed by a space then an upper case letter is a separate token

void MPtok::tok_11(const char *text, int *tokflag)
{
	for (int i = 0; i < strlen(text); i++)
	{
		if (text[i] == '.'
		&& (i + 1 < strlen(text) && isspace(text[i+1]))
		&& (i - 1 < 0 || text[i - 1] != '.' || isspace(text[i-1]) == 0)
		&& isupper(nextchar(text, i + 1)))
			tokflag[i] = 1;
	}
}

// A normal word followed by a period
//
// A period followed by a space
// and preceded by 2 or more alphabetic characters or hyphens
// is a separate token

void MPtok::tok_12(const char *text, int *tokflag)
{
	int wcnt = 0;

	for (int i = 0; i < strlen(text); i++)
	{
		if (text[i] == '.'
		&& tokflag[i + 1]
		&& wcnt >= 2)
			tokflag[i] = 1;

		if (isalpha(text[i]) || text[i] == '-')
			++wcnt;
		else
			wcnt = 0;
	}
}

// A non-normal token (that has no lower case letters) followed by a period
//
// A period at the end of a token made of characters excluding lower case
// is a separate token

void MPtok::tok_13(const char *text, int *tokflag)
{
	int	stok = 0;
	int	wcnt = 0;

	for (int i = 0; i < strlen(text); i++)
	{
		if (text[i] == '.'
		&& tokflag[i + 1]
		&& wcnt >= 2)
			tokflag[i] = 1;

		if (tokflag[i] == 1) stok = 1;

		if (islower(text[i]) || text[i] == '.')
		{
			stok = 0;
			wcnt = 0;
		}

		if (stok)
			wcnt++;
	}
}

// put some periods with single-letter abbreviations
//
// A single alphabetic token followed by a period followed
// by a token that does not begin with an upper case letter
// or number is taken to be an abbreviation and the period
// does not start a new token.
//
// NOTE: This does not recognize initials in people's names,
//	 that problem is not simply solved.

void MPtok::tok_14(const char *text, int *tokflag)
{
	for (int i = 0; i < strlen(text); i++)
	{
		if (text[i] == '.'
		&& i - 1 >= 0 && isalpha(text[i - 1]) && tokflag[i - 1]
		&& tokflag[i + 1]
		&& isupper(nextchar(text, i + 1)) == 0
		&& isdigit(nextchar(text, i + 1)) == 0
		&& nextchar(text, i + 1) != '('
		)
		{
			tokflag[i] = 0;
		}
	}
}

// some abbreviations, cannot end a sentence
//
// Look for some specific common abbreviations
// that don't usually end a sentence.

enum { ABB_ABB, ABB_EOS, ABB_NUM };

#define NUM_ABB_4 15
#define NUM_ABB_7 81

static struct _abb {
	char *str;
	int len;
	int eos;
} common_abb[] = {
	"i.e.", 4, ABB_ABB,
	"e.g.", 4, ABB_ABB,
	"approx.", 7, ABB_EOS,
	"vs.", 3, ABB_ABB,
	"al.", 3, ABB_EOS,
	"viz.", 4, ABB_ABB,
	"v.", 2, ABB_ABB,
	"Mr.", 3, ABB_ABB,
	"Ms.", 3, ABB_ABB,
	"Dr.", 3, ABB_ABB,
	"Mrs.", 4, ABB_ABB,
	"Drs.", 4, ABB_ABB,
	"Prof.", 5, ABB_ABB,
	"Sen.", 4, ABB_ABB,
	"St.", 3, ABB_ABB,
	"ca.", 3, ABB_ABB,
	"et.", 3, ABB_ABB,
	"etc.", 4, ABB_EOS,
	"sp.", 3, ABB_ABB,
	"spp.", 4, ABB_ABB,
	"subsp.", 6, ABB_ABB,
	"pv.", 3, ABB_ABB,
	"U.S.", 4, ABB_ABB,
	"U.K.", 4, ABB_ABB,
	"no.", 3, ABB_NUM,
	"conc.", 5, ABB_EOS,
	"prep.", 5, ABB_EOS,
	"rel.", 4, ABB_EOS,
	"hum.", 4, ABB_EOS,
	"bv.", 3, ABB_ABB,
	"An.", 3, ABB_ABB,
	"Bio.", 4, ABB_ABB,
	"Biol.", 5, ABB_ABB,
	"Ae.", 3, ABB_ABB,
	"Approx.", 7, ABB_ABB,
	"Exp.", 4, ABB_NUM,
	"Expt.", 5, ABB_NUM,
	"No.", 3, ABB_NUM,
	"nos.", 4, ABB_NUM,
	"Nos.", 4, ABB_NUM,
	"cm.", 3, ABB_EOS,
	"hrs.", 4, ABB_EOS,
	"kcat.", 5, ABB_EOS,
	"kmin.", 5, ABB_EOS,
	"lib.", 4, ABB_EOS,
	"mg.", 3, ABB_EOS,
	"min.", 4, ABB_EOS,
	"ml.", 3, ABB_EOS,
	"mol.", 4, ABB_EOS,
	"mm.", 3, ABB_EOS,
	"pts.", 4, ABB_ABB,
	"ssp.", 4, ABB_ABB,
	"var.", 4, ABB_ABB,
	"wt.", 3, ABB_ABB,
	"Ae.", 3, ABB_EOS,
	"An.", 3, ABB_EOS,
	"Bac.", 4, ABB_EOS,
	"Lu.", 3, ABB_EOS,
	"mg./kg.", 7, ABB_EOS,
	"ng./ml.", 7, ABB_EOS,
	"gm.", 3, ABB_EOS,
	"hr.", 3, ABB_EOS,
	"int.", 4, ABB_EOS,
	"iv.", 3, ABB_EOS,
	"msec.", 5, ABB_EOS,
	"ng.", 3, ABB_EOS,
	"nm.", 3, ABB_EOS,
	"nucl.", 5, ABB_EOS,
	"org.", 4, ABB_EOS,
	"sec.", 4, ABB_EOS,
	"vit.", 4, ABB_ABB,
	"vol.", 4, ABB_EOS,
	"wts.", 4, ABB_EOS,
	"microg.", 7, ABB_EOS,
	"pmol.", 5, ABB_EOS,
	"pt.", 3, ABB_NUM,
	"yr.", 3, ABB_EOS,
	"yrs.", 4, ABB_EOS,
	"cv.", 3, ABB_ABB,
	"Mt.", 3, ABB_ABB,
	"microM.", 7, ABB_EOS,
	"Jr.", 3, ABB_EOS,
	"Sr.", 3, ABB_EOS,
	"Fig.", 4, ABB_ABB,
};

#define NUM_ABB (sizeof common_abb)

void MPtok::tok_15(const char *text, int *tokflag)
{
	int	i, j, k, a;
	int	num_abb = NUM_ABB;

	if (option_new <= 4)
		num_abb = NUM_ABB_4;
	else if (option_new <= 7)
		num_abb = NUM_ABB_7;

	for (i = 0; i < strlen(text); i++)
	{
		if (tokflag[i] == 1)
		{
			for (a = 0; a < num_abb; a++)
			{
				k = common_abb[a].len;
				if (strncmp(&text[i], common_abb[a].str, k) == 0 && tokflag[i + k])
				{
					// For abbreviations that require a number

					if (option_new >= 5
					&& common_abb[a].eos == ABB_NUM)
					{
						for (j = i + k; isspace(text[j]); ++j)
							;

						if (! isdigit(text[j]))
							continue;
					}

					// Clear the token flags

					for (j = 1; j < k; j++)
						tokflag[i + j] = 0;

					// Check that this isn't the end of a sentence?
					// if it is, then make the last period a full stop

					if (option_new >= 5
					&& common_abb[a].eos == ABB_EOS)
					{
						for (j = i + k; isspace(text[j]); ++j)
							;

						if (isupper(text[j]))
							tokflag[i + k - 1] = 1;
					}

					break;
				}
			}
		}
	}
}

// Check for common pairs that should not be considered sentence breaks

void MPtok::tok_15_1(const char *text, int *tokflag)
{
	int	i, j, k;

	for (i = 0; i < strlen(text); i++)
	{
		j = find_common_pair(text, i, tokflag);
		if (j > 0)
		{
			for (k = i; k < i + j; k++)
			{
				if (text[k] == '.')
				{
					tokflag[k] = 0;
					break;
				}
			}
			i += j - 1;
		}
	}
}

// Get cases where a space after a sentence has been omitted
//
// A period that occurs in a token consisting of alphabetic
// letters with a vowel to the left and the right is a
// separate token.

void MPtok::tok_16(const char *text, int *tokflag)
{
	int	j;
	int	has_vowel;

	for (int i = 0; i < strlen(text); i++)
	{
		if (text[i] == '.' && tokflag[i] == 0)
		{
			has_vowel = 0;
			for (j = i - 1; j >= 0; --j)
			{
				if (isalpha(text[j]) == 0)
					break;
				if (strchr("aeiouAEIOU", text[j]))
					has_vowel = 1;
				if (tokflag[j])
					break;
			}
			if ((j >= 0 && tokflag[j] == 0) || has_vowel == 0)
				continue;

			j = i + 1;

			has_vowel = 0;
			for (; j < strlen(text) && tokflag[j] == 0; ++j)
			{
				if (isalpha(text[j]) == 0)
					break;
				if (strchr("aeiouAEIOU", text[j]))
					has_vowel = 1;
			}

			if ((j < strlen(text) && tokflag[j] == 0) || has_vowel == 0)
				continue;

			tokflag[i] = 1;
			tokflag[i + 1] = 1;
		}
	}
}

// Correction to tok_16,
// Don't count if the token before is a single letter
// or the token following is a single letter other than 'a'.
// Also, don't count if the token to the right is gov, com, edu, etc.
// because those are web addresses!

#define COMPLEX_WINDOW 40

enum {COMPLEX_NOT = 0, COMPLEX_YES, COMPLEX_DONE};

struct _complex {
	int	flag;
	int	offset;
	char	*str;
	int	len;
} complex[] = {
	COMPLEX_YES, 0, "complex", 7,
	COMPLEX_NOT, 0, "complexi", 8,
	COMPLEX_NOT, 0, "complexed", 9,
	COMPLEX_NOT, 0, "complexa", 8,
	COMPLEX_NOT, 0, "complex-", 8,
	COMPLEX_NOT, 0, "complexl", 8,
	COMPLEX_NOT, 0, "complexu", 8,
	COMPLEX_NOT, -1, "-complex", 7,
	COMPLEX_NOT, -2, "nocomplex", 9,
	COMPLEX_NOT, -3, "subcomplex", 10,
	COMPLEX_YES, 0, "hybrid", 6,
	COMPLEX_NOT, 0, "hybridi", 7,
	COMPLEX_NOT, 0, "hybrido", 7,
	COMPLEX_NOT, 0, "hybrida", 7,
	COMPLEX_NOT, 0, "hybrid-", 7,
	COMPLEX_NOT, -1, "-hybrid", 7,
	COMPLEX_YES, 0, "duplex", 6,
	COMPLEX_NOT, -1, "oduplex", 7,
	COMPLEX_DONE, 0, NULL, 0,
};

static int complex_check(const char *text)
{
	int	last_period = -2*COMPLEX_WINDOW;
	int	last_complex = -2*COMPLEX_WINDOW;
	int	i, j;
	int	complex_match;

	for (i = 0; i < strlen(text); i++)
	{
		if (text[i] == '.')
		{
			if (i - last_complex <= COMPLEX_WINDOW)
				return 1;
			last_period = i;
		}

		complex_match = 0;
		for (j = 0; complex[j].str; j++)
		{
			if (complex[j].flag == COMPLEX_NOT)
			{
				if (i + complex[j].offset >= 0
				&& strncmp(text+i+complex[j].offset, complex[j].str, complex[j].len) == 0)
				{
					// don't match here
					complex_match = 0;
				}
			} else if (complex[j].flag == COMPLEX_YES)
			{
				if (i + complex[j].offset >= 0
				&& strncmp(text+i+complex[j].offset, complex[j].str, complex[j].len) == 0)
				{
					// match here
					complex_match = 1;
				}
			}
		}

		if (complex_match)
		{
			if (i - last_period <= COMPLEX_WINDOW)
				return 1;
			last_complex = i;
		}
	}
	return 0;
}

void MPtok::tok_16_1(const char *text, int *tokflag)
{
	int	i, j;
	char	v1, v2;
	int	c1, c2;

	if (option_new == 3 && strstr(text, "complex"))
		return;

	if (option_new >= 4 && complex_check(text))
		return;

	for (i = 0; i < strlen(text); i++)
	{
		if (text[i] == '.' && tokflag[i] == 0)
		{
			char	suffix[10];
			int	s_i;

			v1 = '\0';
			c1 = 0;
			for (j = i - 1; j >= 0; --j)
			{
				if (isalpha(text[j]) == 0)
					break;
				if (strchr("aeiouAEIOU", text[j]))
					v1 = tolower(text[j]);
				c1++;
				if (tokflag[j])
					break;
			}
			if ((j >= 0 && tokflag[j] == 0)
			|| v1 == '\0'
			|| c1 == 1)
				continue;

			j = i + 1;

			v2 = '\0';
			c2 = 0;
			s_i = 0;
			for (; j < strlen(text) && tokflag[j] == 0; ++j)
			{
				if (isalpha(text[j]) == 0)
					break;
				if (strchr("aeiouAEIOU", text[j]))
					v2 = tolower(text[j]);
				if (s_i < 3)
					suffix[s_i++] = tolower(text[j]); suffix[s_i] = '\0';
				c2++;
			}

			if ((j < strlen(text) && tokflag[j] == 0)
			|| v2 == '\0'
			|| (c2 == 1 && v2 != 'a')
			|| (c2 == 3 && tokflag[j] == 1 && s_i == 3
				&& (strcmp(suffix, "gov") == 0
					|| strcmp(suffix, "edu") == 0
					|| strcmp(suffix, "org") == 0
					|| strcmp(suffix, "com") == 0)))
				continue;

			tokflag[i] = 1;
			tokflag[i + 1] = 1;
		}
	}
}


// Numeric endings of sentences
//
// A period after a numeric token followed by a token that starts
// with an alphabetic character, is a separate token.
//
// This should be covered already by tok_13

void MPtok::tok_17(const char *text, int *tokflag)
{
	int	j;

	for (int i = 0; i < strlen(text); i++)
	{
		if (text[i] == '.'
		&& tokflag[i] == 0
		&& tokflag[i + 1])
		{
			for (j = i - 1; j >= 0 && isdigit(text[j]) && tokflag[j] == 0; --j)
				;
			if (j >= 0 && j < i - 1 && tokflag[j] && isalpha(nextchar(text, i + 1)))
				tokflag[i] = 1;
		}
	}
}

// period at end of string is a token

void MPtok::tok_20(const char *text, int *tokflag)
{
	for (int i = strlen(text) - 1; i >= 0; --i)
	{
		if (isspace(text[i]))
			continue;

		if (strchr(".!?", text[i]))
			tokflag[i] = 1;

		break;
	}
}

// a period that follows a non-common word, and that is
// followed by a lower case common word is probably not a token

void MPtok::tok_20_1(const char *text, int *tokflag)
{
	int	j;

	for (int i = 0; i < strlen(text); ++i)
	{
		if (text[i] == '.' && tokflag[i] == 1)
		{
			int tcnt, lcnt, ocnt;
			tcnt = lcnt = ocnt = 0;

			// make sure the previous word was *not* common

			for (j = i - 1; j >= 0; j--)
			{
				if (isspace(text[j])) continue;
				if (option_new >= 2)
				{
					if (islower(text[j]) == 0 && text[j] != '-') ocnt++;
				} else
				{
					if (! islower(text[j])) ocnt++;
				}

				if (tokflag[j] || j == 0)
				{
					if (ocnt == 0)
					{
						goto nexti;
					}
					break;
				}
			}

			tcnt = lcnt = ocnt = 0;

			// make sure the next word is common

			for (j = i + 1; j < strlen(text); j++)
			{
				if (isspace(text[j])) continue;
				if (tokflag[j]) tcnt++;

				if (tcnt == 2 || j == strlen(text) - 1)
				{
					if (lcnt > 0 && ocnt == 0) tokflag[i] = 0;
					break;
				}

				if (islower(text[j])) lcnt++;
				else ocnt++;
			}
		}
nexti:		;
	}
}

// tokenized period followed by non-space other than close paren
// is not a token

void MPtok::tok_20_2(const char *text, int *tokflag)
{
	int	j;

	for (int i = 0; i < strlen(text) - 1; ++i)
	{
		if (text[i] == '.' && tokflag[i] == 1
		&& strchr(" ()[]\"\'\n\t\r", text[i+1]) == 0)
		{
			tokflag[i] = 0;
		}
	}
}


// long dash
//
// A pair of hyphens is a complete token

void MPtok::tok_21(const char *text, int *tokflag)
{
	for (int i = 0; i + 1 < strlen(text); i++)
	{
		if (strncmp(&text[i], "--", 2) == 0)
		{
			tokflag[i] = 1;
			if (i + 2 < strlen(text))
			{
				i += 2;
				tokflag[i] = 1;
			}
		}
	}
}

// hyphens
//
// If specified as an option, a hyphen between letters is a complete token

void MPtok::tok_21a(const char *text, int *tokflag)
{
	if (option_hyphen == 0) return;

	for (int i = 0; i + 1 < strlen(text); i++)
	{
		if (text[i] == '-'
		&& (i == 0 || text[i-1] != '-')
		&& text[i+1] != '-')
		{
			tokflag[i] = 1;
			tokflag[i+1] = 1;
		}
	}
}


// quote
//
// Any double quote is a separate token

void MPtok::tok_22(const char *text, int *tokflag)
{
	for (int i = 0; i < strlen(text); i++)
	{
		if (text[i] == '"')
		{
			tokflag[i] = 1;
			if (i + 1 < strlen(text))
			{
				i += 1;
				tokflag[i] = 1;
			}
		}
	}
}

// possessive
//
// Any single quote at the end of a token that is not
// preceded by a single quote is a separate token

void MPtok::tok_23(const char *text, int *tokflag)
{
	for (int i = 0; i < strlen(text); i++)
	{
		if (text[i] == '\''
		&& (i - 1 >= 0 && text[i - 1] != '\'')
		&& tokflag[i + 1])
		{
			tokflag[i] = 1;
		}
	}
}


// quote
//
// If a single quote starts a token, or is preceded by a
// single quote, and followed by a character
// that is not a single quote, then
// the character to it's right is the start of a new token

void MPtok::tok_24(const char *text, int *tokflag)
{
	for (int i = 0; i < strlen(text); i++)
	{
		if (text[i] == '\''
		&& (tokflag[i] == 1 || (i - 1 >= 0 && text[i - 1] == '\''))
		&& (i + 1 < strlen(text) && text[i + 1] != '\''))
		{
			tokflag[i + 1] = 1;
		}
	}
}

// put back possessive
//
// A single quote that is a whole token followed by a lower case s
// that is also a whole token (without space between them)
// should be merged into a single token

void MPtok::tok_25(const char *text, int *tokflag)
{
	for (int i = 0; i < strlen(text); i++)
	{
		if (text[i] == '\''
		&& tokflag[i] == 1
		&& i + 1 < strlen(text) && text[i + 1] == 's'
		&& tokflag[i+1] == 1
		&& (i + 2 >= strlen(text) || isspace(text[i + 2]) || tokflag[i + 2] == 1))
		{
			tokflag[i + 1] = 0;
		}
	}
}

// quote
//
// A pair of single quotes is a separate token

void MPtok::tok_26(const char *text, int *tokflag)
{
	for (int i = 0; i < strlen(text); i++)
	{
		if (strncmp(&text[i], "''", 2) == 0
		|| strncmp(&text[i], "``", 2) == 0)
		{
			tokflag[i] = 1;
			if (i + 2 < strlen(text)) tokflag[i + 2] = 1;
		}
	}
}

// possessive
//
// A single quote followed by a letter s is a possessive

void MPtok::tok_27(const char *text, int *tokflag)
{
	for (int i = 0; i < strlen(text); i++)
	{
		if (text[i] == '\''
		&& i + 1 < strlen(text)
		&& tolower(text[i + 1]) == 's'
		&& (i + 2 >= strlen(text) || tokflag[i + 2]))
		{
			tokflag[i] = 1;
		}
	}
}

// split "cannot" to "can not"
//
// A single token that is the word cannot (in any case)
// is split into two words

void MPtok::tok_28(const char *text, int *tokflag)
{
	for (int i = 0; i < strlen(text); i++)
	{
		if ((strncmp(&text[i], "cannot", 6) == 0
		|| strncmp(&text[i], "Cannot", 6) == 0)
		&& tokflag[i + 6])
		{
			tokflag[i + 3] = 1;
		}
	}
}

// put list item elements back at sentence end
//
// A period that is preceded by an alphanumeric (no space)
// and any amount of preceding space and an end-mark
// stays with the alphanumeric.

void MPtok::tok_29(const char *text, int *tokflag)
{
	int	j;

	for (int i = 0; i < strlen(text); i++)
	{
		if (text[i] == '.'
		&& tokflag[i] && tokflag[i + 1]
		&& i - 1 >= 0 && isalnum(text[i - 1])
		&& tokflag[i - 1]
		&& ((j = lookbehind(text, i-2, ".", tokflag)) >= 0
		||  (j = lookbehind(text, i-2, "?", tokflag)) >= 0
		||  (j = lookbehind(text, i-2, "!", tokflag)) >= 0)
		&& tokflag[j])
		{
			tokflag[i] = 0;
		}
	}
}

// attach list elements to the beginnings of their sentences
// this means, attach the period to the list element
//
// a list element is a single letter or a one or two digits
// which is preceded by an end of sentence ".!?;"
// or colon (provided it doesn't belong to a proportion construct)

void MPtok::tok_29a(const char *text, int *tokflag)
{
	int	i, j;

	for (i = 0; i < strlen(text); i++)
	{
		if (text[i] == '.' && tokflag[i])
		{
			// Look back, make sure the token before the period
			// is either single alphanumeric, or at most a two digit number
			// and the character before that is a punctuation ".?!:,"

			int tcnt, acnt, dcnt, pcnt, ocnt, scnt;
			tcnt = acnt = dcnt = pcnt = ocnt = scnt = 0;
			char p;

			for (j = i - 1; j >= 0; j--)
			{
				if (isspace(text[j])) { scnt++; continue; }
				else if (tcnt == 0 && isalpha(text[j])) ++acnt;
				else if (tcnt == 0 && isdigit(text[j])) ++dcnt;
				else if (tcnt == 1 && strchr(".!?:;,", text[j])) { pcnt++; p = text[j]; }
				else ocnt++;

				if (tokflag[j] || j == 0)
				{
					tcnt++;
					if (tcnt == 1 && ocnt == 0 && scnt == 0
					&& ((acnt == 1 && dcnt == 0) || (acnt == 0 && dcnt > 0 && dcnt <= 2)))
					{
						// This is acceptable
					} else if (tcnt == 2 && pcnt <= 1 && ocnt == 0 && scnt > 0)
					{
						if (p == ':')
						{
							while (--j >= 0 && isspace(text[j]))
								;
							if (j >= 0 && isdigit(text[j]))
							{
								// It's probably a proportion
								break;
							}
						}
						// Jackpot
						tokflag[i] = 0;
					} else
					{
						// This is not
						break;
					}
					scnt = 0;
				}
			}
		}
	}
}

// list elements at the beginning of a string
//
// An alphanumeric token followed by a period
// at the beginning of the line stays with the
// alphanumeric

void MPtok::tok_30(const char *text, int *tokflag)
{
	int	i = 0;

	while (isspace(text[i])) i++;

	if (isalnum(text[i])
	&& tokflag[i]
	&& i + 1 < strlen(text)
	&& text[i + 1] == '.'
	&& tokflag[i + 1])
	{
		tokflag[i + 1] = 0;
	}
}

// process American style numbers

void MPtok::tok_31(const char *text, int *tokflag)
{
	int	j;

	for (int i = 0; i < strlen(text); i++)
	{
		if (text[i] == ','
		&& i + 3 < strlen(text)
		&& tokflag[i] && tokflag[i + 1]
		&& isdigit(text[i + 1])
		&& isdigit(text[i + 2])
		&& isdigit(text[i + 3])
		&& i - 1 >= 0 && isdigit(text[i - 1])
		)
		{
			tokflag[i] = 0;
			tokflag[i + 1] = 0;
		}
	}
}

// process British style numbers

void MPtok::tok_32(const char *text, int *tokflag)
{
	int	j;

	for (int i = 0; i < strlen(text); i++)
	{
		if (text[i] == ' '
		&& i + 3 < strlen(text)
		&& tokflag[i] && tokflag[i + 1]
		&& isdigit(text[i + 1])
		&& isdigit(text[i + 2])
		&& isdigit(text[i + 3])
		&& i - 1 >= 0 && isdigit(text[i - 1])
		)
		{
			tokflag[i] = 0;
			tokflag[i + 1] = 0;
		}
	}
}

// tokenize unicode escapes
//
// Added

void MPtok::tok_33(const char *text, int *tokflag)
{
	int	j;

	for (int i = 0; i < strlen(text); i++)
	{
		if (text[i] == '&')
		{
			if (text[i + 1] == '#')
			{
				for (j = i + 2; isdigit(text[j]); j++)
					;
			} else
			{
				for (j = i + 1; isalpha(text[j]); j++)
					;
			}

			if (text[j] == ';')
			{
				// Tokenize the escape, untokenize everything inside

				tokflag[i] = 1;
				for (i++; i <= j; i++) tokflag[i] = 0;
				tokflag[i] = 1;
			}
		}
	}
}

// Remove tags if they are present

static void tok_un(char *text)
{
	int untok = 0;
	for (int i = 0; text[i]; ++i)
	{
		if (isspace(text[i])) untok = 0;
		if (text[i] == option_tagsep) untok = 1;
		if (untok) text[i] = ' ';
	}
}


void MPtok::set_tokflag(const char *text, int *tokflag)
{

	int	i;

	tok_0(text, tokflag);
	tok_1(text, tokflag);
	tok_2(text, tokflag);
	tok_3(text, tokflag);

	// step 4 replaces tag char, this is done at output

	tok_5_6_7(text, tokflag);
	tok_8_9(text, tokflag);

	tok_10(text, tokflag);
	tok_11(text, tokflag);
	if (option_new >= 1)
	{
		tok_21(text, tokflag);
		tok_21a(text, tokflag);
		tok_22(text, tokflag);
		tok_23(text, tokflag);
		tok_24(text, tokflag);
		tok_25(text, tokflag);
		tok_26(text, tokflag);
		tok_27(text, tokflag);
	}
	tok_12(text, tokflag);
	tok_13(text, tokflag);
	tok_14(text, tokflag);
	if (option_new <= 5)
		tok_15(text, tokflag);
	if (option_new < 2)
		tok_16(text, tokflag);
	tok_17(text, tokflag);

	// steps 18 and 19 recognize periods within parens,
	// and this is moved to the segmentation section

	tok_20(text, tokflag);
	if (option_new >= 1)
	{
		tok_20_1(text, tokflag);
		tok_20_2(text, tokflag);
		if (option_new >= 2)
			tok_16_1(text, tokflag);
		if (option_new >= 6)
			tok_15(text, tokflag);
		if (option_new >= 7)
			tok_15_1(text, tokflag);
	}
	if (option_new < 1)
	{
		tok_21(text, tokflag);
		tok_21a(text, tokflag);
		tok_22(text, tokflag);
		tok_23(text, tokflag);
		tok_24(text, tokflag);
		tok_25(text, tokflag);
		tok_26(text, tokflag);
		tok_27(text, tokflag);
	}
	tok_28(text, tokflag);
	if (option_new >= 1)
		tok_29a(text, tokflag);
	else
		tok_29(text, tokflag);
	tok_30(text, tokflag);
	tok_31(text, tokflag);
	tok_32(text, tokflag);

	tok_33(text, tokflag);
}

/* set_endflag
** 
** After tokflag has been set, find the possible sentence endings.
*/

void MPtok::set_endflag(const char *text, int *tokflag, int *endflag)
{
	int	i;

	// The following tests look for end-stops and label them.
	// They include steps 18 and 19

	for (i = 0; i <= strlen(text); i++)
		endflag[i] = 0;

	// Count the number of unmatched parens

	int up = 0;	// unmatched round parens
	int ub = 0;	// unmatched brackets

	for (i = 0; i < strlen(text); i++)
	{
		if (text[i] == '(') ++up;
		if (text[i] == ')') --up;
		if (text[i] == '[') ++ub;
		if (text[i] == ']') --ub;
		if (up < 0) up = 0;
		if (ub < 0) ub = 0;
	}

	// Now find the end-of-sentence marks

	// tok_18: periods within parentheses, allow for nesting
	// tok_19: periods within brackets, allow for nesting
	//	the perl version solves this by putting the period
	//	back with the previous token, but a better solution
	//	is to allow it to be tokenized but just don't
	// 	allow it to be an end-of-sentence.
	//	Therefore, these are moved to the segmentation
	//	section

	int p = 0;	// round parens
	int b = 0;	// brackets

	for (i = 0; i < strlen(text); i++)
	{
		if (text[i] == '(') ++p;
		if (text[i] == ')') --p;
		if (text[i] == '[') ++b;
		if (text[i] == ']') --b;
		if (p < 0) p = 0;
		if (b < 0) b = 0;

		if (strchr(".!?", text[i])
		&& tokflag[i]
		&& tokflag[i + 1])
		{
			if (option_segment && p <= up && b <= ub)
				endflag[i] = 1;

			// This is optional to join periods with
			// probable abbreviations

			if (p > up || b > ub)
				tokflag[i] = 0;
		}
	}

	// endtokens followed by a single or double quote, which matches
	// a single or double quote in the previous sentence

	if (option_new >= 1)
	{
		int	dquo, squo;
		dquo = squo = 0;

		for (i = 0; i < strlen(text); i++)
		{
			if (text[i] == '"') dquo = ! dquo;
			else if (text[i] == '\'') squo = ! squo;
			else if (endflag[i])
			{
				if ((text[i+1] == '"' && dquo) || (text[i+1] == '\'' && squo))
				{
					endflag[i] = 0;

					// But don't end at all if the next token is something
					// other than an upper case letter.

					if (option_new >= 2)
					{
						int	j;
						int	ok = 0;

						for (j = i + 2; j < strlen(text); j++)
						{
							if (isspace(text[j])) continue;
							// if (isupper(text[j]))
							if (isupper(text[j]) || text[j] == '(')
							{
								ok = 1;
								break;
							}
							if (tokflag[j]) break;
						}

						if (ok)
							endflag[i+1] = 1;
					} else
					{
						endflag[i+1] = 1;
					}
				}
				dquo = squo = 0;
			}
		}
	}
}


/* set_endflag_01
** 
** After tokflag has been set, find the possible sentence endings.
** This has improved paren matching.
*/

#define MAX_MATCH 500		// Maximum length to get a paren match

void MPtok::set_endflag_01(const char *text, int *tokflag, int *endflag)
{
	int	match[strlen(text)];
	int	i, j;

	// The following tests look for end-stops and label them.
	// They include steps 18 and 19

	for (i = 0; i <= strlen(text); i++)
		endflag[i] = 0;

	for (i = 0; i < strlen(text); i++)
		match[i] = 0;

	for (i = strlen(text) - 1; i >= 0; i--)
	{
		if (text[i] == '(' || text[i] == '[')
		{
			for (j = i + 1; text[j] && j - i <= MAX_MATCH; j++)
			{
				// Skip parens that are already matched

				if (match[j] > j)
				{
					j = match[j];
					continue;
				}

				// Look for a matching close paren

				if (match[j] == 0
				&& ((text[i] == '(' && text[j] == ')')
				|| (text[i] == '[' && text[j] == ']')))
				{
					match[i] = j;
					match[j] = i;
					break;
				}
			}
		}
	}

	int next_match = 0;
	for (i = 0; i < strlen(text); i++)
	{
		if (match[i] > next_match)
			next_match = match[i];

		if (strchr(".!?", text[i])
		&& tokflag[i]
		&& tokflag[i + 1]
		&& (option_new <= 4 || option_doteos == 1 || (i > 0 && isspace(text[i-1]) == 0)))
		{
			if (i <= next_match)
				tokflag[i] = 0;
			else if (option_segment)
				endflag[i] = 1;
		}
	}

	// endtokens followed by a single or double quote, which matches
	// a single or double quote in the previous sentence

	int	dquo, squo;
	dquo = squo = 0;

	for (i = 0; i < strlen(text); i++)
	{
		if (option_new <= 7 && text[i] == '"') dquo = ! dquo;
		else if (option_new >= 8 && text[i] == '"' && tokflag[i] && tokflag[i+1]) dquo = ! dquo;
		else if (option_new <= 7 && text[i] == '\'') squo = ! squo;
		else if (option_new >= 8 && text[i] == '\''
		&& tokflag[i] && (tokflag[i+1] || (text[i+1] == '\'' && tokflag[i+2]))) squo = ! squo;
		else if (endflag[i])
		{
			if ((text[i+1] == '"' && dquo) || (text[i+1] == '\'' && squo))
			{
				endflag[i] = 0;

				// But don't end at all if the next token is something
				// other than an upper case letter.

				if (option_new >= 2)
				{
					int	j;
					int	ok = 0;

					for (j = i + 2; j < strlen(text); j++)
					{
						if (isspace(text[j])) continue;
						// if (isupper(text[j]))
						if (isupper(text[j]) || text[j] == '(')
						{
							ok = 1;
							break;
						}
						if (tokflag[j]) break;
					}

					if (ok)
						endflag[i+1] = 1;
				} else
				{
					endflag[i+1] = 1;
				}
			}
			dquo = squo = 0;
		}
	}
}


// Size buffer: return the size of the buffer required to hold all of the tokenized text.
// It can be simply estimated by a formula that depends only on the length of text and number of tokens.

int MPtok::size_buff(const char *text, int *tokflag, int *endflag)
{
	int	size = 1;			// Start with null terminator
	int	t = strlen(option_pretag);	// for each tag, the length of the UNTAG string

	if (t <= 0) t = 1;			// Make sure there is at least one
	t += 2;					// Add one for underscore and one for space

	for (int i = 0; i < strlen(text); i++)
	{
		size++;				// Count all characters
		if (tokflag[i]) size += t;	// Count token delimiters (may overcount)
		if (endflag[i]) size++;		// Add one for newline
	}
	return size;
}


/* save_tok
** 
** Save a single token to a buffer.
*/

char *MPtok::save_tok(char *buff, int &sp, char *tok, int ef)
{
	// Convert tag separator chars and back quotes (?)

	for (int i = 0; tok[i]; i++)
	{
		if (tok[i] == option_tagsep) tok[i] = option_replacesep;
		if (tok[i] == '`') tok[i] = '\'';
	}

	// Skip whitespace if tokens are being output
	// Otherwise, skip whitespace at the start of a sentence

	if (option_token != 0 || sp == 0) while (isspace(*tok)) ++tok;

	// Save the token

	if (strlen(tok) > 0)
	{
		// Add delimiter if needed

		if (option_token != 0 && sp != 0) *(buff++) = ' ';

		// Append token to output

		if (option_new < 9)
		{
			while (*tok && (option_token == 0 || isspace(*tok) == 0))
				*(buff++) = *(tok++);
		} else
		{
			while (*tok)
				*(buff++) = *(tok++);
		}

		sp = 1;

		// Add tag holders

		if (option_token != 0 && strlen(option_pretag) > 0)
		{
			*(buff++) = option_tagsep;
			strcpy(buff, option_pretag);
			buff += strlen(buff);
		}

		// If it was end of sentence, then add newline

		if (ef != 0)
		{
			*(buff++) = '\n';
			sp = 0;
		}

	}

	*buff = '\0';
	return buff;
}

// Strip whitespace after sentences

static void adjust_space(char *buff)
{
	int	i, j;
	char	sp;

	// Start on a non-whitespace char

	for (i = 0; isspace(buff[i]); i++)
		;

	for (j = 0, sp = '\0'; buff[i]; i++)
	{
		if (isspace(buff[i]))
		{
			if (sp == '\0') sp = ' ';
			if (buff[i] == '\n') sp = '\n';
		} else
		{
			if (sp) buff[j++] = sp;
			buff[j++] = buff[i];
			sp = '\0';
		}
	}
	buff[j] = '\0';
}

/* save_sents
**
** After the tokflag and endflag have been set, copy the tokens to the buffer.
*/

char *MPtok::save_sents(const char *text, int *tokflag, int *endflag)
{
	char *buff = new char[size_buff(text, tokflag, endflag)];
	*buff = '\0';

	char *ptr = buff;

	int	i;

	// Move token starts to non-whitespace chars

	int last_tok = 0;
	for (i = 0; i < strlen(text); i++)
	{
		if (tokflag[i] == 1 && isspace(text[i]))
		{
			tokflag[i] = 0;
			last_tok = 1;
		} else if (isspace(text[i]) == 0 && last_tok)
		{
			tokflag[i] = 1;
			last_tok = 0;
		}
	}

	// Extract the tokens and print them out now

	char	tok[1000];
	int	pos = 0;
	int	sp = 0;
	int	ef = 0;

	tok[pos] = '\0';

	for (i = 0; i <= strlen(text); i++)
	{
		// The start of a new token

		if (tokflag[i])
		{
			// Print the current token

			ptr = save_tok(ptr, sp, tok, ef);

			// Start a new token

			pos = 0;
			tok[pos] = '\0';

			ef = 0;
		}

		// Append to the current token

		tok[pos++] = text[i];
		tok[pos] = '\0';

		// If any of the characters in the token are endflagged,
		// Then pass this information along for end-of-sentence

		if (endflag[i]) ef = 1;
	}

	// Print the last token

	ptr = save_tok(ptr, sp, tok, ef);

	// Adjust the end of sentence boundaries

	adjust_space(buff);

	return buff;
}

static void map_escapes(char *text)
{
	char	*s;
	int	j, k, ch;
	char	buff[10];
	int	len;

	k = 0;
	len = strlen(text);
	for (int i = 0; text[i]; i++)
	{
		if (text[i] == '&' && text[i + 1] == '#')
		{
			for (s = &buff[0], j = 2; j <= 4 && i + j < strlen(text) && isdigit(text[i + j]); j++)
				*s++ = text[i + j];
			*s = '\0';
			ch = atoi(buff);
			if (strlen(buff) > 0 && text[i + j] == ';' && ch > 0 && ch <= 256)
			{
				text[k] = ch;
				if (! text[k]) text[k] = ' ';
				k++;
				i = i + j;
				continue;
			}
		}
		text[k++] = text[i];
	}
	text[k] = '\0';
}

MPtok::MPtok(char *cnam)
{
	word_str = NULL;
	word_ptr = NULL;
	num_words = 0;

	sent_str = NULL;
	sent_ptr = NULL;
	num_sents = 0;

	strcpy(option_pretag, "");
	option_token = 1;
	option_segment = 1;
	option_hyphen = 0;
	option_comma = 1;
	option_pretok = 0;
	option_new = MPTOK_VERSION;
	option_doteos = 0;

	common_pair = NULL;
	num_common_pairs = 0;

	if (cnam && strlen(cnam) > 0)
	{
		suf = new char[strlen(cnam) + 2];
		*suf = '_';
		strcpy(suf + 1, cnam);
	} else
	{
		suf = new char[1];
		*suf = '\0';
	}
}

void MPtok::init(void)
{
	char    *idir = getenv("MEDPOST_HOME");
	char    buff[1000];

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

void MPtok::init(char *idir)
{
	char    fname[1000];

	sprintf(fname, "%s/medpost%s.pairs", idir, suf);
	init_common_pairs(fname, 10000);
}

MPtok::~MPtok()
{
	if (word_str) { delete[] word_str; word_str = NULL; }
	if (word_ptr) { delete[] word_ptr; word_ptr = NULL; }
	num_words = 0;

	if (sent_str) { delete[] sent_str; sent_str = NULL; }
	if (sent_ptr) { delete[] sent_ptr; sent_ptr = NULL; }
	num_sents = 0;

	if (common_pair)
	{
		for (int i = 0; i < num_common_pairs; i++)
			delete[] common_pair[i];
		delete[] common_pair;
	}
	num_common_pairs = 0;

	if (suf) delete[] suf;
}

// Global tokenizer

char *MPtok::tokenize(char *text)
{
	map_escapes(text);

	int	len = strlen(text);
	if (len == 0) return NULL;

	int *tokflag = new int[len + 1];
	int *endflag = new int[len + 1];

	if (option_pretok)
	{
		tok_un(text);
		tok_0(text, tokflag);
		for (int i = 0; i <= strlen(text); i++)
			endflag[i] = 0;
	} else
	{
		set_tokflag(text, tokflag);
		if (option_new < 3)
			set_endflag(text, tokflag, endflag);
		else
			set_endflag_01(text, tokflag, endflag);
	}

	char *buff = save_sents(text, tokflag, endflag);

	delete[] tokflag;
	delete[] endflag;

	return buff;
}

// Tokenizer for text that is already tokenized

char *MPtok::tokenize_pre(char *text)
{
	int	save_option_pretok = option_pretok;

	option_pretok = 1;
	char *buff = tokenize(text);
	option_pretok = save_option_pretok;

	return buff;
}


void MPtok::tokenize_save(char *text)
{
	int	save_option_token = option_token;
	int	save_option_segment = option_segment;

	option_token = 1;
	option_segment = 0;

	save_string(tokenize(text));

	option_token = save_option_token;
	option_segment = save_option_segment;
}

void MPtok::tokenize_presave(char *text)
{
	int	save_option_token = option_token;
	int	save_option_segment = option_segment;

	option_token = 1;
	option_segment = 0;

	save_string(tokenize_pre(text));

	option_token = save_option_token;
	option_segment = save_option_segment;
}

void MPtok::save_string(char *s)
{
	// Clear the word cache

	if (word_str) { delete[] word_str; word_str = NULL; }
	if (word_ptr) { delete[] word_ptr; word_ptr = NULL; }
	num_words = 0;

	word_str = s;

	if (! word_str) return;

	// Count the number of words

	while (*s && isspace(*s)) ++s;

	int nw = 0;

	while (*s)
	{
		++nw;

		// Go to the end of the token

		while (*s && isspace(*s) == 0) s++;

		// Null terminate the token, and go to the next one

		while (*s && isspace(*s)) *s++;
	}
	s = word_str;

	// Allocate twice as many spaces for idiom splitting
	// (which still might not be enough)

	word_ptr = new char*[2*nw];

	while (*s && isspace(*s)) ++s;

	num_words = 0;

	while (*s)
	{
		word_ptr[num_words] = s;

		++num_words;

		// Go to the end of the token

		while (*s && isspace(*s) == 0) s++;

		// Null terminate the token, and go to the next one

		while (*s && isspace(*s)) *(s++) = '\0';
	}
}

void MPtok::segment_save(char *text)
{
	int	save_option_token = option_token;
	int	save_option_segment = option_segment;
	option_token = 0;
	option_segment = 1;

	// Clear the sent cache

	if (sent_str) { delete[] sent_str; sent_str = NULL; }
	if (sent_ptr) { delete[] sent_ptr; sent_ptr = NULL; }
	num_sents = 0;

	sent_str = tokenize(text);

	if (! sent_str) return;

	char	*s = sent_str;

	// Count the number of sentences

	while (*s && isspace(*s)) ++s;

	int ns = 0;

	while (*s)
	{
		++ns;

		// Go to the end of the sentence

		while (*s && *s != '\n') s++;

		// Go to the next one

		while (*s && isspace(*s)) *s++;
	}
	s = sent_str;

	sent_ptr = new char*[ns];

	while (*s && isspace(*s)) ++s;

	while (*s)
	{
		sent_ptr[num_sents] = s;

		++num_sents;

		// Go to the end of the sentence

		while (*s && *s != '\n') s++;

		// Null terminate the token, and go to the next one

		while (*s && isspace(*s)) *(s++) = '\0';
	}

	option_token = save_option_token;
	option_segment = save_option_segment;
}


char *MPtok::word(int i)
{
	if (word_str == NULL || word_ptr == NULL || i < 0 || i >= num_words) return "";
	return word_ptr[i];
}

char *MPtok::sent(int i)
{
	if (sent_str == NULL || sent_ptr == NULL || i < 0 || i >= num_sents) return "";
	return sent_ptr[i];
}

// Callable functions to set internal options

void MPtok::set_segment(int i) { option_segment = i; }
void MPtok::set_token(int i) { option_token = i; }
void MPtok::set_hyphen(int i) { option_hyphen = i; }
void MPtok::set_comma(int i) { option_comma = i; }
void MPtok::set_pretag(char *a) { strcpy(option_pretag, a); }
void MPtok::set_pretok(int i) { option_pretok = i; }
void MPtok::set_new(int i) { option_new = i; }
void MPtok::set_doteos(int i) { option_doteos = i; }

