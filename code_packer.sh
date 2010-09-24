#!/usr/bin/env bash

if [ -z "$1" ]; then
	echo "usage: code_packer.sh <milestone_number>"
else
	tar cvzf aos-m$1.tar.gz --exclude=.hg --exclude=.svn --exclude=*.pyc --exclude=*.o --exclude=build/* --exclude=.sconsign.dblite src
fi




