# -*- coding: utf-8 -*-


"""
Docstring for corpus_reader.py module.
This module belongs to the epistemonikos corpus.
"""


import cPickle
import os
import sys
import nltk


# Corpus path definitions.
CORPUS_BASE = '/'.join(os.path.realpath(__file__).split('/')[:-1])
CORPUS_TXTS = CORPUS_BASE + '/ready2use'
CORPUS_PCKL = CORPUS_BASE + '/pickled'


# Definition of arguments used to instantiate the PlaintextCorpusReader class.
FILES_RE = r'.*\.txt'
SENT_TKN_PCKL = 'sent_tokenizer.pickle'
PARA_SEG_FUNC = nltk.corpus.reader.util.read_line_block
WORD_TKN_RE = (r"""([A-Z]\.)+"""      # capitalized acronyms.
               r"""|i\.v\."""         # lower case acronyms.
               r"""|(http://)?www(\.\w+)+"""  # urls
               r"""|t\.i\.d\."""
               r"""|b\.i\.d\."""
               r"""|s\.e\.m\."""
               r"""|t\.d\.s\."""
               r"""|q\.i\.d."""
               r"""|\d+([,.]\d+)*"""  # nums with our without . and ,.
               r"""|\w+(-\w+)*"""     # hyphenated words.
               r"""|\.+"""            # one . or many periods (ellipsis).
               r"""|-+"""             # one - or many dashes.
               r"""|[][}{><"'&$%,:;!?()*+/=-]""")  # all other.
#TXT_ENCO = ''  # Undefined encoding.


def train_sent_tokenizer():
    """
This function trains NLTK's punkt setence tokenizer on the corpus' raw data.
If operation successful the function returns True. If the sub-directory for
pickled files does not exist, or if the file declared for the trained sentence
tokenizer already exists, the function returns False and does nothing. When
training the tokenizer learns how to segment sentences properly (e.g., that
dots in acronyms should not be interpreted as the end of a sentence). The
function then pickles and saves the resulting trained sentence tokenizer to a
file for later use. The aim is to avoid having to train the sentence tokenizer
every time the corpus is loaded, therefore greatly reducing loading times. So
this function should only be used once: to train and pickle/save the resulting
sentence tokenizer. After this is done, there is no further need for this
function, and the trained sentence tokenizer can be read/unpickled and used as
an argument to instantiate the PlaintextCorpusReader class.
    """

    if not os.path.isdir(CORPUS_PCKL):
        print >> sys.stderr, ('\nThe sub-directory for pickled files does not '
                              'exist!\nDOING NOTHING...\n')
        return False
    else:
        if os.path.isfile(CORPUS_PCKL + '/' + SENT_TKN_PCKL):
            print >> sys.stderr, ('\nThe file selected for saving the trained '
                                  'sentence tokenizer already exists!\n'
                                  'DOING NOTHING...\n')
            return False
        else:
            print >> sys.stdout, ('\nTraining sentence tokenizer and saving it'
                                  ' as:\n%s\n' % (CORPUS_PCKL.split('/')[-1]
                                                  + '/' + SENT_TKN_PCKL))

    corpus_txts = []
    for fname in sorted(os.listdir(CORPUS_TXTS)):
        with open(CORPUS_TXTS + '/' + fname, 'r') as txt_file:
            corpus_txts.append(txt_file.read().strip())
    corpus_txts = ' '.join(corpus_txts)
    sent_tkn = nltk.tokenize.PunktSentenceTokenizer(train_text=corpus_txts)
    with open(CORPUS_PCKL + '/' + SENT_TKN_PCKL, 'wb') as pckl_fname:
        cPickle.dump(sent_tkn, pckl_fname, -1)
    return True


def corpus_load():
    """
Docstring for corpus_load() function.
    """

    if not os.path.isfile(CORPUS_PCKL + '/' + SENT_TKN_PCKL):
        print >> sys.stderr, ('\nThe file indicated for the trained sentence '
                              'tokenizer does not exist!\n'
                              'CANNOT LOAD THE CORPUS...\n')
        return False

    with open(CORPUS_PCKL + '/' + SENT_TKN_PCKL, 'rb') as pckl_fname:
        my_sent_tkn = cPickle.load(pckl_fname)
    my_word_tkn = nltk.tokenize.RegexpTokenizer(WORD_TKN_RE)
    return nltk.corpus.PlaintextCorpusReader(root=CORPUS_TXTS,
                                             fileids=FILES_RE,
                                             sent_tokenizer=my_sent_tkn,
                                             word_tokenizer=my_word_tkn,
                                             para_block_reader=PARA_SEG_FUNC)


if __name__ == '__main__':
    pass
