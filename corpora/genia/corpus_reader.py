# -*- coding: utf-8 -*-


"""
Docstring for the corpus_reader.py module.
This module belongs to the genia corpus.
"""


import os
import nltk


# Corpus path definitions.
CORPUS_BASE = '/'.join(os.path.realpath(__file__).split('/')[:-1])
CORPUS_TXTS = CORPUS_BASE + '/ready2use'


# Definition of other arguments to instantiate the TaggedCorpusReader class.
FILES_RE = r'.*\.txt'
SENT_TKN_RE = r'====='
WORD_TKN_CLS = nltk.tokenize.LineTokenizer
TAG_DELIM = '/'
PARA_SEG_FUNC = nltk.corpus.reader.util.read_blankline_block
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
                                          sent_tokenizer=my_sent_tkn,
                                          para_block_reader=PARA_SEG_FUNC)


if __name__ == '__main__':
    pass
