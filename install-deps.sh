#!/bin/sh

wget https://github.com/premake/premake-core/releases/download/v5.0.0-alpha8/premake-5.0.0-alpha8-linux.tar.gz -O /tmp/premake5.tar.gz
mkdir premake5
tar -zxf /tmp/premake5.tar.gz -C premake5/
export PATH=$PATH:$PWD/premake5

wget https://github.com/munificent/wren/archive/master.zip -O wren-master.zip
unzip wren-master.zip
cd wren-master
make vm
export PATH=$PATH:$PWD
