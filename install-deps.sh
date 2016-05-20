#!/bin/sh

wget https://github.com/premake/premake-core/releases/download/v5.0.0-alpha8/premake-5.0.0-alpha8-linux.tar.gz -O /tmp/premake5.tar.gz
tar -zxf /tmp/premake5.tar.gz -C ./

wget https://github.com/munificent/wren/archive/master.zip -O wren-master.zip
unzip wren-master.zip
cd wren-master
make vm

