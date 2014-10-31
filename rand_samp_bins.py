#!/usr/bin/env python
# -*- coding: UTF-8 -*-


import random
import os


def binning(data_seq, num_bins):
    """
Receives a data sequence and an integer for the number of bins as input.
Returns the binned data as a list of bins.
    """
    bins = []
    split = (1.0/num_bins)*len(data_seq)
    for i in range(num_bins):
        bins.append(data_seq[int(round(i*split)):int(round((i+1)*split))])
    return bins


def rand_sample(bin_data_list, samp_size):
    """
Receives a list of multi-word term candidates followed by their c-value score
[str('[multi-word candidate] [score]')] and an integer for the sample size.
Returns a random sample of unique term candidates of the specified size as a
list sorted by c-value score.
    """
    sample = random.sample(bin_data_list, samp_size)
    sample = [l.rsplit(' ', 1)[::-1] for l in sample]
    sample = [(float(t[0]), t[1]) for t in sample]
    sample = sorted(sample, reverse=True)
    sample = [' '.join((l[1], str(l[0]))) for l in sample]
    return sample


def main():
    with open('cvalue.txt', 'r') as f:
        terms = [l.strip() for l in f.readlines()]
    bins = binning(terms, 4)
    os.mkdir('binned')
    for b in enumerate(bins):
        samp1k = rand_sample(b[1], 1000)
        with open('binned/cval_bin%s.txt' % str(b[0]+1), 'w') as f:
            f.write('\n'.join(samp1k))


if __name__ == '__main__':
    main()
