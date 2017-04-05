#!/bin/bash

PRG="$0"
CURDIR="`pwd`"
# need this for relative symlinks
while [ -h "$PRG" ] ; do
  ls=`ls -ld "$PRG"`
  link=`expr "$ls" : '.*-> \(.*\)$'`
  if expr "$link" : '/.*' > /dev/null; then
    PRG="$link"
  else
    PRG=`dirname "$PRG"`"/$link"
  fi
done
SCRIPTDIR=`dirname "$PRG"`
SCRIPTDIR=`cd "$SCRIPTDIR"; pwd -P`
ROOTDIR=`cd "$SCRIPTDIR/.."; pwd -P`

SRC="$ROOTDIR"/source
BIN="$SCRIPTDIR"

gcc -O2 "$SRC"/gha_lsa.c -lm -o "$BIN"/gha_lsa
gcc -O2 "$SRC"/process_wordbag_corpus.c -lm -o "$BIN"/process_wordbag_corpus
gcc -O2 "$SRC"/process_wordbag_corpus_svdlibc.c -lm -o "$BIN"/process_wordbag_corpus_svdlibc
