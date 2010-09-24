#!/usr/bin/env bash

if [ -z "$1" ]; then
	echo "usage: code_packer.sh <milestone_number>"
else
	tar cvzf aos-m$1.tar.gz --exclude=.hg --exclude=.svn src
fi




