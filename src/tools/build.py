#####################################################################
# 
# Australian Public Licence A (OZPLA)
# Version 0-1
# 
# Copyright (c) 2004 National ICT Australia Ltd
# All rights reserved.
# 
# Developed by: Embedded, Real-Time and Operating Systems Program (ERTOS)
#               National ICT Australia Ltd (NICTA)
#               http://ertos.nicta.com.au
# 
#  - Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimers.
#  - Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimers in the
#    documentation and/or other materials provided with the distribution.
#  - Neither the names of NICTA, nor the names of its
#    contributors, may be used to endorse or promote products derived from
#    this Software without specific prior written permission.
# 
# EXCEPT AS EXPRESSLY STATED IN THIS LICENCE AND TO THE FULL EXTENT
# PERMITTED BY APPLICABLE LAW, THE SOFTWARE IS PROVIDED "AS-IS", AND NICTA
# AND ITS CONTRIBUTORS MAKE NO REPRESENTATIONS, WARRANTIES OR CONDITIONS
# OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO ANY
# REPRESENTATIONS, WARRANTIES OR CONDITIONS REGARDING THE CONTENTS OR
# ACCURACY OF THE SOFTWARE, OR OF TITLE, MERCHANTABILITY, FITNESS FOR A
# PARTICULAR PURPOSE, NONINFRINGEMENT, THE ABSENCE OF LATENT OR OTHER
# DEFECTS, OR THE PRESENCE OR ABSENCE OF ERRORS, WHETHER OR NOT
# DISCOVERABLE.
# 
# TO THE FULL EXTENT PERMITTED BY APPLICABLE LAW, IN NO EVENT SHALL NICTA
# OR ITS CONTRIBUTORS BE LIABLE ON ANY LEGAL THEORY (INCLUDING, WITHOUT
# LIMITATION, IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHERWISE) FOR ANY
# CLAIM, LOSS, DAMAGES OR OTHER LIABILITY, INCLUDING (WITHOUT LIMITATION)
# LOSS OF PRODUCTION OR OPERATION TIME, LOSS, DAMAGE OR CORRUPTION OF DATA
# OR RECORDS; OR LOSS OF ANTICIPATED SAVINGS, OPPORTUNITY, REVENUE, PROFIT
# OR GOODWILL, OR OTHER ECONOMIC LOSS; OR ANY SPECIAL, INCIDENTAL,
# INDIRECT, CONSEQUENTIAL, PUNITIVE OR EXEMPLARY DAMAGES, ARISING OUT OF
# OR IN CONNECTION WITH THIS LICENCE, THE SOFTWARE OR THE USE OF OR OTHER
# DEALINGS WITH THE SOFTWARE, EVEN IF NICTA OR ITS CONTRIBUTORS HAVE BEEN
# ADVISED OF THE POSSIBILITY OF SUCH CLAIM, LOSS, DAMAGES OR OTHER
# LIABILITY.
# 
# If applicable legislation implies representations, warranties, or
# conditions, or imposes obligations or liability on NICTA or one of its
# contributors in respect of the Software that cannot be wholly or partly
# excluded, restricted or modified, the liability of NICTA or the
# contributor is limited, to the full extent permitted by the applicable
# legislation, at its option, to:
# 
# a. in the case of goods, any one or more of the following:
#    i.   the replacement of the goods or the supply of equivalent goods;
#    ii.  the repair of the goods;
#    iii. the payment of the cost of replacing the goods or of acquiring
#         equivalent goods; 
#    iv.  the payment of the cost of having the goods repaired; or
# b. in the case of services:
#    i.  the supplying of the services again; or
#    ii. the payment of the cost of having the services supplied again.
# 
# The construction, validity and performance of this licence shall be
# governed by the laws in force in the State of New South Wales,
# Australia.
#
#####################################################################

"""
This is the main implementation of the kenge build system.

People don't have to use it, but makes their life much easier
if they do.

Users should put an execfile("tools/build.py") at the top of their
SConstruct.
"""

######################################################################
# Library Import + Version checking
######################################################################

import sys
sys.path.append("tools") # Allow us to import bootimg.py

import SCons.Errors, os.path
import SCons.Defaults
import os, glob, copy, string, shutil, stat
import traceback
from cStringIO import StringIO
from types import *
from test_setup import TestEnvironment
from bootimg import BootImageBuilder, GrubBootImage, GrubFloppyImage, BuildMenuLst
from util import *
import sets

try:
    from pyelf import elf
    Export("elf")
except:
    print "Error import ELF. This is probably OK right now."

# Make it easier to raise UserErrors
UserError = SCons.Errors.UserError

# We want the SConsign file (where it stores
# information on hashs and times
SConsignFile(".sconsign")

# Work make the scons version easy to test
scons_version = tuple([int(x) for x in SCons.__version__.split(".")[:2]])
Export("scons_version")
Export("UserError")

# Make sure that we have a decent version of SCons
if scons_version <= (0, 95):
    raise UserError, "Support for SCons 0.95 has been dropped. Please upgrade to at least 0.96"


# We check we have at least version 2.3, If we don't we are in trouble!
if sys.version_info < (2, 3):
    raise UserError, "To use the Kenge build system you need Python2.3, including the python devel packages."

def pyc_clean(dir):
    """Remove all .pyc files from a directory"""
    def rmglob(arg, top, names):
        rmlist = [ top + os.path.sep + x for x in names if x.endswith(arg)]
        for x in rmlist:
            print "Removing", x
            os.remove(x)
    if cleaning:
        os.path.walk(dir, rmglob, ".pyc")

############################################################################
# Configuration functions
############################################################################

def add_help(text):
    """Add some text to the help"""
    global default_text
    default_text += text

def _add_arg(name, desc, default, options = None, as_list = False):
    def_val = default
    # Check if option is specified in the user config file, use it as default
    # unless overrided on the commandline
    if name in opts:
        def_val = opts[name]
    arg = ARGUMENTS.get(name, def_val)
    optstr = ""
    mapping = identity_mapping

    if options and type(options[0]) == TupleType:
        # This means a user has specified a list like:
        # [("foo", foo_object), ("bar", bar_object)]
        mapping = dict(options)
        options = [x[0] for x in options]

    if options and arg == None:
        if arg not in options:
            raise UserError, "need argument for option %s. Valid options are: %s" % (name, options)
        globals()[name] = mapping[arg];
    else:
        if options:
            arg = arg.split(",")
            for x in arg:
                if x not in options:
                    raise UserError, "%s is not a valid argument for option %s. Valid options are: %s" % (arg, name, options)

            if as_list:
                globals()[name] = arg;
            else:
                globals()[name] = mapping[arg[0]];

            optstr = " Valid options are:\n\t\t%s" % list2str(options)
        else:
            globals()[name] = mapping[arg];

    return "\t%s: %s%s\n\t\tdefault: %s\n\t\tcurrent: %s\n" % (name, desc, optstr, default, arg)

def _add_bool(name, desc, default, options = None):
    def_val = default
    # Check if option is specified in the user config file, use it as default
    # unless overrided on the commandline
    if name in opts:
        def_val = opts[name]
    x = ARGUMENTS.get(name, def_val)
    if type(x) is type(""):
        x.lower()
        if x in [1, True, "1", "true", "yes"]:
            x = True
        elif x in [0, False, "0", "false", "no"]:
            x = False
        else:
            raise UserError, "%s is not a valid argument for option %s. It should be True or False" % (x, name)

    globals()[name] = x;
    return "\t%s: %s (True/False)\n\t\tdefault: %s\n\t\tcurrent: %s\n" % (name, desc, default, x)

def add_arg(name, desc, default):
    global default_text
    default_text += _add_arg(name, desc, default, None)

def add_list(name, desc, default, options):
    global default_text
    default_text += _add_arg(name, desc, default, options)

def add_bool(name, desc, default):
    global default_text
    default_text += _add_bool(name, desc, default)

def get_bool(name):
    return globals()[name]

def add_config_help(text):
    # Function to add config specific args
    global config_text
    config_text += text

def add_config_choice(name, desc, default, options = None):
    global config_text
    config_text += _add_arg(name, desc, default, options)

def add_config_list(name, desc, default, options = None):
    global config_text
    config_text += _add_arg(name, desc, default, options, as_list = True)

def add_config_arg(name, desc, default):
    global config_text
    config_text += _add_arg(name, desc, default, None)

def get_arg(name):
    return globals()[name]

def add_config_bool(name, desc, default):
    global config_text
    config_text += _add_bool(name, desc, default)

def setup_help():
    """Print out help"""
    Help(default_text + config_text)

builtin_SConscript = SConscript

def SConscript(file_name, *args, **kargs):
    """We replace the default SConscript file with one that provides information
    on error, so users have a better idea of what is going on"""
    if not os.path.exists(file_name):
        if not os.path.exists(os.path.dirname(file_name)):
            error = "%s does not exists. Perhaps you forgot to download it?" % os.path.dirname(file_name)
        else:
            error = "SConscript %s not found" % file_name
        raise UserError, error
    if os.path.isdir(file_name):
        if "SConscript" in os.listdir(file_name):
            file_name = file_name + os.sep + "SConscript"
        elif "SConstruct" in os.listdir(file_name):
            file_name = file_name + os.sep + "SConstruct"
        else:
            raise UserError, "SConscript(%s) called, but niether SConscript or SConstruct file exists" % file_name
    return builtin_SConscript(file_name, *args, **kargs)

class LibraryNotFound(Exception):
    pass

def align(val):
    """Round up a value so its aligned on ALIGNMENT"""
    ovr = val % ALIGNMENT
    if (ovr):
        val = val + ALIGNMENT - ovr
    return val
Export("align")

def src_glob(search):
    """Src glob is used to find source files easily e.g: src_glob("src/*.c"),
    the reason we can't just use glob is due to the way SCons handles out
    of directory builds."""
    dir = os.path.dirname(search)
    if dir != "":
        dir = dir + os.sep
    src_path = Dir('.').srcnode().abspath
    files = glob.glob(src_path + os.sep + search)
    files = map(os.path.basename, files)
    ret_files = []
    for file in files:
        ret_files.append(dir + file)
    return ret_files

# We export it so the sub-SConstruct files can use it
Export("src_glob")

############################################################################
# Globals
############################################################################

# The default text for the build system
default_text = """
Kenge build system
Type: 'scons'                to build the default version
      'scons simulate'       to build and simulate the build OS
Options:
"""

config_text = ""

# new dictionary, the user configfile will add all the default commandline
# options into this list.
opts = {}

tool_prefix = "#tools/install"

# Determine if the user is trying to clean
cleaning = contains_any(["--clean", "--remove", "-c"], sys.argv)

generic_flags = " -g -nostdlib -nostdinc "

ALIGNMENT=0x10000

##########################################################################
# Get machine information
##########################################################################

import machines
available_machines = []
for each in dir(machines):
    attr = getattr(machines, each)
    if type(attr) == ClassType and issubclass(attr, machines.Machine) and not attr.virtual:
        available_machines.append(attr)

# Make all the machines globally accessable
from machines import *

# Put all the machines in the namespace so that user config file can
# access them easily.

##########################################################################
# Run user config
##########################################################################
def read_user_config(filename):
    try:
        execfile(filename, opts)
    except:
        print
        print "An error occured while reading you config file: %s. " % filename
        print "A traceback follows:" 
        print
        print "-" * 60
        traceback.print_exc()
        print "-" * 60
        raise UserError, "Cannot continnue after error in config file"

if os.path.exists(".conf"):
    read_user_config(".conf")

# Run ~/.sconf file
if "HOME" in os.environ and os.path.exists(os.environ["HOME"] + os.sep + ".sconf"):
    read_user_config(os.environ["HOME"] + os.sep + ".sconf")

add_arg("conf",
        "Use this as an alternative to build_dir."
        "It sets build_dir to build.$conf", None)

if conf is None:
    add_arg("build_dir", "Set to choose your build output directory", "build")
else:
    build_dir = "build.%s" % conf

#build_conf = build_dir + os.sep + ".conf"
if conf:
    conf_file = "%s.conf" % conf
    if not os.path.exists(conf_file):
        conf_file = os.path.join("configs", conf_file)
    if not os.path.exists(conf_file):
        raise UserError, "Couldn't find config %s" % conf

    read_user_config(conf_file)

############################################################################
# Configs
############################################################################

# We don't offer help to this argument because we don't
# really want it to be changed by the user
idl4_command = ARGUMENTS.get('idl4', 'PYTHONPATH=tools/install/lib tools/magpie/magpidl4.py --magpie-cache-dir=%(build_dir)s/magpie.cache --magpie-template-cache-dir=%(build_dir)s/magpie.templates' % {'build_dir':build_dir})

# Generic L4 options:

# Pistachio is built using "tools/autobuild $conf", this
# expects you to have a useful conf available.

def_l4conf = None

add_list("machine", "Set to your chosen machine.", None,
        [(machine.__name__, machine) for machine in available_machines])

inst_host = os.getenv("TFTPSERVER", "")
if (inst_host != ""):
    inst_host = inst_host + ":"

add_arg("install_dir", "Directory to install binary for netbooting",
        "%s/tftpboot/%s" % (inst_host, os.getenv("USER")))

default_installname = "bootimg"
if conf:
    default_installname += "." + conf
add_arg("install_name", "Name of installed binary", default_installname)

# FIXME: This is really only need for grub.
add_arg("boot_device", "Boot root device", "(nd)")
add_arg("boot_path", "Boot path", boot_device + install_dir.split(":")[-1])

add_arg("l4conf", "Specify an pistachio configuration", def_l4conf)
pistachio_configuration = l4conf

add_arg("toolprefix", "Specify the tool prefix", None)
add_arg("toolsuffix", "Specify the tool prefix", None)

# Endianness
if (hasattr(machine, "endian")):
    if machine.endian == "big":
        if machine.arch == "arm":
            machine.c_flags += ["-mbig-endian"]
        if machine.arch == "mips64":
            machine.c_flags += ["-EB"]
    elif machine.endian == "little":
        if machine.arch == "arm":
            machine.c_flags += ["-mlittle-endian"]
        if machine.arch == "mips64":
            machine.c_flags += ["-EL"]
    else:
            raise "unknown endianness %s" % machine.endian
else:
    print "using default compiler endian"

# Because we are trying to interact with make and don't do explicit
# dependencies, we need this. It basically means that for a dependency
# chain "a <- b <- c" we should explciitly check 'b' rather than 'c'
# when determining if 'a' needs to be  rebuilt. This is no good for us
# because for example, pistachio doesn't have any dependencies, so it
# doesn't to the rebuiling properly..
TargetSignatures("content")

####
# Clean out .pyc files from likely places they exist. This
# is admittedly seedy.
####
for directory in ["tools", "configs", "pistachio/contrib/cml2"]:
    pyc_clean(directory)

class buildl4:
    """buildl4 is a class used for building L4::Pistachio, using its
    autobuild scripts. "pistachio" argument is the directory in which
    pistachio resides, and configuration is the pistachio configuration
    to try and build."""

    def __init__(self, pistachio, configuration):
        self.pistachio = pistachio
        self.configuration = configuration
        
    def __call__(self, target, source, env):
        # Build L4.
        # Sometimes SCons is just a little bit too clever for its
        # own good - it creates the required directory, but our script
        # rlies on it not existing..
        if not os.path.exists(target[0].abspath):
            os.system("rm -fr %s/%s/pistachio" % (env.builddir, env.name))
        cmd = "(cd %s; BUILDDIR=%s/%s/pistachio/%s/ TOOLPREFIX=%s sh tools/autobuild %s)" % (self.pistachio,
		    env.builddir, env.name, self.configuration, env.toolchain, self.configuration)
        x = os.system(cmd)
        if x != 0:
            raise SCons.Errors.UserError, "Pistachio build failed"

def buildlinux(target, source, env):
    """Similar to buildl4, buildlinux is used to build linux using its
    existing Make based build system."""

    print("Building linux")

    # First we make sure linux is configured
    if not os.path.exists("%s/%s/wombat/.config" % (env.builddir, env.name)):
        os.system("cp wombat/%s %s/%s/wombat/.config" % (env.machine.wombat_config, env.builddir, env.name))
        os.system("make -C wombat O=%s/%s/wombat oldconfig" % (env.builddir, env.name))
    # Then we actually build it. We rely on the environment
    # to setup the actualy build command.
    print '***', target[0].linux_build_cmd, '***'
    x = os.system(target[0].linux_build_cmd)
    if x != 0:
        raise "L4Linux build failed"

# Dite stuff
def link_address(file, env):
    last_phys = None
    last_memsize = None
    on_next = 0
    for line in os.popen("%s -l %s" % (env["READELF"], file)).readlines():
        parts = line.split()
        if on_next == 1:
            last_memsize = parts[1]
            on_next = 0
        if len(parts) > 4 and parts[0] == "LOAD":
            last_phys = parts[3]
            last_memsize = parts[5]
        elif len(parts) and parts[0] == "LOAD":
            last_phys = parts[3]
            on_next = 1
    if last_phys is None:
        raise "Error finding end address"
    last_phys = eval(last_phys + "L")
    last_memsize = eval(last_memsize + "L")
    text = last_phys + last_memsize;

    if env.arch=="arm" and str(file).endswith("kernel"):
        text += 4 * 1024 * 1024 # Evil ARM hack!

    text = align(text)
    return text

class buildreloc:
    """Convert a relocatable ELF to a fixed position ELF, such that it falls at
    the end of the previous file"""
    
    def __init__(self, flags = 0):
        self.flags = flags

    def __call__(self, target, source, env):
        # Work out the next link address
        text = link_address(source[1], env)
        app = source[0]
        if self.flags != 'l':
            cmd = "%s -Ttext=0x%x %s -o %s" % (env.subst("$LINK $LINK_ARCH_FLAGS"), text, app, target[0])
        else:
            # This is really hideos way to make sure linux is linked at the right place
            # We copy the normal vmlinux -> vmlinux.tmp, recompile, copy new vmlinux ->
            # vmlinux.reloc, then copy vmlinux.tmp -> vmlinux, this avoids some rebuilds
            # in some situations. But maybe we should just build at the right address in
            # the first place?
            cmd = "cp %s %s.tmp; %s LINUX_START_ADDR=0x%x; cp %s %s; cp %s.tmp %s" % \
                  (app.get_abspath(), app.get_abspath(), app.linux_build_cmd,
                   text, app.get_abspath(), target[0], app.get_abspath(),
                   app.get_abspath())
	res = os.system(cmd)
        if res != 0:
            # We failed... so we better remove it incase someone tries to run it.
	    os.remove(app.get_abspath())
	return res

def markup(template, output, environment):
	"""
	This function reads a template. The template format is:
	{{python code:}} execute "python code"
	{{?python code}}: evaluate "python code" and replace the whole thing with the result.
	{{
	python code
	...
	}}: execute "python code". 
	"""		
	environment['out'] = output
	
	window_start = 0
	window_end = 0
	eof = len(template)

	STATE_NORMAL = 0
	STATE_EXECCODE = 1
	STATE_EVALCODE = 2
	STATE_LOOP = 3
	STATE_IF = 4

	state = STATE_NORMAL

	while window_start < eof:
		if state == STATE_NORMAL:
			window_end = template.find('{{', window_start)
			if window_end == -1:
				window_end = eof
			output.write(template[window_start:window_end])
			window_start = window_end
			if template[window_start:window_start + 9] == '{{ABORT}}':
				return False
			if template[window_start:window_start + 4] == '{{if':
				state = STATE_IF
				window_start += 4
			elif template[window_start:window_start + 3] == '{{?':
				state = STATE_EVALCODE
				window_start += 3
			elif template[window_start:window_start + 3] == '{{*':
				state = STATE_LOOP
				window_start += 3
			elif template[window_start:window_start + 2] == '{{':
				state = STATE_EXECCODE
				window_start += 2
		elif state == STATE_EVALCODE:
			window_end = template.find('}}', window_start)
			if window_end != -1:
				code = template[window_start:window_end]
				output.write(str(eval(code, environment)))
				window_start = window_end + 2
			state = STATE_NORMAL
		elif state == STATE_EXECCODE:
			window_end = template.find('}}', window_start)
			if window_end != -1:
				code = template[window_start:window_end]
				exec code in environment
				window_start = window_end + 2
			state = STATE_NORMAL
		elif state == STATE_IF:
			window_end = template.find('fi}}', window_start)
			ifcode_end = template.find(':', window_start)
			code = template[window_start:ifcode_end]
			if eval(code, environment):
				result = markup(template[ifcode_end+1:window_end], output, environment)
				if result is False:
					return False
			window_start = window_end + 4
			state = STATE_NORMAL
		elif state == STATE_LOOP:
			window_end = template.find('*}}', window_start)
			loopcode = template[window_start:window_end]
			for item in environment['LOOP']:
				environment['LOOPITEM'] = item
				result = markup(loopcode, output, environment)
				if result is False:
					return False
			window_start = window_end + 3
			state = STATE_NORMAL
	return True

add_config_bool("print_sim_log", "Print the simulator log for simulate_test on test pass?", False)
class KengeEnvironment(Environment):
    """KengeEnvironment is an extension of the SCons default Environment
    class. This is provides a convenient way of reducing duplication of
    build commands later, which means the build process should be less
    error prone"""

    src_exts = ["c", "cc", "S", "idl4", "template", "reg"]
    obj_exts = ["c", "cc", "S", "o"]    
    
    def __init__(self, BUILDDIR=build_dir, MACHINE=machine, TOOLCHAIN=None,
                 IDL4=idl4_command, **kargs):
        """We support two extra arguments:
        TOOLCHAIN: The prefix of our gcc toolchain, e.g: arm-linux-
        BUILDDIR: The directory in which to build the code.
        """
        global toolprefix
        global toolsuffix
        self.machine = machine
        self.arch = self.machine.arch
        if TOOLCHAIN is None:
            if toolsuffix is None and toolprefix is None:
                TOOLCHAIN = (self.machine.toolchain, "")
            else:
                if toolsuffix is None:
                    toolsuffix = ""
                if toolprefix is None:
                    toolprefix = ""
                TOOLCHAIN = (toolprefix, toolsuffix)

        self.toolchain = TOOLCHAIN
        
        def may_scp_copy(dest, source, env):
            print "may_scp_copy", dest, source
            if ":" in dest:
                cmd = "scp %s %s" % (source, dest)
                print "SCP", cmd
                os.system(cmd)
                return 0
            else:
                SCons.Defaults.copyFunc
                print "SHUTIL", (dest, source, env)
                return 0

        self.static = kargs.get("STATIC", 0)
        self.gcc_as_link = kargs.get("GCC_AS_LINK", 0)
        self.testenv = kargs.get("TESTENV", 0)
	self.test_lib = None

        self.install_dir = install_dir
        self.install_name = install_name

        kargs["EXPECT_TEST_DATA"] = None
        kargs["SELF"] = self

        kargs["CCFLAGS"] = ["-O2", "-g", "-nostdlib", "-nostdinc"]
        # This is the C compiler command to use
        kargs["CCCOM_NOTARGET"] =  "$CC $_CCFLAGS $_CCCOMCOM $SOURCES"
        kargs["CCCOM"] =  "$CC  -o $TARGET -c $_CCFLAGS  $_CCCOMCOM $SOURCES"
        kargs["CXXCOM"] = "$CXX -o $TARGET -c $_CXXFLAGS $_CCCOMCOM $SOURCES"

        kargs["_CCCOMCOM"] = "$CPPFLAGS $_CPPDEFFLAGS $_CPPINCFLAGS"

        kargs["ARCOMSTR"] = "=> Archiving $TARGET"
        kargs["CCCOMSTR"] = "=> Compiling $SOURCES"
        kargs["CXXCOMSTR"] = "=> Compiling $SOURCES"
        kargs["ASPPCOMSTR"] = "=> Compiling $SOURCES"
        kargs["LINKCOMSTR"] = "=> Linking $TARGET"
 
        # Setup to allow our customer CC line
        kargs["_CXXFLAGS"] = "              $_CXX_WARNINGS $CC_ARCH_FLAGS $_CXX_OPTIMISATIONS $CXXFLAGS"
        kargs["_CCFLAGS"] =  "$CC_STD_FLAGS $_CC_WARNINGS  $CC_ARCH_FLAGS $_CC_OPTIMISATIONS $CCFLAGS"

        # Setup to allow standard specification to be defined
        kargs["CC_STD"] = "gnu99"
        kargs["CC_STD_FLAGS"] = "--std=$CC_STD"
        # Setup to include warning on the compile line
        kargs["CC_WARNINGPREFIX"] = "-W"
        kargs["CC_WARNINGSUFFIX"] = ""
        kargs["CC_WARNINGS"] = ["all", "error", "strict-prototypes", "missing-prototypes",
                               "nested-externs", "missing-declarations", "redundant-decls",
                               "undef", "pointer-arith", "no-nonnull"]
        kargs["CXX_WARNINGS"] = ["all", "error", "redundant-decls",
                               "undef", "pointer-arith"]
        kargs["_CC_WARNINGS"] = "${_concat(CC_WARNINGPREFIX, CC_WARNINGS, CC_WARNINGSUFFIX, __env__)}"
        kargs["_CXX_WARNINGS"] = "${_concat(CC_WARNINGPREFIX, CXX_WARNINGS, CC_WARNINGSUFFIX, __env__)}"

        kargs["CC_OPTIMISATIONPREFIX"] = "-O"
        kargs["CC_OPTIMISATIONSUFFIX"] = ""
        kargs["_CC_OPTIMISATIONS"] = "${_concat(CC_OPTIMISATIONPREFIX, CC_OPTIMISATIONS, CC_OPTIMISATIONSUFFIX, __env__)}"

        kargs["CC_OPTIMISATIONPREFIX"] = "-O"
        kargs["CC_OPTIMISATIONSUFFIX"] = ""
        kargs["_CXX_OPTIMISATIONS"] = "${_concat(CC_OPTIMISATIONPREFIX, CXX_OPTIMISATIONS, CC_OPTIMISATIONSUFFIX, __env__)}"

        kargs["AS_ARCH_FLAGS"] = self.machine.as_flags
        kargs["CC_ARCH_FLAGS"] = self.machine.c_flags
        kargs["LINK_ARCH_FLAGS"] = self.machine.link_flags

        link_endian = ""
        if machine.endian == "big":
            link_endian = ["-EB"]
        if machine.endian == "little":
            link_endian = ["-EL"]
        kargs["LINK_ARCH_FLAGS"] += link_endian

        kargs["CC_ARCH_FLAGS"] = self.machine.c_flags
        kargs["CPPFLAGS"] = self.machine.cpp_flags
        kargs["INSTALL"] = may_scp_copy
        kargs["CPPPATH"] = []
        kargs["_LIBFLAGS"] = "-\\( ${_stripixes(LIBLINKPREFIX, LIBS, LIBLINKSUFFIX, LIBPREFIX, LIBSUFFIX, __env__)} -\\)"

        kargs["_LINKFLAGS"] =  "$LINKFLAGS $LINK_ARCH_FLAGS"
        kargs["LINKCOM"] = "$LINK $_LINKSCRIPTS $_LINKFLAGS $SOURCES $_LIBDIRFLAGS $_LIBFLAGS $LIBGCC -o $TARGET  "
        kargs["_LINKSCRIPTS"] = "$( ${_concat(LINK_SCRIPT_PREFIX, LINKSCRIPTS, LINK_SCRIPT_SUFFIX, __env__, _abspath_files)} $)"
        kargs["LINK_SCRIPT_SUFFIX"] = ""
        kargs["LINK_SCRIPT_PREFIX"] = "-T"

        if "FLINT_RUN" not in kargs: kargs["FLINT_RUN"] = 0

        kargs["FLINT"] = "flint"
        kargs["FLINT_SUPPRESS"] = []
        kargs["_FLINT_SUPPRESS"] = '${_concat("-e", FLINT_SUPPRESS, "", __env__)}'
        kargs["FLINT_OPTIONS"] = ["-b", # Suppress the banner
                                  "-u", # Do unit-at-a-time tests only
                                  "-w1", # Only really bad warning
                                  "\"+cpp(cc)\"",  # We use .cc as an extension
                                  "-d__asm__=_to_semi", # Eat up __asm__
                                  "\"+rw(_to_semi)\"",
                                  "-e46", # We allow bitfields on non integer size
                                  ]
        def _flint_extra (flint_run):
            if flint_run == 1:
                return "-zero"
            else:
                return ""
        kargs["_FLINT_EXTRA"] = _flint_extra
        kargs["_FLINT_OPTIONS"] = '${_FLINT_EXTRA(FLINT_RUN)} ${_concat("", FLINT_OPTIONS, "", __env__)}'
        kargs["FLINTCOM"] = "$FLINT $_FLINT_OPTIONS $_FLINT_SUPPRESS $_CPPDEFFLAGS $_CPPINCFLAGS  $SOURCE"

        def abspath_files(files):
            return [x.path for x in files]
        kargs["_abspath_files"] = abspath_files

        kargs["ASPPCOM"] = "$CC $ASPPFLAGS $AS_ARCH_FLAGS $CPPFLAGS $_CPPDEFFLAGS $_CPPINCFLAGS -c -o $TARGET $SOURCES"

        kargs["CPPCOM"] = "$CPP $_CCCOMCOM $SOURCES  -o $TARGET"

        kargs["BUILD_TESTS"] = kargs.get("BUILD_TESTS", False)
        if BUILDDIR:
            self.builddir = os.path.abspath(BUILDDIR)

        # Although SCons has good reasons for not using the default
        # path, it is so heavily used in practice that people expect it
        # and we use it as a default
        kargs['ENV'] = {}
        for envarg in ('PATH', 'PYTHONPATH'):
            try:
                kargs['ENV'][envarg] = os.environ[envarg]
            except KeyError, e:
                continue

        # Now we can set up the normal Environment stuff
        Environment.__init__(self, **kargs)

        if self.static:
            self["LINKFLAGS"] = " -static"

        self.set_toolchain(TOOLCHAIN)
	
	self["LIBGCC"] = '"%s"' % os.popen("%s %s --print-libgcc-file-name" % (self["CC"], ' '.join(self["CC_ARCH_FLAGS"]))).readline()[:-1]

        # Add the idl4 command
        if IDL4 is not None:
            self["IDL4"] = IDL4

        # Initialise our library and apps data structures
        self.libs = OrderedDict()
        self.apps = OrderedDict()
        # Now setup idl4, libgcc and templates

        self.cpp_path = []
        #self.real_cpp_path = []
        self.end_cpp_path = []

        if self.has_key("IDL4"):
            self.setup_idl4()
        self.setup_libgcc()
        self.setup_templates()

        # Finally the first MyEnvironment that is setup is called "based"
        # it is expected that new ones will be created with "Copy", which
        # specifies a new name
        self.name = "base"
        #self.arch = kargs["ARCH"]
        
        self.Append(CPPDEFINES = ["ARCH_%s" % self.machine.arch.upper()])
        self.Append(CPPDEFINES = ["MACHINE_%s" % self.machine.__name__.upper()])
        self.Append(CPPDEFINES = ["PLATFORM_%s" % self.machine.platform.upper()])
        if (hasattr(self.machine, "subplatform")):
            self.Append(CPPDEFINES = ["SUBPLATFORM_%s" % self.machine.subplatform.upper()])
        self.Append(CPPDEFINES = ["ENDIAN_%s " % self.machine.endian.upper()])

        # Setup custom builders

        # This is a custom builder for .reg files.
        def reg_build(target, source, env, for_signature):
            dir =  os.path.dirname(str(target[0]))
            cmd =  "python tools/dd_dsl.py --endian %s_ENDIAN -o %s %s" % (self.machine.endian.upper(), dir, source[0])
            return cmd

        def reg_targets(target, source, env):
            target_2 = str(target[0]).split(".")[0] + "_types.reg.h"
            target.append(target_2)
            return target, source

        reg_builder = self.Builder(generator = reg_build,
                                   emitter = reg_targets,
                                   src_suffix = ".reg",
                                   suffix = ".reg.h")
        self["BUILDERS"]["Reg"] = reg_builder

        # This is a custom builder for .idl4 files
        def idl4_build(target, source, env, for_signature):
            idl4_command =  self["IDL4"]
	    idl4_defines = ""
	    if machine.pidreloc == True:
		idl4_defines += "-DARM_PID_RELOC "

            dir =  os.path.dirname(str(target[0]))
            base_cmd = "%s %%s %s --with-cpp=`which %s` %s %s -h %%s -i v4nicta_n2 " \
                       "-fno-use-malloc -w %s -p generic_biguuid %s" % \
                       (idl4_command, source[0], env["CPP"], env["_CPPINCFLAGS"], env["_CPPDEFFLAGS"], env.machine.wordsize, idl4_defines)
            c_cmd = base_cmd % ("-c", target[1])
            s_cmd = base_cmd % ("-s", target[0])
            cmd = Action("%s && %s" % (c_cmd, s_cmd), mkstr("=> Generating IDL %s" % " ".join([str(x) for x in target])))
            return cmd

        def idl4_targets(target, source, env):
            targets = [str(target[0]) + "_server.h",
                       str(target[0]) + "_client.h"]
            return targets, source

        idl4_builder = self.Builder(generator = idl4_build,
                                    emitter = idl4_targets)
                                    #src_suffix = ".idl4")
        self["BUILDERS"]["IDL4"] = idl4_builder
        
        def build_from_template(target, source, env, for_signature = None):
            assert len(target) == 1
            assert len(source) == 1
            target = str(target[0])
            source = str(source[0])

            template = file(source).read()
            output = StringIO()
            result = markup(template, output, env["TEMPLATE_ENV"])
            assert result is True
            file(target, 'wb').write(output.getvalue())

        template_builder = self.Builder(action = build_from_template,
                                        src_suffix = ".template")
        self['BUILDERS']['Template'] = template_builder
        self["TEMPLATE_ENV"] = {}

        self.files = []

    def SConscript(self, file_name, *args, **kargs):
        """We replace the default SConscript file with one that provides information
        on error, so users have a better idea of what is going on"""

        if not os.path.exists(file_name):
            if not os.path.exists(os.path.dirname(file_name)):
                error = "%s does not exists. Perhaps you forgot to download it?" % os.path.dirname(file_name)
            else:
                error = "SConscript %s not found" % file_name
            raise UserError, error
        if os.path.isdir(file_name):
            if "SConscript" in os.listdir(file_name):
                file_name = file_name + os.sep + "SConscript"
            elif "SConstruct" in os.listdir(file_name):
                file_name = file_name + os.sep + "SConstruct"
            else:
                raise UserError, "SConscript(%s) called, but niether SConscript or SConstruct file exists" % file_name
        return Environment.SConscript(self, file_name, *args, **kargs)

    def add_files(self, files):
        self.files.append(files)
        self.files = Flatten(self.files)

    def get_files(self):
        return self.files

    def Copy(self, name, **kargs):
        """Create a new environment based on self.
        name is the name of the new environment
        libs is a list of libraries to put into this environment. Basically a shortcut
        for calling build_lids later"""
        if "TOOLCHAIN" in kargs and not kargs["TOOLCHAIN"] is None:
            self.set_toolchain(kargs["TOOLCHAIN"])
        if "STATIC" in kargs:
            self.static = kargs["STATIC"]
        else:
            self.static = 0
        if "TESTENV" in kargs:
            self.testenv = kargs["TESTENV"]
        else:
            self.testenv = 0
            
        new_self = Environment.Copy(self, **kargs)
        new_self.libs = copy.copy(self.libs)
        new_self.apps = copy.copy(self.apps)
        new_self.cpp_path = copy.copy(self.cpp_path)
        new_self.files = copy.copy(self.files)
        new_self.name = name
        #new_self.build_libs(*libs)
        return new_self

    def set_toolchain(self, toolchain, suffix=""):
        self.toolchain = toolchain[0]
        self["RANLIB"] = toolchain[0] + "ranlib"
        self["AR"] = toolchain[0] + "ar"
        self["CC"] = toolchain[0] + "gcc" + toolchain[1]
        self["CXX"] = toolchain[0] + "g++" + toolchain[1]
        self["CPP"] = toolchain[0] + "cpp" + toolchain[1]
        if self.gcc_as_link == 1:
            self["LINK"] = self["CC"]
        else:
            self["LINK"] = toolchain[0] + "ld"
        self["NM"] = toolchain[0] + "nm"
        self["READELF"] = toolchain[0] + "readelf"
        self["OBJCOPY"] = toolchain[0] + "objcopy"
        self["STRIP"] = toolchain[0] + "strip" 

    def setup_templates(self):
        """Setup the include path to allow the tempaltes to be included"""
        self.libs["template"] = ("#libs/templates/include", None, "")
        self["CPPPATH"].append("#libs/templates/include")

    def setup_libgcc(self):
        """Find the lication of libgcc.a and add it as a library"""
        libgccpath = os.path.dirname(os.popen("%s --print-libgcc-file-name" % self.subst("$CCCOM_NOTARGET")).read())
        self.libs["gcc"] = ([], 1, libgccpath, None, [])

    def setup_idl4(self):
        """Find the idl4 headers files and add"""
        #idl4_include = os.popen("idl4-config --include-dir").read()
        idl4_include = "#tools/magpie/include"
        self["CPPPATH"].append(idl4_include);
        self.libs["idl4"] = (idl4_include, None, "", None, [])

    def Pistachio(self, configuration=None, pistachio_dir="pistachio"):
        """Called to setup a pistachio target. Configuration is the name of the
        pistachio autoconf configuration, and pistachio_dir describes the location
        of pistachio source files. This uses the buildl4 class as the builder"""

        if configuration is None:
            configuration = self.machine.l4conf

        # These are the target files
        pistachio_files = ["%s/%s/pistachio/%s/kernel/%s-kernel" % (self.builddir, self.name,
                                                                    configuration, self.machine.arch)]
        # We need to make these precious so SCons doesn't delete them
        Precious(pistachio_files)

        l4kernel = pistachio_files[0]

        # The rule for how to build pistachio
        pistachio = self.Command(pistachio_files, [], buildl4(pistachio_dir, configuration))

        # Determining the dependancies for all of pistachio is quite expensive,
        # and we don't want to always rebuild this, because the pistachio build system
        # *always* rebuilds the kernel on a make, even if no file has changed.
        # So, we force the user to let us know when pistachio needs recompilation
        # by specifying "scons build_l4="
        add_arg("build_l4", "Set this option if you want to rebuild L4 on this build", 0)
        if build_l4 != 0:
            self.AlwaysBuild(pistachio)
        self.Depends(pistachio, self.Value(configuration))

        # We keep track of where pistachio is...
        self.pistachio = "%s/%s/pistachio/user" % (self.builddir, self.name)
        self.l4kernel = l4kernel

        if cleaning and os.path.exists("%s/%s/pistachio/%s/" % (self.builddir, self.name, configuration)):
            shutil.rmtree("%s/%s/pistachio/%s/" % (self.builddir, self.name, configuration))

        return l4kernel

    def linux_build_dir(self):
        return "%s/%s/build" % (self.builddir, self.name)

    def linux_install_dir(self):
        return "%s/base/install" % (self.builddir)

    def build_busybox(self, debug=0):
        """Set up build for busbox"""
        bdir =  "%s/busybox" % self.linux_build_dir()
        idir =  self.linux_install_dir()
        # Generate .config
        def busy_build(target, source, env):
            out_f = file(target[0].abspath, "w")
            in_f = file(source[0].abspath)
            for line in in_f.readlines():
                if "USING_CROSS_COMPILER" in line:
                    line = "USING_CROSS_COMPILER=y\n"
                elif "CONFIG_STATIC" in line and self.static:
                    line = "CONFIG_STATIC=y\n"
                elif "CONFIG_DEBUG" in line and debug:
                    line = "CONFIG_DEBUG=y\n"
                elif "CONFIG_NO_DEBUG_LIB" in line and debug:
                    line = "CONFIG_NO_DEBUG_LIB=y\n"
                elif "CROSS_COMPILER_PREFIX" in line:
                    line = 'CROSS_COMPILER_PREFIX="%s"\n' % self.toolchain
                elif "PREFIX" in line:
                    line = 'PREFIX="%s"\n' % idir
                out_f.write(line)

        self.Command("%s/.config" % bdir, "linux_apps/busybox/configs/defconfig", busy_build);
        self.Command("%s/include/config.h" % bdir, "%s/.config" % bdir,
                     "make -C linux_apps/busybox O=%s oldconfig" % bdir)
        self.Command("%s/busybox" % bdir, [Dir("linux_apps/busybox"),"%s/include/config.h" % bdir] ,
                     "make -C linux_apps/busybox O=%s" % bdir)
        bb = self.Command("%s/bin/busybox" % idir, "%s/busybox" % bdir ,
                          "make -C linux_apps/busybox O=%s install" % bdir)

        # Primitive rules on how to cleanup busybox
        if cleaning and os.path.exists(bdir):
            shutil.rmtree(bdir)
        if cleaning and os.path.exists(idir):
            shutil.rmtree(idir)

        if scons_version <= (0, 95):
            bb = [bb]

        return bb

    def build_linux_app(self, app, **kargs):
        """Method to allow specification of a Linux application."""
        app_builddir = "%s/%s" % (self.linux_build_dir(), app)
        
        # Export env and args to the app SConstruct
        env = self
        Export("env")
        args = kargs
        args["install_dir"] = self.linux_install_dir()
        args["build_dir"] = app_builddir
        Export("args")
        
        files = SConscript("%s/SConstruct" % app, build_dir=app_builddir, duplicate=0)
        return files

    def build_ramdisk(self, apps):
        ramdisk = "%s/%s/ramdisk" % (self.builddir, self.name)
        SConscript("tools/genext2fs/SConstruct", build_dir="tools/build/genext2fs", duplicate=0)

        args = {}
        args["install_dir"] = self.linux_install_dir()
        Export("args")
        default_files = SConscript("linux_apps/rootfs/SConstruct", build_dir=self.linux_build_dir(), duplicate=0)
        apps += default_files

        # Install compiler toolchain libraries

        # libc, libm, ld-linux
        def get_lib(lib):
            lib_fn = os.popen("%s --print-file-name=%s"  %  (self["CC"], lib)).read().strip()
            apps.append(Install("%s/lib" % self.linux_install_dir(), lib_fn))

        if self.static == 0:
            get_lib("libc.so.6")
            get_lib("libm.so.6")
            get_lib("libpthread.so.0")
            get_lib("libresolv.so.2")
            get_lib("libnss_dns.so.2")
            get_lib("libnss_files.so.2")
            get_lib("libutil.so.1")
            get_lib("libcrypt.so.1")
            # Evil!
            if self.arch in ["arm", "ia32"]:
                get_lib("ld-linux.so.2")
            else:
                get_lib("ld.so.1")
        
        disk_size = 8000
        
        cmd = self.Command(ramdisk, apps, "tools/build/genext2fs/genext2fs -b %s -d %s -f linux_apps/rootfs/dev.txt %s" % (disk_size, self.linux_install_dir(), ramdisk))
        self.Depends(cmd, "tools/build/genext2fs/genext2fs")
        # FIXME: Maybe we should use python GZip module here
        #cmd = self.Command("%s.gz" % ramdisk, ramdisk, "gzip %s" % ramdisk)
        # "%s.gz" % ramdisk
        return ramdisk

    def build_linux(self, **kargs):
        """Function to specify that L4Linux should be built. Currently we assume
        that it it in the l4linux directory under the root"""
        self.linux_files = ["%s/%s/wombat/vmlinux" % (self.builddir, self.name)]
        LIB_DEPENDS = [self.libs["mutex"][1]]
        LIB_DEPENDS += [self.libs["iguana"][1]]
        LIB_DEPENDS += [self.libs["l4"][1]]
        LIB_DEPENDS += [self.libs["timer"][1]]
        LIB_DEPENDS += [self.libs["l4e"][1]]
        LIB_DEPENDS += [self.libs["c"][1]]
        LIB_DEPENDS += [self.libs["circular_buffer"][1]]
        LIB_DEPENDS += [self.libs["ll"][1]]
        LIB_DEPENDS += [self.libs["range_fl"][1]]
        LIB_DEPENDS += [self.libs["naming"][1]]

        if "pxa" in self.machine.drivers:
            LIB_DEPENDS += [self.libs["pxa"][1]]
        
        l4linux = self.Command(self.linux_files, LIB_DEPENDS, buildlinux)
        l4linux = Flatten([l4linux])[0]
        Precious(self.linux_files)

	wombat_cflags = "-DENDIAN_%s " % self.machine.endian.upper()

	if machine.pidreloc == True:
            wombat_cflags += " -DARM_PID_RELOC "

        if restrict_vm == True:
            wombat_cflags += " -DCONFIG_RESTRICTED_VM=1 "

	if (hasattr(machine, "c_flags")):
	    wombat_cflags += ' '.join(machine.c_flags)

        # This is horrible :(
        mutex_include = os.getcwd() + os.sep + self.libs["mutex"][0][0][1:]
        ig_include = os.getcwd() + os.sep + self.libs["iguana"][0][0][1:]
        ig_idl4_include = self.libs["iguana"][0][-1]
        l4_include = os.getcwd() + os.sep + self.libs["l4"][0][0][1:]
        timer_include = os.getcwd() + os.sep + self.libs["timer"][0][0][1:]
        cb_include = os.getcwd() + os.sep + self.libs["circular_buffer"][0][0][1:]
        idl4_include = os.getcwd() + os.sep + self.libs["idl4"][0][1:] + os.sep
        naming_include = os.getcwd() + os.sep + self.libs["naming"][0][0][1:] + os.sep
        
        mutex_lib = os.getcwd() + os.sep + self.libs["mutex"][2][1:] + os.sep
        ig_lib = os.getcwd() + os.sep + self.libs["iguana"][2][1:] + os.sep
        l4_lib = os.getcwd() + os.sep + self.libs["l4"][2][1:] + os.sep
        timer_lib = os.getcwd() + os.sep + self.libs["timer"][2][1:] + os.sep
        l4e_lib = os.getcwd() + os.sep + self.libs["l4e"][2][1:] + os.sep
        c_lib = os.getcwd() + os.sep + self.libs["c"][2][1:] + os.sep
        cb_lib = os.getcwd() + os.sep + self.libs["circular_buffer"][2][1:] + os.sep
        ll_lib = os.getcwd() + os.sep + self.libs["ll"][2][1:] + os.sep
        rfl_lib = os.getcwd() + os.sep + self.libs["range_fl"][2][1:] + os.sep
        naming_lib = os.getcwd() + os.sep + self.libs["naming"][2][1:] + os.sep

        LIB_ARGS = ""
        LIB_ARGS += " LIBL4_INCLUDE=%s" % l4_include
        LIB_ARGS += " LIBTIMER_INCLUDE=%s" % timer_include
        LIB_ARGS += " LIBCB_INCLUDE=%s" % cb_include
        LIB_ARGS += " IGUANA_INCLUDE=%s" % ig_include
        LIB_ARGS += " IGUANA_IDL_INCLUDE=%s" % ig_idl4_include
        LIB_ARGS += " IDL4_INCLUDE=%s" % idl4_include
        LIB_ARGS += " NAMING_INCLUDE=%s" % naming_include
        LIB_ARGS += " MUTEX_INCLUDE=%s" % mutex_include
        if "pxa" in self.machine.drivers:
            pxa_include = os.getcwd() + os.sep + self.libs["pxa"][0][0][1:] + os.sep
            LIB_ARGS += " LIBPXA_INCLUDE=%s" % pxa_include

        LIB_ARGS += " LIBCDIR=%s" % c_lib
        LIB_ARGS += " LIBIGUANADIR=%s" % ig_lib
        LIB_ARGS += " LIBL4DIR=%s" % l4_lib
        LIB_ARGS += " LIBTIMERDIR=%s" % timer_lib
        LIB_ARGS += " LIBL4EDIR=%s" % l4e_lib
        LIB_ARGS += " LIBCBDIR=%s" % cb_lib
        LIB_ARGS += " LIBLLDIR=%s" % ll_lib
        LIB_ARGS += " LIBRANGE_FLDIR=%s" % rfl_lib
        LIB_ARGS += " LIBNAMINGDIR=%s" % naming_lib
        LIB_ARGS += " LIBMUTEXDIR=%s" % mutex_lib
        if "pxa" in self.machine.drivers:
            pxa_lib = os.getcwd() + os.sep + self.libs["pxa"][2][1:] + os.sep
            LIB_ARGS += " LIBPXADIR=%s" % pxa_lib

        l4linux.linux_build_cmd = "make -C wombat O=%s/%s/wombat WOMBAT_CFLAGS=\'%s\' V=0 %s " \
                                  "CROSS_COMPILE=%s " % \
                                  (self.builddir, self.name, wombat_cflags, LIB_ARGS, self.toolchain)

        if cleaning and os.path.exists("%s/%s/wombat" % (self.builddir, self.name)):
            shutil.rmtree("%s/%s/wombat" % (self.builddir, self.name))

        # As for pistachio we don't track the L4Linux dependencies so the
        # use needs to explicitly specify scons build_linux= to get L4Linux
        # rebuilt
        add_arg("build_linux", "Set this option if you want to rebuild Wombat on this build", 0)
        if build_linux != 0:
            AlwaysBuild(l4linux)

	env['EXPECT_TEST_DATA'] = [("Iguana init starting", None),
                                   ("Loading linux", None),
                                   ("Memory: \d+k/\d+k available", None),
                                   ("Please press Enter to activate this console.", None)]

        return l4linux

    def Bootimage(self, kernel, init, *args):
        """Build a bootimage we the specified loader, kernel, root task
        and applications."""

        # FIXME: I don't at all like the way this currently works. It is very kludgy
        # If you find a bug in here, you probably really want to talk to me (Benno)
        # because it is a very evil bit of code
        others = []
        for arg in args:
            if type(arg) == TupleType:
                others.append(arg)
            else:
                others.append((arg, ""))

        depends = [kernel]
        for binary in [init] + others:
            last = depends[-1]
            flags = ''
            if type(binary) == TupleType:
                flags = binary[1]
                binary = binary[0]
            if flags != "raw":
                def buildreloc_str(targets, source, env):
                    return "=> Relocating %s" % targets[0]
                new_binary = self.Command(str(binary) + ".reloc", [binary, last], Action(buildreloc(flags), buildreloc_str))
                # SCons changed its API :(
                if scons_version <= (0, 95):
                    new_binary = [new_binary]

                new_binary = new_binary[0]
            else:
                new_binary = binary
            last = new_binary
            depends.append(new_binary)

        def simulate(target, source, env):
            cmdline = env.machine.sim(target, source, env)
            os.system(cmdline)

        def simulatestr(target, source, env):
            return "=> Simulating %s" % source[0]

        def test(target, source, env):
	    def print_log(log):
	        print '--- SIMULATOR LOG ' + '-'*60
	        print log.getvalue()
	        print '-'*78
            test_data = env["EXPECT_TEST_DATA"]
            if test_data is None:
                raise UserError, "No expected test output supplied for this build"
            try:
                import pexpect
            except ImportError:
                raise UserError, "There was a problem importing the pexpect library.\n" \
                      "This is required for running the simulate_test target."
	    if pexpect.__version__ >= '2.0':
		log = StringIO()
		x = pexpect.spawn(env.machine.sim(target, source, env), timeout=float(timeout), logfile=log)
	    else:
		x = pexpect.spawn(env.machine.sim(target, source, env), timeout=float(timeout))
		log = StringIO()
		x.setlog(log)
            for in_, out_ in test_data:
                try:
                    x.expect(in_)
                except pexpect.ExceptionPexpect, e:
		    if isinstance(e, pexpect.TIMEOUT):
			print "Timed out waiting for: <%s>" % in_
		    elif isinstance(e, pexpect.EOF):
			print "Simulator exited while waiting for: <%s>" % in_
		    print_log(log)
                    raise UserError, "Failed test."
                if not out_ is None:
                    x.sendline(out_)
	    if print_sim_log:
	        print_log(log)
        def teststr(target, source, env):
            return "=> Testing %s" % source[0]

        if self.arch in ["arm", "mips64", "alpha", "powerpc", "powerpc64"]:
            self.dite = "tools/build/dite/dite"
            SConscript("tools/dite/src/SConstruct", build_dir="tools/build/dite", duplicate=0)
            cmd = self.Command("%s/bootimg.dite" % self.builddir, depends,
                               BootImageBuilder(self.arch, kernel,
                                                [(init, "i")] + others, dite_cmd=self.dite))
            self.Depends(cmd, self.dite)
            sim_cmd = cmd

            if self.machine.elfloader:
                # I don't like this hack at all, but seems to work.
                Export("cmd")
                if self.arch in ["arm"]:
                    self["CPPFLAGS"] = []
                elf_loader_env = self.Copy("elf-loader")
                elf_loader_env.AddLibrary("l4")
                elf_loader_env.AddLibrary("c", system = "baremetal")
                elf_loader_env.AddLibrary("elf")
                elf_loader = elf_loader_env.Application("loaders/elf-loader")
                cmd = elf_loader

            installed = []
            installed.append(self.InstallAs("%s/%s" % (self.install_dir, self.install_name), cmd))
            self.Alias("install", installed)
        elif self.arch == "ia32":
            # On ia32 we need kickstart
            kickstart_env = self.Copy("kickstart")
            kickstart_env.AddLibrary("l4")
            kickstart_env.AddLibrary("c", system="baremetal")
            kickstart_env.AddLibrary("elf")
            kickstart = kickstart_env.Application("loaders/kickstart")

            
            install_menulst = self.Command("%s/inst/menu.lst" % self.builddir, depends,
                                           BuildMenuLst, ROOT=boot_path)
            env.Depends(install_menulst, Value(boot_path))
            # Build a menu.lst
            installed = []
            installed.append(self.Install(self.install_dir, kickstart))
            installed.append(self.Install(self.install_dir, kernel))
            installed.append(self.Install(self.install_dir, install_menulst))
            for binary in [init] + others:
                flags = ""
                if type(binary) == TupleType:
                    flags = binary[1]
                    binary = binary[0]
                if flags != "raw":
                    installed.append(self.InstallAs("%s/%s" % (self.install_dir, os.path.basename(str(binary))), "%s.reloc" % binary))
                else:
                    installed.append(self.InstallAs("%s/%s" % (self.install_dir, os.path.basename(str(binary))), binary))                    
            self.Alias("install", installed)

            # Generate an image that can be used for the simulator...
            sim_menulst = self.Command("%s/sim/menu.lst" % self.builddir, depends,
                                       BuildMenuLst, ROOT="/boot/grub")
            simdeps = copy.copy(depends)
            simdeps.append(sim_menulst)
            simdeps.append(kickstart)
            mtools = SConscript("tools/mtools/SConstruct", build_dir="#tools/build/mtools",
                                duplicate=0, exports=["tool_prefix"])
            build_dir = "tools/build/grub"
            Export("build_dir")
            grub = SConscript("tools/grub/SConstruct", build_dir="#tools/build/grub",
                              duplicate=0, exports=["tool_prefix"])
            cmd = self.Command("%s/c.img" % self.builddir, simdeps, GrubBootImage)
            self.Depends(cmd, mtools)
            self.Depends(cmd, grub)
            sim_cmd = cmd

            # Generate an image that can be used for the usb
            usb_menulst = self.Command("%s/usbfloppy/menu.lst" % self.builddir, depends,
                                       BuildMenuLst, ROOT="/boot/grub")
            usbdeps = copy.copy(depends)
            usbdeps.append(usb_menulst)
            usbdeps.append(kickstart)
            build_dir = "tools/build/grub"
            Export("build_dir")
            grub = SConscript("tools/grub/SConstruct", build_dir="#tools/build/grub",
                              duplicate=0, exports=["tool_prefix"])
            cmd = self.Command("%s/usb.img" % self.builddir, usbdeps, GrubFloppyImage)
            self.Depends(cmd, mtools)
            self.Depends(cmd, grub)
            self.Alias("usbimage", cmd)

        self.Command("simulate", sim_cmd, Action(simulate, simulatestr))
        self.Command("simulate_test", sim_cmd, Action(test, teststr))
        return cmd

    def AddLibrary(self, lib, buildname=None, **kargs):
        """Used to specify that a library should be built for this environment.
        lib is the name of the library. If you want to build variants of one library
        source in the same environment set buildname. Any keyword arguments are passed
        on as arguments to the library build itself"""
        
        if buildname is None:
            buildname = lib
        lib_builddir = "%s/%s/libs/%s" % (self.builddir, self.name, buildname)

        # We export "env" and "args" to the library build file
        env = self
        Export("env")
        args = kargs
        Export("args")
        args["buildname"] = buildname
        # We call the libraries SConstruct file to build it.
        #if self.headers_only:
        #    self.cpp_path += SConscript("libs/%s/SConstruct" % lib, build_dir=lib_builddir, duplicate=0)
        #else:
        self.libs[buildname] = SConscript("libs/%s" % lib, build_dir=lib_builddir, duplicate=0)
        #self["_EXTRA_CPPPATH"] += 
        self["CPPPATH"] += self.libs[buildname][0]
        return self.libs[buildname]

    def Application(self, app, **kargs):
        """Method to allow specification of an application."""
        app_builddir = "%s/%s/%s" % (self.builddir, self.name, app)

        # Export env and args to the app SConstruct/home/benno/local//lib/scons-0.96/SCons/Node/FS.py
        env = self
        Export("env")
        args = kargs
        Export("args")

        # Call the apps SConstruct file to build it
        self.apps[app] = self.SConscript(app, build_dir=app_builddir, duplicate=0)
        return self.apps[app]

    def get_libs(self, lib, seen=None):
        if seen == None:
            seen = []
        if lib not in self.libs.keys():
            raise LibraryNotFound, lib

        # This little bit of magic is to allow us to have libraries
        # with just header files, no actual library code
        if self.libs[lib][1] is not None:
            libs = [lib]
        else:
            libs = []
        if lib in seen:
            return libs
        else:
            seen.append(lib)
        for dep in self.libs[lib][4]:
            libs += self.get_libs(dep, seen)
        return libs

    def KengeLibrary(self, name, buildname=None, source = None, public_headers = None, **kargs):
        """This is a replacement for the normal StaticLibrary method. However it
        provides some sensible defaults and works out most of it, assuming libraries
        have the simple default layout. The libraries on which this library depends
        should be listed"""
        library_args = {}

        library_args["CPPPATH"] = []

        if buildname is None:
            buildname = name

        if source is None:
            # User didn't provide any source files
            # explicitly, so we work out it form them
            # based on some hueristics.
            glob_list = []
            dirs = ["include/interfaces/", "src/", "src/arch-%s/" % env.arch]
	    if self.test_lib == name:
		dirs.append("test/")
            if self["BUILD_TESTS"]:
                dirs += ["test/"]
            for src_ext in env.src_exts:
                for dir_ in dirs:
                    glob_list.append(dir_ + "*." + src_ext)
        else:
            glob_list = source

        libs = []
            
        if "LIBS" in kargs:
            if self["BUILD_TESTS"]:
                kargs["LIBS"].append("check")
            for lib in kargs["LIBS"]:
                libs.append(lib)
                if lib not in self.libs.keys():
                    raise SCons.Errors.UserError, "Library [%s] was looking for library [%s] but it doesn't exist " \
                          "in environment [%s]\n This environment has: %s" % (name, lib, self.name, self.libs.keys())

            del kargs["LIBS"]

        # He we expand the glob to a list of files
        source_list = Flatten([src_glob(glob) for glob in glob_list])

        idl_files = [fn for fn in source_list if fn.endswith(".idl4")]
        reg_files = [fn for fn in source_list if fn.endswith(".reg")]

        # Now we go through everything in the kargs:
        for arg in kargs:
            if arg.startswith("EXTRA_"):
                argname = arg[6:]
                library_args[argname] = self[argname] + kargs[arg]
            else:
                library_args[arg] = kargs[arg]

        # Generally this is the only public headers
        if public_headers is None:
            public_headers = ["#libs/%s/include" % name, "#libs/%s/test" % name]

        if len(idl_files) or len(reg_files):
            # Unless we have generated files
            public_headers.append(Dir(".").abspath + "/include")

        # Now if we are for real compiling stuff...
        cpp_path = copy.copy(self["CPPPATH"])
	if self.test_lib:
		cpp_path.append("#libs/check/include")
        
        # Make sure we include any of the libraries header file's
        for each in public_headers:
            cpp_path.append(each)

        # This ensure that any generated header files
        # Maybe move this somewhere else later though
        cpp_path.append(Dir('.').abspath + "/src") # Broken
        cpp_path.append(Dir('.').abspath + "/include")

        # Find any .idl4 files that should be generated
        for file in idl_files:
            gen_file = self.IDL4(file)

        # Generate any .reg files
        for file in reg_files:
            self.Reg(file)
            
        library_args["CPPPATH"] += cpp_path + self.end_cpp_path # End cpp_path is a hack hack hack!
        
        # Now we just call the normal StaticLibrary with our simple defaults
        lib = self.StaticLibrary(buildname, source_list, **library_args)

        assert(len(lib) == 1)
        lib = lib[0]

        if self["FLINT_RUN"]:
            for each in lib.children():
                if str(each).endswith(".o"):
                    if str(each.children()[0]).endswith(".c") or \
                           str(each.children()[0]).endswith(".cc"):
                        self.AddPreAction(each, "$FLINTCOM")

        # And construct our definition of the library
        # This should suck muhc much less... how about a class?
        lib = (public_headers, lib, "#" + os.path.dirname(lib.path), None, libs)
        return lib

    # backwards compatability
    MyLibrary = KengeLibrary


    def KengeProgram(self, name, source = None, **kargs):
        """This is a replacement for the normal Program method. It works out most
        of the stuff for you"""
        program_args = {} 
        # we only want unique libraries, since re can't handle huge strings
        libs = sets.Set()
        libpath = []
        cpp_path = [] #copy.copy(self.cpp_path)

        # First we work out all the required libraries
        for lib in kargs.get("LIBS", []):
            try:
		libs.union_update(self.get_libs(lib))
            except LibraryNotFound, badlib:
                raise SCons.Errors.UserError, "Program %s was looking for library %s but it doesn't exist " \
                      "in this environment\n This environment has: %s" % (name, badlib, self.libs.keys())
	libs = list(libs)
        
        del kargs["LIBS"]

        # Now we go through to get the library path for all the
        # libraries
        for libname in libs:
            if not self.libs[libname][2] is None:
                libpath += [self.libs[libname][2]]

        # This ensure that any generated header files
        # Maybe move this somewhere else later though

        cpp_path.append(Dir('.').abspath + "/src")

        # Now we go through everything in the kargs:
        for arg in kargs:
            if arg.startswith("EXTRA_"):
                argname = arg[6:]
                program_args[argname] = self.get(argname, []) + kargs[arg]
            else:
                program_args[arg] = kargs[arg]

        if source is None:
            # User didn't provide any source files
            # explicitly, so we work out it form them
            # based on some hueristics.
            glob_list = []
            dirs = ["src/", "src/arch-%s/" % env.arch]
            for src_ext in env.src_exts:
                for dir_ in dirs:
                    glob_list.append(dir_ + "*." + src_ext)
        else:
            glob_list = source

        # He we expand the glob to a list of files
        source_list = Flatten([src_glob(glob) for glob in glob_list])

        # Now automatically handle any templates
        for file_name in source_list:
            if file_name.endswith(".template"):
                template_env = kargs.get("TEMPLATE_ENV", self["TEMPLATE_ENV"])
                template = self.Template(file_name, TEMPLATE_ENV=template_env)
                env.Depends(template, Value(template_env))
                source_list.append(str(template[0]))

        program_args["LIBS"] = libs
        program_args["LIBPATH"] = libpath

        object_list = []
        for source in source_list:
            for ext in self.obj_exts:
                if str(source).endswith(".%s" % ext):
                    object_list.append(source)
                    continue

        if kargs.has_key("EXTRAOBJECTS"):
            object_list += kargs["EXTRAOBJECTS"]

        # Prepend the crt
        if "c" in self.libs.dict.keys():
            object_list = self.libs["c"][3]+ object_list

        prog = self.Program(name, object_list, **program_args)

        # SCons changed to program returning a list of object. But it makes
        # much more sense to return a single item
        assert(len(prog) == 1)
        prog = prog[0]

        if "LINKSCRIPTS" in program_args:
            for linkscript in program_args["LINKSCRIPTS"]:
                self.Depends(prog, linkscript)

        if self["FLINT_RUN"]:
            for each in prog.children():
                if str(each).endswith(".o"):
                    if str(each.children()[0]).endswith(".c") or \
                           str(each.children()[0]).endswith(".cc"):
                        self.AddPreAction(each, "$FLINTCOM")

        return prog

    # For backwards compatability
    MyProgram = KengeProgram 

######################################################################################
# Setup the test environment
######################################################################################
tests_env = TestEnvironment()
# Copy src_glob reference. This is a bit hackish.
tests_env.src_glob = src_glob
Export('tests_env')
