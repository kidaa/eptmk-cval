# -*- coding: utf-8 -*-


"""
Docstring for corpus_reader.py module.
This module belongs to the medpost corpus.
"""


import os
import nltk


# Corpus path definitions.
CORPUS_BASE = '/'.join(os.path.realpath(__file__).split('/')[:-1])
#CORPUS_TXTS = CORPUS_BASE + '/tagged'  # For corpus with original tagset.
CORPUS_TXTS = CORPUS_BASE + '/tagged_tbank'  # For corpus with Treebank tagset.


# Definition of other arguments to instantiate the TaggedCorpusReader class.
#FILES_RE = r'.*\.ioc'  # For corpus with original tagset.
FILES_RE = r'.*\.ptb'  # For corpus with Treebank tagset.
SENT_TKN_RE = r'P\d{8}A\d{2}'
WORD_TKN_CLS = nltk.tokenize.WhitespaceTokenizer
#TAG_DELIM = '_'  # For corpus with original tagset.
TAG_DELIM = '/'  # For corpus with Treebank tagset.
#PARA_SEG_FUNC = ''  # Undefined paragraph segmentation function.
#TAG_MAP_FUNC = ''  # Undefined func to map between tagsets or simplify tags.
#TXT_ENCO = ''  # Undefined encoding.


def corpus_load():
    """
Docstring for the corpus_load() function.
    """

    my_sent_tkn = nltk.tokenize.RegexpTokenizer(SENT_TKN_RE, gaps=True)
    my_word_tkn = WORD_TKN_CLS()
    return nltk.corpus.TaggedCorpusReader(root=CORPUS_TXTS,
                                          fileids=FILES_RE,
                                          sep=TAG_DELIM,
                                          word_tokenizer=my_word_tkn,
                                          sent_tokenizer=my_sent_tkn)


if __name__ == '__main__':
    pass
