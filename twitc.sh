#!/bin/bash
if [ -z "$1" ]; then
    echo "Usage: twitc <file.twit>"
    exit 1
fi

BASE=$(basename "$1" .twit)
DIR=$(dirname "$1")

./build/twitc "$1" && \
llc output.ll -o "$DIR/$BASE.s" && \
clang "$DIR/$BASE.s" -o "$DIR/$BASE" && \
echo "Done! -> $DIR/$BASE"
