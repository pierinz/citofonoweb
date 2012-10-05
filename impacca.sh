#!/bin/bash
version='0.9'
CWD="$(readlink -f `dirname $0`)"

rm -rf $CWD/../sandbox/*
mkdir -p $CWD/../sandbox/citofonoweb-$version

cd $CWD/src
rm -f *.tar.bz2
tar cjf citofonoweb-$version.tar.bz2 *
mv *.tar.bz2 ../../sandbox/citofonoweb-$version
cp -r * ../sandbox/citofonoweb-$version
cd ../../sandbox/citofonoweb-$version

DEBFULLNAME="Gabriele Martino" dh_make -m -e g.martino@osai-as.it -f citofonoweb-$version.tar.bz2
cp $CWD/debian-defs/* debian/
sed -i "s/##date/`date`/" debian/copyright
rm debian/*.ex
rm debian/README*
rm $CWD/../sandbox/*.orig.tar.gz

exit 0
