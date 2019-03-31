#!/bin/sh

wget https://github.com/wren-lang/wren/archive/master.zip -O wren-master.zip
unzip -q wren-master.zip
cd wren-master
make config=release vm
