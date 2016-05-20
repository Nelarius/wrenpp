#!/bin/sh

wget https://github.com/munificent/wren/archive/master.zip -O wren-master.zip
unzip wren-master.zip
cd wren-master
make vm
