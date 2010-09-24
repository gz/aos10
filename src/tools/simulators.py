import os
import commands
import re

###################
# Simulator targets
###################

def qemu_sim(target, source, env):
    status, output = commands.getstatusoutput("qemu")
    version = re.search(r'QEMU PC emulator version ([0-9\.]+)', output).group(1)
    if version <= '0.7.0':
	return "qemu %s -hda %s -nics 0" % (" ".join(env.machine.qemu_args), source[0].abspath)
    else:
	return "qemu %s -hda %s -net nic -net tap" % (" ".join(env.machine.qemu_args), source[0].abspath)

def skyeye_sim(target, source, env):
    return "skyeye -c tools/%s -e %s" % (env.machine.skyeye, source[0].abspath)

def sulima_sim(target, source, env):
    return "u4600 %s" % source[0].abspath

def m5_sim(target, source, env):
    return "m5.opt %s" % source[0].abspath
