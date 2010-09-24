#!/usr/bin/python
#
# Authour: Philip O'Sullivan <philip.osullivan@nicta.com.au>
# Date   : Wed 26 Apr 2006
#

"""
This modules provides all the support for running unit tests in a Kenge project
using these build tools.
"""

import commands
import copy
import optparse
import time
import sys

__revision__ = '1.0'

# The version of OrderedDict in util is not used as it provides an items
# method that is different from a python dictionaries items method
class OrderedDict:
        """
        A very simple insertion ordered dictionary so we can gets out keys 
        in insertion order.
        """
        def __init__(self):
                self.dict = {}
                self.insert_order = []

        def __setitem__(self, key, value):
                self.dict[key] = value
                self.insert_order.append(key)

        def __getitem__(self, key):
                return self.dict[key]

        def keys(self):
                """
                D.keys() -> list of D's keys
                
                @rtype: list
                @return: list of the dictionaries keys
                """
                return self.insert_order

        def __copy__(self):
                new = OrderedDict()
                new.dict = copy.copy(self.dict)
                new.insert_order = copy.copy(self.insert_order)
                return new

        def values(self):
                """
                D.values() -> list of D's values

                @rtype: list
                @return: list of the dictionaries values
                """
                return [self.dict[key] for key in self.keys()]

        def items(self):
                """
                D.items() -> list of D's (key, value) pairs, as 2-tuples

                @rtype: list
                @return: list of key, value tuples in the dictionary
                """
                return [(key, self.dict[key]) for key in self.keys()]


def build_time_taken(duration):
        """
        Return a string representation of the time taken (mm:ss)

        @type duration: int
        @param duration: number of seconds in the time period

        @rtype: string
        @return: time in formatted string
        """
        secs = duration % 60
        mins = duration / 60
        mins = mins % 60
        return '%d:%02d' % (mins, secs)


class TestRun:
        """
        Encapsulates a single compile and simulate cycle for a given 
        configuration.

        To change how a configuration is tested just inherit from this case and
        redefine to appropriate methods.
        """
        def __init__(self, options, config):
                """
                Initialise an instance of the L{TestRun} class.

                @param options: The command line options as returned by L{get_options}
                @type options: optparse.Values
                """
                self.options = options
                self.config = config

                # The following three variables are not set until run_test has
                # completed.
                self.status = None
                self.duration = None
                self.output = None

        def build_dir(self):
                """
                From the test configuration return the name of the build 
                directory to use.

                @rtype: string
                @return: The build dir to use
                """
                if self.config.dict.has_key('machine'):
                        return 'build.%s' % self.config['machine']
                else:
                        return 'build'
                
        def log_name(self):
                """
                From the test configuration return the name of the log file.

                @rtype: string
                @return: Name of the log file to use
                """
                if self.config.dict.has_key('example'):
                        name = self.config['example']
                elif self.config.dict.has_key('test_lib'):
                        name = 'lib' + self.config['test_lib']
                elif self.config.dict.get('wombat', 'no') == 'yes':
                        name = 'wombat'
                else:
                        name = 'unknown'
                return '%s_%s.err' % (self.config['machine'], name)
                
        def command_options(self):
                """
                Return a string with the scons options for the config.

                @rtype: string
                @return: Options to scons
                """
                return ' '.join(['%s=%s' % (name, val) for name, val in self.config.items() if val])

        def command_line(self):
                """
                Return a string with the command line for scons for the config.

                @rtype: string
                @return: Shell command to build and run the test
                """
                build_dir = self.build_dir()
                self.config['build_dir'] = build_dir
                cmd = 'scons ' + self.command_options()
                if self.options.execute and not self.options.build:
                        cmd += ' simulate_test'
                return cmd

        def result_str(self):
                """
                Return a string representing the result appropriate for the
                output method for the given status.
                This is only valid after L{run_test} has completed.
                
                @rtype: string
                @return: String describing result of the test run
                """
                if self.options.log_output == 'plain':
                        if self.status:
                                return 'FAILED'
                        else:
                                return 'OK'
                elif self.options.log_output == 'terminal':
                        if self.status:
                                return '[31mFAILED[0m'
                        else:
                                return '[32mOK[0m' 
                elif self.options.log_output == 'html':
                        if self.status:
                                return '<font color="#aa0000">FAILED</font>'
                        else:
                                return '<font color="#00aa00">OK</font>'

        def log_str(self):
                """
                Return a string to be used in the summary to indicate log status

                @rtype: string
                @return: String to be used in summary to describe logging status
                """
                res = ''
                if self.options.log_method == 'file':
                        res = '-> LOG %s' % self.log_name()
                return res

        def duration_str(self):
                """
                Return a string representing the duration of the test run.
                This is only valid after L{run_test} has completed.

                @rtype: string
                @return: The duration of the test run
                """
                return build_time_taken(self.duration)
                
        def summary(self):
                """
                Return a string with a one line summary of the configuration

                @rtype: string
                @return: Short summary of the test's configuration
                """
                return self.command_options()

        def do_logging(self):
                """
                Log the output of the test using the appropriate format.
                """
                cmd_str = 'BUILD COMMAND: ' + self.command_line()
                if self.options.log_method == 'inline':
                        print cmd_str
                        print self.output
                elif self.options.log_method == 'file':
                        log = open(self.log_name(), 'w+')
                        log.write(cmd_str + '\n\n')
                        log.write(self.output)
                        log.close()

        def run_test(self):
                """
                Run the configuration and store the status, duration and output internally.
                """
                start = time.time()
                self.status, self.output = commands.getstatusoutput(self.command_line())
                self.duration = time.time() - start


class ConfigTree:
        """
        Iterator that is built from the config trees and the constraints
        that returns valid configurations in an OrderedDict.
        """
        def __init__(self, tree, constraints):
                self.tree = tree
                self.constraints = constraints
        def __to_dict(self, args):
                """
                Convert a list of (name, val) tuple to an OrderedDict.

                @type args: list[(string, string)]
                @param args: list of name, val tuples that define a 
                configuration

                @rtype: OrderedDict
                @return: dictionary (name -> val) that defines the 
                configuration
                """
                config = OrderedDict()
                for name, val in args:
                        config[name] = val
                return config

        def check_options(self, config):
                """
                Check the given config against the constraints.

                @type config: OrderedDict
                @param config: the dict holding the configuration for a test

                @rtype: Boolean
                @return: True if the config matches the constraints, false 
                otherwise
                """
                for name, vals in self.constraints.items():
                        if config.dict.get(name, None) not in vals:
                                return False
                return True

        def __iter__(self):
                """
                x.__iter__() <==> iter(x)

                @rtype: iterator
                @return: iterator for the collection
                """
                for args in self.tree:
                        config = self.__to_dict(args)
                        if not self.check_options(config):
                                continue
                        yield config
                return


def main(trees, test_run_builder=TestRun):
        """
        Given the trees of configuration options run each configuration and 
        report the results.

        This function does not return.  It exits with the apporpriate return
        value for the test run status.

        @param test_run_builder: Builder class for tests
        @type test_run_builder: L{TestRun}
        """
        options, constraints = get_options()
        ctree = ConfigTree(trees, constraints)
        total = 0
        for config in ctree:
                total += 1
        
        i = 1
        passes = 0
        failures = 0
        run_start = time.time()
        try:
                for config in ctree:
                        test = test_run_builder(options, config)
                        if options.execute:
                                print '%03d/%03d %s:' % (i, total, test.summary()),
                                sys.stdout.flush()
                                test.run_test()
                                if test.status:
                                        failures += 1
                                else:
                                        passes += 1
                                if (options.log_level == 'failures' and test.status) or \
                                    options.log_level == 'all':
                                        print '%s %s %s' % (test.result_str(), test.log_str(), test.duration_str())
                                        # We need to log something
                                        test.do_logging()
                                else:
                                        print '%s %s' % (test.result_str(), test.duration_str())
                        else:
                                print '%03d/%03d: %s' % (i, total, test.command_line())
                        i += 1
        except KeyboardInterrupt:
                print           # Make sure the summary is on a new line
        print 'RESULTS: %d passes, %d failures, %d tests; in %s' % \
                (passes, failures, total, build_time_taken(time.time()-run_start))
        sys.exit(failures != 0)

def get_options():
        """
        Parse the command line arguments and return the options and constraints.
        
        @rtype: optparse.Values
        @return: The options as parsed by optparse
        """
        usage = 'Usage: %prog [options] [name=val ...]'
        parser = optparse.OptionParser(usage=usage, version="%%prog %s" % __revision__)
        parser.add_option('-b', '--build', action='store_true', dest='build', 
                          help="Only build the target, don't simulate", default=False)
        parser.add_option('-n', '--no-exec', action='store_false', dest='execute', 
                          help='Don\'t execute any tests, just show the commands that would be run', 
                          default=True)
        log_output_choices = ['plain', 'html', 'terminal']
        parser.add_option('-o', '--log-output', action='store', dest='log_output', choices=log_output_choices,
                          default='terminal', 
                          help='Format output of log for a output device, choices are %s' %\
                            log_output_choices)
        log_method_choices = ['file', 'inline']
        parser.add_option('-m', '--log-method', action='store', dest='log_method', choices=log_method_choices, 
                          default='file', 
                          help='Method of output for log, choices are %s' % log_method_choices)
        log_level_choices = ['failures', 'all']
        parser.add_option('-l', '--log-level', action='store', dest='log_level', choices=log_level_choices, 
                          default='failures', help='Level of logging, choices are %s' % log_level_choices)
        options, args = parser.parse_args()
        constraints = {}
        for constraint in args:
                pair = constraint.split('=')
                if len(pair) != 2:
                        parser.error('invalid arg format: must be name=val')
                constraints[pair[0]] = pair[1].split(',')
        return options, constraints
