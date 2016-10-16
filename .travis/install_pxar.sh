#!/bin/bash

# This package is necessary for the CMS pixel option

echo "Entering install_pxar"
echo "Installing pxar library"

export temporary_path=`pwd`

cd --

git clone https://github.com/simonspa/pxar.git

cd pxar

git checkout testbeam-2016

mkdir build

cd build

cmake -DBUILD_pxarui=OFF -DBUILD_dtbemulator=ON ..

make install

export PXARPATH=~/pxar

cd $temporary_path

echo "Installed pxar library"
