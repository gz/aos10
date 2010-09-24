#!/usr/bin/env python

import os, sys

if len(sys.argv) != 2:
    print "Usage: %s <config>" % sys.argv[0]
    sys.exit()

for each in os.popen("baz cat-config %s" % sys.argv[1]).readlines():
    dir, archive = each[:-1].split()
    arch_part = archive.split("/")[-1].split("--")
    #print len(arch_part), archive, arch_part
    if len(arch_part) != 3:
        print "BAD : Should specify verion only: %s" % archive
    else:
        latest = os.popen("baz revisions --reverse %s" % archive).readline()
        if not latest.startswith("version-"):
            print "BAD : Must specify only sealed versions: %s" % archive
        else:
            print "Good: %s" % archive

