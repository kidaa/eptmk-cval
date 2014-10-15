#!/usr/bin/env python
# -*- coding: UTF-8 -*-

from __future__ import division
import cPickle
import random
import os
from corpora import epistemonikos, genia, medpost
import nltk


GENIA_TAG_SUBS = {'-': ":",      # For a couple of incorrectly tagged dashes.
                  'XT': 'DT',    # Fix 1 incorrect tag for 'a' (DT, not XT).
                  'CT': 'DT',    # Fix 4 incorrect tags for determiners (DT).
                  'PP': 'PRP$',  # Fix 1 incorrect possesive pronoun (PRP$).
                  'N': 'NN',     # Fix 1 incorrect tag for noun (NN).
                  '``': "''",    # Standarize tags for quote symbols.
                  'JJ|NN': 'JJ',
                  'NN|NNS': 'NN'
                  }
MPOST_TAG_SUBS = {'``': "''"
                  }
TBANK_TAG_SUBS = {'``': "''",
                  '$': 'SYM',
                  '#': 'SYM'
                  }


def transform_tags(tagged_sents, mapping_dict):
    """
Transforms (following a key/value map) the tags in the list of tagged sentences
(the 'tagged_sents' input argument) to the appropiate or equivalent POS tag
according to the Treebank POS tagset. The key/value map should be a dictionary
where the keys are the tags to be replaced, and the values correspond to the
Treebank tags that replace them. This function is meant to be used before the
function below ('filter_tagged_sents').
    """
    new_tagged_sents = []
    for sent in tagged_sents:
        new_sent = []
        for tpl in sent:
            if tpl[1] in mapping_dict.keys():
                new_sent.append((tpl[0], mapping_dict[tpl[1]]),)
            else:
                new_sent.append(tpl)
        new_tagged_sents.append(new_sent)
    return new_tagged_sents


def fix_tbank_brackets(tbank_tagged_sents):
    """
For the Treebank tagged corpus (as a list of tagged sentences): substitutes
tokens and tags that have the special codes for brackets, parenthesis and
braces, with the actual bracket, parenthesis or brace (for both token and tag).
    """
    bracket_map = {'-LRB-': '(', '-RRB-': ')',
                   '-LCB-': '{', '-RCB-': '}',
                   '-LSB-': '[', '-RSB-': ']'}
    new_tagged_sents = []
    for sent in tbank_tagged_sents:
        new_sent = []
        for tpl in sent:
            if tpl[1].upper() == '-LRB-':
                new_sent.append((bracket_map[tpl[0]], '('),)
            elif tpl[1].upper() == '-RRB-':
                new_sent.append((bracket_map[tpl[0]], ')'),)
            else:
                new_sent.append(tpl)
        new_tagged_sents.append(new_sent)
    return new_tagged_sents


VALID_TAGS = {'N': ['NN', 'NNS', 'NNP', 'NNPS'],
              'V': ['VB', 'VBD', 'VBN', 'VBZ', 'VBG', 'VBP'],
              'J': ['JJ', 'JJR', 'JJS'],
              'C': ['CC', 'CD'],
              'D': ['DT'],
              'E': ['EX'],
              'F': ['FW'],
              'I': ['IN'],
              'L': ['LS'],
              'M': ['MD'],
              'P': ['PDT', 'POS', 'PRP', 'PRP$'],
              'R': ['RB', 'RBR', 'RBS', 'RP'],
              'S': ['SYM'],
              'T': ['TO'],
              'U': ['UH'],
              'W': ['WDT', 'WP', 'WP$', 'WRB'],
              '.': ['.'],
              ',': [','],
              '(': ['('],
              ')': [')'],
              ':': [':'],
              ';': [';'],
              "'": ["''"]
              }


def filter_tagged_sents(tagged_sents, VALID_TAGS):
    # TODO: [Critical] Tiene que ser mucho más estricto el chequeo.
    """
Filter out sentences with unwanted/invalid tags, so as to normalize a tagged
corpus (the list of tagged sentences) to follow Penn Treebank POS tag encoding
scheme. Of course, this is only useful if not too many tagged sentences are
discarded from the input tagged corpus. If too many sentences are lost, it is
better to modify the offending tags with the transform_tags() function above.
    """
    new_tagged_sents = []
    invalid_sents = []
    for sent in tagged_sents:
        is_valid = True
        for tpl in sent:
            for char in VALID_TAGS.keys():
                if tpl[1].startswith(char) and tpl[1] not in VALID_TAGS[char]:
                    is_valid = False
        if is_valid is True:
            new_tagged_sents.append(sent)
        else:
            invalid_sents.append(sent)
    return new_tagged_sents, invalid_sents


def concat_corpus(*tagged_corpora):
    """
Function takes the input, a sequence of any number of tagged corpora (each
represented as a list of tagged sentences), and concatenates these lists of
tagged sentence as one big list, which it then returns as output.
    """
    new_tagged_sents = []
    for corpus in tagged_corpora:
        new_tagged_sents.extend(corpus)
    return new_tagged_sents


def train_backoff_tagger(train_sents, default_tag=None):
    """
This function returns a tagger composed of a backoff sequence of taggers, some
of which can be trained on the function input. The input is a list of tagged
sentences, where each sentence is a list of tuples consisting of token/tag
pairs -- the same data structure that is returned by the .tagged_sents() method
of NLTK's tagged corpora. It should only be executed once, and the output
object --a trained tagger-- should be pickled and saved for later use.
    """
    default_tagger = nltk.DefaultTagger(default_tag)
    regexp_patterns = [(r'\d+([,.]\d+)*', 'CD')]
    regexp_tagger = nltk.RegexpTagger(regexp_patterns, backoff=default_tagger)
    unigram_tagger = nltk.UnigramTagger(train_sents, backoff=regexp_tagger)
    bigram_tagger = nltk.BigramTagger(train_sents, backoff=unigram_tagger)
    trigram_tagger = nltk.TrigramTagger(train_sents, backoff=bigram_tagger)
    return trigram_tagger


def test_backoff_tagger(name_corpus_dict, data_split=0.9, default_tag=None):
    ## Backoff tagger accuracy (gold standard evaluation).
    all_tagged_sents = concat_corpus(*name_corpus_dict.values())
    random.shuffle(all_tagged_sents)
    split_at = int(len(all_tagged_sents) * data_split)
    train_sents = all_tagged_sents[:split_at]
    test_sents = all_tagged_sents[split_at:]
    trained_tagger = train_backoff_tagger(train_sents,
                                          default_tag=default_tag)
    print '\n\nGLOBAL ACCURACY:\t\t\t%.2f%%' \
        % trained_tagger.evaluate(test_sents)
    ## Cross-corpora accuracy.
    for key in name_corpus_dict.keys():
        test_sents = name_corpus_dict[key]
        train_pair = []
        train_sents = []
        for crp in [c for c in name_corpus_dict.keys() if c != key]:
            train_sents.extend(name_corpus_dict[crp])
            train_pair.append(crp)
        trained_tagger = train_backoff_tagger(train_sents,
                                              default_tag=default_tag)
        print '%s+%s | %s (%.2f%%) ACCURACY:\t%.2f%%' % (
            train_pair[0], train_pair[1], key,
            (len(test_sents) / len(all_tagged_sents)) * 100,
            trained_tagger.evaluate(test_sents))
    print


def pprint_tag_stats(tagged_sents, none_tag=None):
    tagged_tokens = []
    untagged_tokens = []
    for sent in tagged_sents:
        for tpl in sent:
            if tpl[1] == none_tag:
                untagged_tokens.append(tpl[0].lower())
            else:
                tagged_tokens.append(tpl[0].lower())
    total_num = sum(len(sent) for sent in tagged_sents)
    unique_num = len(set(tagged_tokens)) + len(set(untagged_tokens))
    print '===[ # SENTS: %s]===========' % len(tagged_sents)
    print '===[ TOTAL ]' + '=' * 20
    print '# of tagged tokens:\t%s\t%.2f%%' % \
        (len(tagged_tokens),
        (len(tagged_tokens) / total_num) * 100)
    print '# of untagged tokens:\t%s\t%.2f%%' % \
        (len(untagged_tokens),
        (len(untagged_tokens) / total_num) * 100)
    print '===[ UNIQUE ]' + '=' * 19
    print '# of tagged tokens:\t%s\t%.2f%%' % \
        (len(set(tagged_tokens)),
        (len(set(tagged_tokens)) / unique_num) * 100)
    print '# of untagged tokens:\t%s\t%.2f%%\n' % \
        (len(set(untagged_tokens)),
        (len(set(untagged_tokens)) / unique_num) * 100)
    print


def untagged_tokens_freq(tagged_sents, none_tag=None):
    """
Returns a dictionary ordered by frequency of tokens that could not be tagged.
    """
    freq_list = []
    for sent in tagged_sents:
        for tpl in sent:
            if tpl[1] == none_tag:
                freq_list.append(tpl[0].lower())
    return nltk.FreqDist(freq_list)


EPTMK_DICT = {'crd': 'NNP',
              'cesarean': 'JJ',
              'stockings': 'NNS',
              'cream': 'NN',
              'deep-vein': 'JJ',
              'variceal': 'JJ',
              'deep': 'JJ',
              'nurse': 'NN',
              'fissure': 'NN',
              'non-small': 'JJ',
              'double-blind': 'JJ',
              'folic': 'JJ',
              'pre-operative': 'JJ',
              'intravaginal': 'JJ',
              'non-small-cell': 'JJ',
              'vacuum': 'NN',
              'post-operative': 'JJ',
              'dihydroergotamine': 'NN',
              'olanzapine': 'NN',
              'canonization': 'NN',
              'sphincterotomy': 'NN',
              'thromboprophylaxis': 'NN',
              'venlafaxine': 'NN',
              'obsessive-compulsive': 'JJ',
              'plethysmography': 'NN',
              'chemoradiotherapy': 'NN',
              'monotherapy': 'NN',
              'worksite': 'NN',
              'neurosurgical': 'JJ',
              'dimensions': 'NNS',
              'findings': 'NNS',
              'mesh': 'NNP',
              'postcoital': 'JJ',
              'sick': 'JJ',
              'humidifier': 'NN',
              'poor-quality': 'JJ',
              'flunarizine': 'NN'
              }


def dict_tagger(tagged_sents, tagging_dict, none_tag=None):
    """
This function, given a list of POS-tagged sentences, adds tags to tokens that
do not have a tag (in other words, that could not be tagged by the backoff
tagger created by train_backoff_tagger() function above). Tags are added
following a token->tag mapping dictionary (where keys=tokens and values=tags).
If an untagged token is not specified in the mapping dictionary the token/tag
tuple is not modified. A new list of tagged sentences is returned.
    """
    new_tagged_sents = []
    for sent in tagged_sents:
        new_sent = []
        for tpl in sent:
            if tpl[1] == none_tag:
                if tpl[0].lower() in tagging_dict.keys():
                    new_sent.append((tpl[0], tagging_dict[tpl[0].lower()]))
                else:
                    new_sent.append(tpl)
            else:
                new_sent.append(tpl)
        new_tagged_sents.append(new_sent)
    return new_tagged_sents


def wordnet_tagger(tagged_sents, none_tag=None):
    """
wordnet_tagger() function docstring.
    """
    wnet = nltk.corpus.wordnet
    first_pass = []
    for sent in tagged_sents:
        new_sent = []
        for i in range(len(sent)):
            if not sent[i][1] == none_tag:
                new_sent.append(sent[i])
            else:
                wnss = wnet.synsets(sent[i][0].lower())
                if not wnss:
                    new_sent.append(sent[i])
                else:
                    pos_tags_set = set(ss.pos for ss in wnss)
                    if len(pos_tags_set) == 1:
                        if pos_tags_set == set(['n']):
                            # TODO: distinguish between NNP, NNS, NNPS
                            new_sent.append((sent[i][0], 'NN'),)
                        elif pos_tags_set == set(['v']):
                            # TODO: distinguish between conjugations.
                            new_sent.append((sent[i][0], 'VB'),)
                        elif pos_tags_set == set(['a']):
                            new_sent.append((sent[i][0], 'JJ'),)
                        elif pos_tags_set == set(['s']):
                            new_sent.append((sent[i][0], 'JJ'),)
                        elif pos_tags_set == set(['r']):
                            new_sent.append((sent[i][0], 'RB'),)
                        else:
                            new_sent.append(sent[i])
                    elif len(pos_tags_set) == 2:
                        if pos_tags_set == set(['a', 's']):
                            new_sent.append((sent[i][0], 'JJ'),)
                        else:
                            new_sent.append(sent[i])
                    else:
                        new_sent.append(sent[i])
        first_pass.append(new_sent)
    ret_sents = first_pass
    return ret_sents


def load_tagged_corpora(show_stats=True):
    """
Loads the taggaed corpora (GENIA, MedPost and Treebank), makes a few
changes to the tags, and returns them.
    """
    genia_tagged = genia.corpus_load().tagged_sents()
    mpost_tagged = medpost.corpus_load().tagged_sents()
    tbank_tagged = nltk.corpus.treebank.tagged_sents()

    genia_tagged = transform_tags(genia_tagged, GENIA_TAG_SUBS)
    genia_accept, genia_reject = \
        filter_tagged_sents(genia_tagged, VALID_TAGS)

    mpost_tagged = transform_tags(mpost_tagged, MPOST_TAG_SUBS)
    mpost_accept, mpost_reject = \
        filter_tagged_sents(mpost_tagged, VALID_TAGS)

    tbank_tagged = transform_tags(tbank_tagged, TBANK_TAG_SUBS)
    tbank_tagged = fix_tbank_brackets(tbank_tagged)
    for i in range(len(tbank_tagged)):    # Remove tokens/tags from sentences
        tbank_tagged[i] = [               # when tag == '-NONE-'. These
            w for w in tbank_tagged[i]    # shouldn't be there, it's a bug
            if w[1].upper() != '-NONE-']  # in how NLTK loads the corpus.
    tbank_accept, tbank_reject = \
        filter_tagged_sents(tbank_tagged, VALID_TAGS)

    if show_stats is True:
        print '\n\nGENIA sents: ', len(genia_tagged)
        print 'GENIA passed:', len(genia_accept)
        print 'GENIA reject:', len(genia_reject)
        print '\nMPOST sents: ', len(mpost_tagged)
        print 'MPOST passed:', len(mpost_accept)
        print 'MPOST reject:', len(mpost_reject)
        print '\nTBANK sents: ', len(tbank_tagged)
        print 'TBANK passed:', len(tbank_accept)
        print 'TBANK reject:', len(tbank_reject)

    return genia_accept, mpost_accept, tbank_accept


def save_tagger():
    genia_accept, mpost_accept, tbank_accept = \
        load_tagged_corpora()
    tagged_corpora = concat_corpus(genia_accept, mpost_accept, tbank_accept)
    trained_tagger = train_backoff_tagger(tagged_corpora, default_tag=None)
    test_backoff_tagger({'GENIA': genia_accept, 'MPOST': mpost_accept,
                         'TBANK': tbank_accept})
    if not os.path.isfile('tagger.pickle'):
        with open('tagger.pickle', 'wb') as tagger_file:
            print '\nPickled and saved POS tagger.\n'
            cPickle.dump(trained_tagger, tagger_file)
    else:
        print '\nFAILED saving tagger.pickle... File already exists!\n'


def tag_eptmk(show_stats=True):
    eptmk_untagged = epistemonikos.corpus_load().sents()
    if not os.path.isfile('tagger.pickle'):
        print ("\ntagger.pickle file doesn't exist! Use save_tagger() function"
               " to create it.\n")
    else:
        with open('tagger.pickle', 'rb') as tagger_file:
            print '\nLoaded and unpickled POS tagger.\n'
            trained_tagger = cPickle.load(tagger_file)

        eptmk_tagged = trained_tagger.batch_tag(eptmk_untagged)
        if show_stats is True:
            print 'BACKOFF:'
            pprint_tag_stats(eptmk_tagged, none_tag=None)

        eptmk_tagged = dict_tagger(eptmk_tagged, EPTMK_DICT, none_tag=None)
        if show_stats is True:
            print '+ DICT:'
            pprint_tag_stats(eptmk_tagged, none_tag=None)

        eptmk_tagged = wordnet_tagger(eptmk_tagged, none_tag=None)
        if show_stats is True:
            print '+ WNET:'
            pprint_tag_stats(eptmk_tagged, none_tag=None)

        return eptmk_tagged


def main():
    """
Before using this function, one has to create file tagger.pickle (the
trained pos tagger) with save_tagger() function.
    """
    eptmk_tagged = tag_eptmk(show_stats=False)
    return eptmk_tagged


if __name__ == '__main__':
    save_tagger()
    tag_eptmk(show_stats=True)
