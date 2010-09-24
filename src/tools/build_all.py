#!/usr/bin/env python

import glob, os, sys, commands
import optparse, re
from select import select

target = "simulate_test"

# This only works on Linux
def get_conf_name(filename):
    return os.path.splitext(os.path.split(filename)[-1])[0]

def build_conf(conf):
    print '%s:' % conf,
    sys.stdout.flush()
    build_line = "scons -Q conf=%s %s" % (conf, target)
    status, output = commands.getstatusoutput(build_line)
    if status == 0:
        print "OK"
    else:
        print "FAILED, output in %s.output.err" % conf
        open("%s.output.err" % conf, "w").writelines(output)
    return status

def main(options):
    global target
    all_confs = map(get_conf_name, glob.glob("*.conf") + glob.glob("configs/*.conf"))
    if options.build:
	target = ''

    n_tests = 0
    n_failures = 0
    for conf in all_confs:
	m = re.match('([^_]*)_(.*)', conf)
	if m:
	    platform, test = m.groups()
	    if options.platform and platform != options.platform:
		continue
	    if options.unittests and test[:3] != 'lib':
		continue
	    if options.test and test != options.test:
		continue
	    n_tests += 1
	    res = build_conf(conf)
	else:
	    print '%s: Don\'t know how to parse this config'
	    res = 1
	if res:
	    n_failures += 1
    if n_failures:
	print 'Some tests failed: %d failures out of %d tests' % (n_failures, n_tests)
	sys.exit(1)
    else:
	print 'ALL TESTS PASSED!'

if __name__ == '__main__':
    usage = 'Usage: %prog [options]'
    parser = optparse.OptionParser(usage=usage)
    parser.add_option('-p', '--platform', dest='platform', help='Run tests only on PLATFORM')
    parser.add_option('-u', '--unittests', action='store_true', dest='unittests', help='Run only unit tests', default=False)
    parser.add_option('-b', '--build', action='store_true', dest='build', help="Only build the target, don't simulate", default=False)
    parser.add_option('-t', '--test', dest='test', help='Architecture independent test name to run e.g. naming or libiguana')
    options, args = parser.parse_args()
    if args:
	parser.error('build_all.py accepts no arguements')
    main(options)
