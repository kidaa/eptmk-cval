#!/usr/bin/env python
# -*- coding: utf-8 -*-

from collections import defaultdict
from corpora import epistemonikos
from nltk.corpus import stopwords

eptmk = epistemonikos.corpus_load().words()
words = [w.lower() for w in eptmk if w.isalpha()]

word_freq = defaultdict(int)
for w in words:
    word_freq[w] += 1
word_freq = [(v, k) for k, v in word_freq.items()]
word_freq = sorted(word_freq, reverse=True)
for w in word_freq[:50]: print w
raw_input('press ENTER')

stoplist = stopwords.words('english')
nonfunc_freq = defaultdict(int)
for w in [x for x in words if x not in stoplist]:
    nonfunc_freq[w] += 1
nonfunc_freq = [(v, k) for k, v in nonfunc_freq.items()]
nonfunc_freq = sorted(nonfunc_freq, reverse=True)
for w in nonfunc_freq[:50]: print w
