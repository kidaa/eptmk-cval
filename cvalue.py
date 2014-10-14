#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import division
from collections import defaultdict
from math import log
import nltk
import tagger

# Step 1.
tagged_corpus = tagger.main()
new_tagged_corpus = []
for s in tagged_corpus:
    new_s = []
    for w in s:
        if w[1] is None:
            new_s.append((w[0], '-None-'),)
        else:
            new_s.append(w)
    new_tagged_corpus.append(new_s)

# Step 2 & 3.
term_candidate_pattern = 'TC: {<[JN].*|POS>+<N.*>}'
chunker = nltk.RegexpParser(term_candidate_pattern)
term_candidates = []
for sent in new_tagged_corpus:
    for chunk in [c for c in chunker.parse(sent).subtrees()
                  if str(c).startswith('(TC')]:
        term_candidates.append(' '.join([w.split('/')[0].lower() for w
                                         in str(chunk)[4:-1].split()]))

# Step 4.
# TODO: remove strings below freq. threshold

# Step 5.
stoplist = ['%', 'additional', 'apparent', 'author', 'authors', 'beneficial',
            'bias', 'considerable', 'contribution', 'excessive', 'explanation',
            'few', 'fewer', 'fewest', 'further', 'good', 'great', 'greater',
            'greatest', 'higher', 'highest', 'important', 'included', 'just',
            'larger', 'largest', 'less', 'lesser', 'main', 'modest', 'more',
            'most', 'necessary', 'numerous', 'other', 'possible', 'potential',
            'preliminary', 'present', 'publication', 'reason', 'relative',
            'several', 'significant', 'slight', 'smaller', 'smallest',
            'unnecessary', 'various', 'well-conducted']

filtered_candidates = []
for tc in term_candidates:
    for w in stoplist:
        if w in tc.split():
            break
    else:
        filtered_candidates.append(tc)

term_candidates_freq = defaultdict(int)
for tc in filtered_candidates:
    term_candidates_freq[tc] += 1


#TODO: min. cvalue threshold


def cvalue(freq_dict):
    term_cvalue = {}
    max_len_term = max(len(c.split()) for c in freq_dict.keys())
    for tc in [c for c in freq_dict.keys() if len(c.split()) == max_len_term]:
        cval = log(len(tc.split()), 2) * freq_dict[tc]
        term_cvalue[tc] = cval
    for term_len in reversed(range(1, max_len_term)):
        for tc in [c for c in freq_dict.keys() if len(c.split()) == term_len]:
            substring_of = [c for c in freq_dict.keys() if tc in c and tc != c]
            if len(substring_of) > 0:
                fa = freq_dict[tc]
                PTa = len(set(substring_of))
                fb = sum(freq_dict[c] for c in substring_of)
                for lc in substring_of:
                    for xc in [c for c in substring_of if lc != c]:
                        if lc in xc:
                            fb -= freq_dict[lc]
                #cval = log(len(tc.split()), 2) * (fa - fb) / PTa)
                cval = log(len(tc.split()), 2) * (fa - (1 / PTa * fb))  # same thing.
                term_cvalue[tc] = cval
            else:
                cval = log(len(tc.split()), 2) * freq_dict[tc]
                term_cvalue[tc] = cval
    return term_cvalue


if __name__ == '__main__':
    #test = {'basal cell carcinoma': 984,
            #'adenoid cystic basal cell carcinoma': 5,
            #'cystic basal cell carcinoma': 11,
            #'ulcerated basal cell carcinoma': 7,
            #'recurrent basal cell carcinoma': 5,
            #'circumscribed basal cell carcinoma': 3}
    #print cvalue(test)

    import operator
    terms_cvalue = cvalue(term_candidates_freq)
    cval_sorted = sorted(terms_cvalue.iteritems(),
                         key=operator.itemgetter(1),
                         reverse=True)
    threshold = 0
    for t in cval_sorted:
        if t[1] > threshold:
            print t[0], round(t[1], 2)
        else:
            break
