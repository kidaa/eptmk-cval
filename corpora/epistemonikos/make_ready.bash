#!/bin/bash

mkdir ready2use
for f in `ls source/ids/*.txt`; do
    basename=$(echo $f| cut -d"/" -f3| cut -d"." -f1)
    title_file="source/titles/$basename.title.txt"
    abstr_file="source/abstracts/$basename.abstract.txt"
    title_text=`cat $title_file`
    abstr_text=`cat $abstr_file`
    echo -e "$title_text\n$abstr_text" > ready2use/$basename.txt
done
