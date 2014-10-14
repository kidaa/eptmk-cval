#!/bin/bash

mkdir tagged_tbank
for fpath in `ls tagged/*.ioc`; do
    fnamebase=$(echo $fpath| cut -d"." -f1| cut -d"/" -f2)
    fnamebase="tagged_tbank/$fnamebase.ptb"
    perl util/medpost2tag.perl -penn -preserve $fpath > $fnamebase
done
