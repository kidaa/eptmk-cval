#ifndef _PARSE_H
#define _PARSE_H

/* Port the parse.perl program */

#include <stdio.h>
#include "MPtag.h"

#define MAXBUFF 100000		// maximum length of input/output, ok to set very large

class MPparnode;

class MPpar
{
public:
	friend class MPparnode;

	MPpar();				// Create an empty tree
	MPpar(char *tarstr);			// Create a tree by loading string
	MPpar(MPtag *mptag);			// Create a tree by loading structure
	MPpar(int nn, MPparnode **nd);		// Create a tree with the given nodes
	~MPpar();
	void copy_options(MPpar *t);		// Copy the options

	void load_str(MPtag *mptag);		// Load tagged structure, and make a flat tree for it
	void load_str(char *tagstr);		// Load tagged text, and make a flat tree for it

	// Functions for parsing
	void parse();				// Apply all parse rules to the current tree
	void match(char *pat, char *tag);	// Replace all matching patterns with tag
	void match(char *pat, char **lbuff, int &num);	// Extract all matching patterns
	void parse_parens();			// Reorg and parse contents of parentheticals

	// Functions for printing

	void print(int rec);			// Print tree (with recurson)
	void copy_tokens(char *buff);		// Copy all of the tokens of the tree
	void copy_tags(char *buff);		// ... the tags
	void print_tokens();			// Print them out
	void print_nodes(char *tg);
	void list_nodes(char *tg, char **lbuff, int &num); // List the nodes

	void flatten();				// Flatten the nodes of a tree
	void flatten(char *tg);			// Flatten all (deep) nodes having the given tag

	// These functions are used to perform attachment
	// of postmodifying phrases

	void transport(char *tg, MPpar *t1, int n1, int n2);
	void search_right(int n, char *tg, MPpar *&t1, int &n1);
	void attach(char *pre, char *post);
	void postmod();				// Perform postmod attachment

	// Maintain ownership

	void set_owners();			// Set owner of all subtrees (needed for internal use only)

	char	*tagstr;			// The original text
	int	num_nodes;			// The number of nodes in the "current" parse
	MPparnode **nodes;			// An array of (pointers to) the nodes of the current parse
	MPpar *owner;				// The owner of this tree (if any)

	int option_parens;			// Parse parens (default 0, 1=as is, 2=as constituent)
	int option_conj;			// Process coords (default 0, 1=simple, 2=phrase)
	int option_gerund;			// Attach direct object of gerundives (default 0)
	int option_postmod;			// Attach postmodifying phrases (PP, VPP, etc) (default 0)
	int option_subparse;			// Parse deleted subtrees (default 0)
	int option_printnull;			// Print removed trees
	int option_prep;			// match prepositional phrases (default 1)

	int option_rev;				// Work right to left in string (internal only, default 0)
};

#define NODE_NONE 0
#define NODE_TOKEN 1
#define NODE_TREE 2

class MPparnode
{
public:
	MPparnode(char *tg, char *tk);		// Make a TOKEN node
	MPparnode(char *tg, MPpar *tr);		// Make a TREE node
	~MPparnode();

	void flatten();				// Flatten a tree node into a token node

	int	type;
	char	*tag;
	char	*tok;
	MPpar *tree;
};


#endif

