#!/usr/bin/env python
# -*- coding: UTF-8 -*-


import os
import sys


# Path definitions.
CORPUS_BASE = '/'.join(os.path.realpath(__file__).split('/')[:-1])
POSTAG_FILE = CORPUS_BASE + '/tagged/GENIAcorpus3.02.pos.txt'
READY_PATH = CORPUS_BASE + '/ready2use'

if not os.path.isdir(READY_PATH):
    print >> sys.stderr, ('\nThe directory to save the processed files does '
                          'not exist.\nDOING NOTHING & EXITING SCRIPT.\n')
    sys.exit()

with open(POSTAG_FILE, 'r') as txt_file:
    raw_text = txt_file.read()
abstracts = raw_text.split('UI/LS\n-/:\n')[1:]
for a in abstracts:
    head, body = a.split('AB/LS\n-/:\n')
    aid, title = head.split('TI/LS\n-/:\n')
    aid = aid.split('\n')[0].split('/')[0]
    title = title.split('\n' + '=' * 20 + '\n')[:-1]
    #title = [sent.strip().replace('\n', ' ') for sent in title]
    if '\n.' + '=' * 20 + '\n' in body:
        body = body.replace('.=====', './.\n=====')
    sents = body.split('\n' + '=' * 20 + '\n')[:-1]
    #sents = [sent.strip().replace('\n', ' ') for sent in sents]
    op_str = '\n=====\n'.join(title) + '\n\n' + '\n=====\n'.join(sents)
    with open(READY_PATH + '/' + aid + '.txt', 'w') as op_file:
        op_file.write(op_str)
