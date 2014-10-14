/* Port the parse.perl program */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <pcre.h>

#include "MPpar.h"

static char *buff = NULL;
static char *subs = NULL;
static int *buff_nodes = NULL;

static void alloc_workmem()
{
	if (buff == NULL) buff = new char[MAXBUFF + 1];
	if (subs == NULL) subs = new char[MAXBUFF + 1];
	if (buff_nodes == NULL) buff_nodes = new int[MAXBUFF + 1];
}

// Create an empty tree

MPpar::MPpar()
{
	alloc_workmem();

	num_nodes = 0;
	nodes = NULL;
	owner = NULL;

	option_parens = 0;
	option_conj = 0;
	option_gerund = 0;
	option_rev = 0;
	option_postmod = 0;
	option_subparse = 0;
	option_printnull = 0;
	option_prep = 1;
}

// Create a tree by reading from a string

MPpar::MPpar(char *tagstr)
{
	alloc_workmem();

	num_nodes = 0;
	nodes = NULL;
	owner = NULL;

	option_parens = 0;
	option_conj = 0;
	option_gerund = 0;
	option_rev = 0;
	option_postmod = 0;
	option_subparse = 0;
	option_printnull = 0;
	option_prep = 1;

	load_str(tagstr);
}

// Create a tree by reading from a string

MPpar::MPpar(MPtag *mptag)
{
	alloc_workmem();

	num_nodes = 0;
	nodes = NULL;
	owner = NULL;

	option_parens = 0;
	option_conj = 0;
	option_gerund = 0;
	option_rev = 0;
	option_postmod = 0;
	option_subparse = 0;
	option_printnull = 0;
	option_prep = 1;

	load_str(mptag);
}



// Create a tree with the given nodes
// IMPORTANT: Memory control of these nodes passes to this class
// which means, the caller should not keep a copy of them

MPpar::MPpar(int nn, MPparnode **nd)
{
	alloc_workmem();

	nodes = new MPparnode*[nn];
	for (int i = 0; i < nn; i++) nodes[i] = nd[i];
	num_nodes = nn;
	owner = NULL;

	option_parens = 0;
	option_conj = 0;
	option_gerund = 0;
	option_rev = 0;
	option_postmod = 0;
	option_subparse = 0;
	option_printnull = 0;
	option_prep = 1;

	set_owners();
}

void MPpar::copy_options(MPpar *t)
{
	option_parens = t->option_parens;
	option_conj = t->option_conj;
	option_gerund = t->option_gerund;
	option_rev = t->option_rev;
	option_postmod = t->option_postmod;
	option_subparse = t->option_subparse;
	option_printnull = t->option_printnull;
	option_prep = t->option_prep;
}

MPpar::~MPpar()
{
	if (nodes)
	{
		if (num_nodes > 0)
		{
			for (int i = 0; i < num_nodes; i++)
			{
				if (nodes[i]) delete nodes[i];
			}
		}
		delete[] nodes;
		nodes = NULL;
	}
}

void MPpar::set_owners()
{
	for (int i = 0; i < num_nodes; i++)
	{
		if (nodes[i]->type == NODE_TREE && nodes[i]->tree)
		{
			nodes[i]->tree->owner = this;
			nodes[i]->tree->set_owners();
		}
	}
}

void MPpar::load_str(MPtag *mptag)
{
	int	i;
	int	nn = 0;

	nodes = new MPparnode *[mptag->num_words];

	for (i = 0; i < mptag->num_words; i++)
		nodes[i] = new MPparnode(mptag->tag(i), mptag->word(i));

	num_nodes = mptag->num_words;

	set_owners();
}

// Load tagged text, and make a flat tree for it

#define NUMNODES 100

void MPpar::load_str(char *tagstr)
{
	int	i;
	char	*tk, *tg, *s;

	int num_alloc = NUMNODES;
	nodes = (MPparnode **) malloc(num_alloc * sizeof(MPparnode *));

	num_nodes = 0;
	while (*tagstr)
	{
		while (isspace(*tagstr)) ++tagstr;
		if (! *tagstr) break;
		strcpy(buff, tagstr);

		s = &buff[0];

		tk = NULL;
		tg = s;

		while (*tagstr && isspace(*tagstr) == 0)
		{
			if (*tagstr == '_')
			{
				tk = tg;
				*s = '\0';
				tg = s + 1;
			}
			++tagstr;
			++s;
		}
		*s = '\0';
		if (num_nodes >= num_alloc)
		{
			num_alloc += NUMNODES;
			nodes = (MPparnode **) realloc((void *) nodes, num_alloc * sizeof(MPparnode *));
		}
		if (tk) nodes[num_nodes++] = new MPparnode(tg, tk);
	}

	set_owners();
}

static void spaces(int n)
{
	while (--n >= 0) printf(" ");
}

void MPpar::print(int rec)
{
	int	 i;
	char	*s;

	for (i = 0; i < num_nodes; i++)
	{
		if (option_printnull == 0 && (nodes[i]->tag == NULL || strlen(nodes[i]->tag) == 0))
			continue;

		spaces(rec);

		s = nodes[i]->tag;
		if (s == NULL || strlen(s) == 0) s = "NULL";
		printf("%s", s);

		if (nodes[i]->tok && strlen(nodes[i]->tok) > 0) printf(" \"%s\"", nodes[i]->tok);

		printf("\n");

		if (nodes[i]->tree) nodes[i]->tree->print(rec+1);
	}
}

// Concatenate the tokens to a buffer
// The buffer should be initialized before calling,
// and it should be large enough to hold the tokens

void MPpar::copy_tokens(char *out)
{
	int	i;

	for (i = 0; i < num_nodes; i++)
	{
		if (option_printnull == 0 && (nodes[i]->tag == NULL || strlen(nodes[i]->tag) == 0))
			continue;
		if (nodes[i]->type == NODE_TOKEN)
		{
			if (*out) strcat(out, " ");
			strcat(out, nodes[i]->tok);
		} else
			nodes[i]->tree->copy_tokens(out);
	}
}

// Or, concatenate tags

void MPpar::copy_tags(char *out)
{
	int	i;

	for (i = 0; i < num_nodes; i++)
	{
		if (option_printnull == 0 && (nodes[i]->tag == NULL || strlen(nodes[i]->tag) == 0))
			continue;
		if (nodes[i]->type == NODE_TOKEN)
		{
			if (*out) strcat(out, " ");
			strcat(out, nodes[i]->tag);
		} else
		{
			if (nodes[i]->tag && strlen(nodes[i]->tag))
			{
				strcat(out, " [");
				strcat(out, nodes[i]->tag);
			} else
			{
				strcat(out, " [???");
			}

			nodes[i]->tree->copy_tags(out);

			strcat(out, "]");
		}
	}
}


// Print all of the tokens in the tree

void MPpar::print_tokens()
{
	strcpy(buff, "");
	copy_tokens(buff);
	printf("%s\n", buff);
}

// Print the tokens belonging to all of the nodes
// named tg

void MPpar::print_nodes(char *tg)
{
	int	i;

	for (i = 0; i < num_nodes; i++)
	{
		if (nodes[i]->type == NODE_TOKEN)
		{
			if (strcmp(nodes[i]->tag, tg) == 0)
				printf("%s\n", nodes[i]->tok);
		} else
		{
			if (strcmp(nodes[i]->tag, tg) == 0)
				nodes[i]->tree->print_tokens();
			else
				nodes[i]->tree->print_nodes(tg);
		}
	}
}

// Make a list of the tokens belonging to all of the nodes
// named tg

void MPpar::list_nodes(char *tg, char **lbuff, int &num)
{
	int	i;
	int	add;

	num = 0;
	for (i = 0; i < num_nodes; i++)
	{
		if (nodes[i]->type == NODE_TOKEN)
		{
			if (strcmp(nodes[i]->tag, tg) == 0)
			{
				// printf("%s\n", nodes[i]->tok);
				lbuff[num] = new char[strlen(nodes[i]->tok) + 1];
				strcpy(lbuff[num], nodes[i]->tok);
				num++;
			}
		} else
		{
			if (strcmp(nodes[i]->tag, tg) == 0)
			{
				strcpy(buff, "");
				nodes[i]->tree->copy_tokens(buff);
				lbuff[num] = new char[strlen(buff) + 1];
				strcpy(lbuff[num], buff);
				num++;
			} else
			{
				nodes[i]->tree->list_nodes(tg, lbuff + num, add);
				num += add;
			}
		}
	}
}

void MPpar::flatten()
{
	for (int i = 0; i < num_nodes; i++) nodes[i]->flatten();
}

void MPpar::flatten(char *tg)
{
	for (int i = 0; i < num_nodes; i++)
	{
		if (strcmp(nodes[i]->tag, tg) == 0)
			nodes[i]->flatten();
		else if (nodes[i]->type == NODE_TREE && nodes[i]->tree)
			nodes[i]->tree->flatten(tg);
	}
}

#define OCNT 1000

// Perform matching with extraction

void MPpar::match(char *pat, char **lbuff, int &num)
{
	pcre	*re;
	const char *err;
	int	ero;
	int	rc;
	int	ovec[3 * OCNT];
	int	i, j, j_min, j_max;
	int	offs = 0;

	num = 0;

	re = pcre_compile(pat, 0, &err, &ero, NULL);

	for (;;)
	{
		buff[0] = '\0';
		j = 0;
		for (i = 0; i < num_nodes; i++)
		{
			if (nodes[i] && nodes[i]->tag && strlen(nodes[i]->tag) > 0)
			{
				sprintf(buff + strlen(buff), " %s", nodes[i]->tag);
				while (j < strlen(buff))
				{
					buff_nodes[j] = (isspace(buff[j])) ? -1 : i;
					j++;
				}
			}
		}

		if (strlen(buff) == 0) break;
		strcat(buff, " ");
		buff_nodes[j] = -1;

		// printf("matching with %s\n", pat);
		// printf("matching against \"%s\"\n", buff);

		rc = pcre_exec(re, NULL, buff, strlen(buff), offs, 0, ovec, OCNT);

		// Got a match, perform extraction

		if (rc <= 0) break;

		j_min = j_max = -1;

		for (j = ovec[0]; j < ovec[1]; j++)
		{
			if (buff_nodes[j] >= 0 && j_min < 0) j_min = buff_nodes[j];
			if (buff_nodes[j] > j_max) j_max = buff_nodes[j];
		}

		offs = ovec[1];

		// printf("extracting chars %d-%d (%d nodes %d-%d)\n", ovec[0], ovec[1]-1, j_max - j_min + 1, j_min, j_max);

		buff[0] = '\0';
		for (j = j_min; j <= j_max; j++)
		{
			if (nodes[j]->tok && strlen(nodes[j]->tok) > 0)
			{
				if (buff[0]) strcat(buff, " ");
				strcat(buff, nodes[j]->tok);
				// strcat(buff, "_");
				// strcat(buff, nodes[j]->tag);
			}
		}

		lbuff[num] = new char[strlen(buff) + 1];
		strcpy(lbuff[num], buff);
		num++;

	}

	pcre_free(re);
}


// Perform match/subs

void MPpar::match(char *pat, char *con)
{
	pcre	*re;
	const char *err;
	int	ero;
	int	rc;
	int	ovec[3 * OCNT];
	int	i, j, j_min, j_max;
	MPpar *nt;
	MPparnode *nd;
	int	nn;

	re = pcre_compile(pat, 0, &err, &ero, NULL);

	for (;;)
	{
		buff[0] = '\0';
		j = 0;
		for (i = 0; i < num_nodes; i++)
		{
			if (nodes[i] && nodes[i]->tag && strlen(nodes[i]->tag) > 0)
			{
				sprintf(buff + strlen(buff), " %s", nodes[i]->tag);
				while (j < strlen(buff))
				{
					buff_nodes[j] = (isspace(buff[j])) ? -1 : i;
					j++;
				}
			}
		}

		if (strlen(buff) == 0) break;
		strcat(buff, " ");
		buff_nodes[j] = -1;

		// printf("matching with %s\n", pat);
		// printf("matching against \"%s\"\n", buff);

		if (option_rev)
		{
			for (i = strlen(buff) - 1; i >= 0; i--)
			{
				rc = pcre_exec(re, NULL, buff, strlen(buff), i, 0, ovec, OCNT);
				if (rc > 0) break;
			}
		} else
		{
			rc = pcre_exec(re, NULL, buff, strlen(buff), 0, 0, ovec, OCNT);
		}

		// Got a match, perform replacement... only replace the first matching text before iterating!
		// Update the parse data structure here

		if (rc <= 0) break;

		strcpy(subs, con);
		if (con[0] == '$' && isdigit(con[1]))
		{
			// Handle backref substitutions here! only forms $1, $2, etc can be supported
			// And, only allow a single tag to substitute!

			int d = atoi(&con[1]);
			char *s;
			for (s = &buff[ovec[2 * d]]; *s && isspace(*s); ++s)
				;
			strcpy(subs, s);
			for (s = &subs[0]; *s && isspace(*s) == 0; ++s)
				;
			*s = '\0';
			
		}

		j_min = j_max = -1;

		for (j = ovec[0]; j < ovec[1]; j++)
		{
			if (buff_nodes[j] >= 0 && j_min < 0) j_min = buff_nodes[j];
			if (buff_nodes[j] > j_max) j_max = buff_nodes[j];
		}

		nn = j_max - j_min + 1;

		// printf("replacing chars %d-%d (%d nodes %d-%d) with \"%s\"\n", ovec[0], ovec[1]-1, nn, j_min, j_max, subs);

		nt = new MPpar(nn, &nodes[j_min]);
		nt->copy_options(this);
		nd = new MPparnode(subs, nt);

		// printf("surgery on tree of length %d\n", num_nodes);

		nodes[j_min] = nd;
		for (j = j_max + 1; j < num_nodes; j++)
		{
			nodes[j - nn + 1] = nodes[j];
		}
		num_nodes = num_nodes - nn + 1;
	}

	pcre_free(re);
}

void MPpar::parse_parens()
{
	int	i;

	if (num_nodes < 3) return;
	if (! nodes) return;

	if (strcmp(nodes[0]->tag, "(") != 0 || strcmp(nodes[num_nodes - 1]->tag, ")") != 0)
	{
		for (i = 0; i < num_nodes; i++)
		{
			if (nodes[i]->type == NODE_TREE && nodes[i]->tree)
				nodes[i]->tree->parse_parens();
		}
		return;
	}

	MPpar *nt = new MPpar(num_nodes - 2, &nodes[1]);
	nt->copy_options(this);
	nt->parse();

	// Rely on the fact that the number of nodes returned by the parse
	// is never more than the original number of nodes
	// otherwise there is a crash!

	if (nt->num_nodes <= num_nodes - 2)
	{
		for (i = 1; i <= nt->num_nodes; i++)
		{
			nodes[i] = nt->nodes[i - 1];
			nt->nodes[i - 1] = NULL;
		}
		nodes[i++] = nodes[num_nodes - 1];
		num_nodes = i;
	}

	nt->num_nodes = 0;

	delete nt;
}

// Apply all parse rules to the current tree

void MPpar::parse()
{
	// Remove parenthetic stuff

	if (option_parens == 0)
	{
		match(" \\( [^\\(\\)]+ \\) ", "");
	} else if (option_parens == 2)
	{
		match(" \\((?: [^ \\(\\)]+)* ([^ ]+) \\) ", "$1");
	}

	// Remove quotes, these are grouped with their adjacent tokens

	match(" `` ([^\\s]+) ", "$1");
	match(" ([^\\s]+) \'\' ", "$1");

	// Multi-word lexical items

	match("(?: [^\\s]+\\+)+ ([^\\s]+) ", "$1");

	// Concatenated conjunctions!

	match("(?: CC){2,} ", "CC");

	// symbols acting as coordinators

	if (option_conj > 0)
	{
		match(" ([^\\s]+) SYM \\1 ", "$1");
	}

	// Convert participle modifiers to adjectives, for subsequent recognition

	match(" (?:VVNJ|VVGJ) ", "JJ");

	// Preposed adverbs

	match(" RRT JJ ", "JJT");
	match(" RRR JJ ", "JJR");
	match("(?: RR| RRT| RRR)+ (VM|V[VBHD][NDBZGI]|VVGN|II|JJR|JJT|JJ|MC|CS|RR|RRR|RRT) ", "$1");
	match(" (V..)(?: RR)+ ", "$1");

	// A common comparative expression involving numbers

	match(" RR CSN(?: MC)+ ", "MC");

	// Get coordinations of identical (simple) types

	if (option_conj > 0)
	{
		match("(?: CC)? ([^\\s]+) ,(?: \\1 ,)* \\1(?: ,)? CC(?: RR)* \\1 ", "$1");
		match("(?: CC)? ([^\\s]+) CC(?: RR)* \\1 ", "$1");
	}

	// Get percent signs

	match(" MC SYM ", "MC");

	// Catenative verbs

	// match("(?:(?:(?: VM)? V[BDH][^I]| VVZ))+ V.[GBN](?:(?: TO)? V[BDHV]I(?: VVN)?)? ", "VP");
	match("(?:(?:(?: VM)? V[BDH][^I]| VVZ))+ V.[GBN] ", "VP");

	// Simple verbs

	match("(?: VM)? V[BDHV][BDZ] ", "VP");

	// Noun phrases

	if (option_gerund)
	{
		match("(?:(?: DB)? DD| PNG)?(?: NNS| NNP| JJR| JJT| JJ| NN| MC| GE)* VVGN ", "NPG");
		match("(?:(?: DB)? DD| PNG)?(?: NNS| NNP| JJR| JJT| JJ| NN| MC| GE)*(?: NNS| NNP| NN| PN| PND| MC) ", "NP");
	} else
	{
		match("(?:(?: DB)? DD| PNG)?(?: NNS| NNP| JJR| JJT| JJ| NN| MC| GE)*(?: NNS| NNP| NN| PN| PND| MC| VVGN) ", "NP");
	}

	// Adjectival nouns

	match("(?: DD| PNG| NP GE)(?: RR| RRR| RRT| JJ| JJR| JJT)* JJT ", "NP");

	// Gerundive phrases with an optional subject and object

	match(" NPG(?: NP)? ", "NP");

	// Other infinitives, used as noun phrases

	if (option_prep)
	{
		match("(?: RR)?(?: TO)? V[BDHV]I(?: NP)? ", "NPI");
	}

	// Infinitival noun phrases
	// The problem of infinitive attachment is very similar to the
	// problem of prepositional attachment, in fact the NPIs appear to have
	// a greater tendancy to be adverbial, even when they follow a noun
	// Therefore, we can't deal with this here

	// match(" NP NPI ", "NP");

	// Make lone adjectives into adjectival clauses
	// with optional prepositional phrases following them
	// Also, recognize some comparative clauses that
	// function the same as adjectival clauses

	match("(?: JJ| JJR| JJT)+(?: CSN NP)? ", "AP");

	// Simple coordination of composite types

	// What about something non-standard, like "XX, and XX, XX" (see med1.proto)

	if (option_conj > 1)
	{
		match("(?: CC)? ([^\\s]+) ,(?: \\1 ,)* \\1(?: ,)? CC(?: RR)* \\1 ", "$1");
		match("(?: CC)? ([^\\s]+) CC(?: RR)* \\1 ", "$1");
	}

	// Deciding when the object of a preposition is conjoined
	// or whether the conjunction is at a higher level is not
	// easy to decide. By assigning PP before CC, we err on the
	// side of non-conjoined PP objects, otherwise any conjunctions
	// following a PP will get lumped into the object.

	// Prepositional and participial phrases
	// A good way to do PP attachment is to find PP's from right to left
	// Then if there is an NP to the immediate left, attach it and continue
	// Unfortunately, there is no way to interleave these two rules in the
	// same "production".

	if (option_prep)
	{
		match(" II NP ", "PP");
		match(" V[BDHV][GN](?: NP)? ", "VPP");
	}

	// Coordinated NPs

	if (option_conj > 1)
	{
		match("(?: DD)?(?: CC)? (?:NP|AP) ,(?: NP ,| AP ,)* (?:NP|AP) CC(?: RR)? NP ", "NP");
		match("(?: DD)?(?: CC)? (?:NP|AP) CC(?: RR)? NP ", "NP");

		// Should redo the higher level chunking after this CC

		// match(" II NP ", "PP");
		// match(" VV[GN] NP ", "VPP");
	}

	if (option_subparse)
		parse_parens();

	if (option_postmod > 0)
		postmod();
}

// Perform attachment of post-modifiers

void MPpar::postmod()
{
	attach("NP", "PP");
	attach("AP", "PP");
	attach("NP", "AP");
	attach("NP", "VPP");
	attach("NPI", "PP");
	attach("VPP", "PP");
}

// Move node n2 from this tree to a node
// in another tree, by replacing the node in the other tree
// with a new node (named tg) that conjoins the two

void MPpar::transport(char *tg, MPpar *t1, int n1, int n2)
{
	MPpar *nt;
	MPparnode *nd[2];
	MPparnode *dd;
	int	i, j;

	// printf("Transporting from %d of %x to %d of %x\n", n2, this, n1, t1);

	// First make the new tree

	nd[0] = t1->nodes[n1];
	nd[1] = nodes[n2];

	nt = new MPpar(2, &nd[0]);
	nt->copy_options(this);
	dd = new MPparnode(tg, nt);

	// Replace the node in tree 1 with the new one

	t1->nodes[n1] = dd;

	// Remove the node from tree 2

	for (i = j = 0; i < num_nodes; i++)
	{
		if (i == n2) continue;
		nodes[j++] = nodes[i];
	}
	num_nodes--;

	set_owners();
}

// Search a tree for a node named tg
// in the right-most position

void MPpar::search_right(int n, char *tg, MPpar *&t1, int &n1)
{
	// Do depth first search

	t1 = NULL;
	n1 = 0;

	// printf("Searching for %s in node %d of this tree %s\n", tg, n, nodes[n]->tag);

	if (n < 0) return;

	// Search the last node of the subtree first

	if (nodes[n]->type == NODE_TREE)
	{
		nodes[n]->tree->search_right(nodes[n]->tree->num_nodes - 1, tg, t1, n1);

		// If the tag was found, return it

		if (t1) return;
	}

	// If the tag was not found, or if the node is not a tree node,
	// then return it only if the tag matches

	if (strcmp(nodes[n]->tag, tg) == 0)
	{
		// printf("Tag found in node %d\n", n);
		t1 = this;
		n1 = n;
	}
}

// Attach post modifying tags named post to adjacent constituents
// named pre, by transporting them

void MPpar::attach(char *pre, char *post)
{
	MPpar *t1;
	int	n1, n2;

	for (n2 = 1; n2 < num_nodes; n2++)
	{
#if 0
		if (nodes[n2-1]->type != NODE_TREE
		|| nodes[n2]->type != NODE_TREE)
			continue;
#endif

		if (strcmp(nodes[n2]->tag, post) == 0)
		{
			// printf("Searching for %s-%s attachment of node %d\n", pre, post, n2);

			// Search for the previous adjacent node
			// see if it is an NP (pre)

			search_right(n2 - 1, pre, t1, n1);

			if (t1)
			{
				transport(pre, t1, n1, n2);

				// Start over from the beginning now

				n2 = 0;
			}
		}
	}
}

// Make a TOKEN node

MPparnode::MPparnode(char *tg, char *tk)
{
	alloc_workmem();

	type = NODE_TOKEN;
	tok = tag = NULL;
	tree = NULL;

	if (! tg) tg = "";
	if (! tk) tk ="";

	tag = new char[strlen(tg) + 1];
	strcpy(tag, tg);

	tok = new char[strlen(tk) + 1];
	strcpy(tok, tk);
}

// Make a TREE node
// IMPORTANT: Memory control of the tree passes to this class
// which means, the caller should not keep a copy of it

MPparnode::MPparnode(char *tg, MPpar *tr)
{
	alloc_workmem();

	type = NODE_TREE;

	if (! tg) tg = "";

	tag = new char[strlen(tg) + 1];
	strcpy(tag, tg);

	tok = new char[1];
	*tok = '\0';

	tree = tr;
}

MPparnode::~MPparnode()
{
	if (tree) delete tree;
	if (tag) delete[] tag;
	if (tok) delete[] tok;
}

// Flatten a tree node

void MPparnode::flatten()
{
	if (type == NODE_TOKEN || tree == NULL) return;

	strcpy(buff, "");
	tree->copy_tokens(buff);
	delete tree;
	tree = NULL;
	type = NODE_TOKEN;

	if (tok) delete[] tok;
	tok = new char[strlen(buff) + 1];
	strcpy(tok, buff);
}

