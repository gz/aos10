import os
import SCons

GRUB_BIN = "tools/install/bin/grub"
GRUB_STAGE_1 = "tools/install/boot/grub/stage1"
GRUB_STAGE_2 = "tools/install/boot/grub/stage2"
GRUB_STAGE_FAT = "tools/install/boot/grub/fat_stage1_5"

ALIGNMENT=0x8000

def align(val):
    ovr = val % ALIGNMENT
    if (ovr):
        val = val + ALIGNMENT - ovr
    return val

def get_symbol_addr(binary, sym, env):
    for line in os.popen("%s %s" % (env["NM"], binary)).readlines():
        add, state, name = line[:-1].split()
        if name == sym:
            return add
    return None

def lstrip_path(p, cp):
    """lstrip_path(p, cp) -> str
       If p starts with cp, return the part of s remaining after
       cp and any leading /. Otherwise, return the original string"""
    if p.startswith(cp):
       return p[len(cp):].lstrip('/')
    return p

def relpath(file):
    """Takes in a file and returns the path relative to the current directory.
       Only works if file is not above the current directory."""
    abspath = os.path.abspath(str(file))
    abscurpath = os.path.abspath(os.curdir)
    commonpath = os.path.commonprefix((abspath, abscurpath))
    return lstrip_path(abspath, commonpath)

#serial --port=0x3f8 --speed=115200
menulist_serial = """
serial --unit=0 --stop=1 --speed=115200 --parity=no --word=8
terminal --timeout=10 serial console
"""

menulst_data = """
default 0
timeout = 5
title = L4 NICTA::Iguana
kernel=%s/kickstart
"""

def BuildMenuLst(target, source, env):
    menulst = target[0]
    dir = env["ROOT"]
    
    menu = menulst_data % dir

    if env.machine.serial_console:
        menu = menulist_serial + menu
        
    for each in source:
        if "kickstart" not in each.abspath:
            basename  = os.path.basename(each.abspath)
            if basename.endswith(".reloc"):
                basename = basename[:-6]
            menu += "\nmodule=%s/%s" % (dir, basename)
    open(str(menulst), "w").write(menu)

def GrubFloppyImage(target, source, env):
    output_image = target[0]

    bmap = "%s/bmap" % env.builddir
    mtoolsrc = "%s/mtoolsrc" % env.builddir

    open(bmap, "w").write("(fd0) %s" % output_image)
    open(mtoolsrc, "w").write('drive a: file="%s"\n' % output_image)

    def mtoolcmd(str):
        return os.system("MTOOLSRC=%s tools/install/bin/%s" % (mtoolsrc, str))

    def copy(file):
        # Hack! If file ends in .reloc, strip it!
        if file.abspath.endswith(".reloc"):
            filenm = file.abspath[:-6]
        elif file.abspath.endswith(".gz"):
            filenm = file.abspath[:-3]
        else:
            filenm = file.abspath
        x = mtoolcmd("mcopy -o %s a:/boot/grub/%s" % (file.abspath, os.path.basename(filenm)))
        if x != 0:
            os.system("rm %s" % output_image)
            raise SCons.Errors.UserError, "Bootimage disk full"

    size = 40000
    sectors = size * 1024 / 512
    tracks = sectors / 64 / 16

    os.system("dd if=/dev/zero of=%s count=%s bs=512" % (output_image, sectors) ) # Create image
    mtoolcmd("mformat -t %s -h 16 -n 63 a:" % tracks) # Format a:
    mtoolcmd("mmd a:/boot")
    mtoolcmd("mmd a:/boot/grub")
    mtoolcmd("mcopy %s a:/boot/grub" % GRUB_STAGE_1)
    mtoolcmd("mcopy %s a:/boot/grub" % GRUB_STAGE_2)
    mtoolcmd("mcopy %s a:/boot/grub" % GRUB_STAGE_FAT)

    os.system("""echo "root (fd0)
              setup (fd0)" | %s --device-map=%s --batch
              """ % (GRUB_BIN, bmap))

    for each in source:
        copy(each)

def GrubBootImage(target, source, env):
    output_image = target[0]

    bmap = "%s/bmap" % env.builddir
    mtoolsrc = "%s/mtoolsrc" % env.builddir

    open(bmap, "w").write("(hd0) %s" % output_image)
    open(mtoolsrc, "w").write('drive c: file="%s" partition=1\n' % output_image)

    def mtoolcmd(str):
        return os.system("MTOOLSRC=%s tools/install/bin/%s" % (mtoolsrc, str))

    def copy(file):
        # Hack! If file ends in .reloc, strip it!
        if file.abspath.endswith(".reloc"):
            filenm = file.abspath[:-6]
        elif file.abspath.endswith(".gz"):
            filenm = file.abspath[:-3]
        else:
            filenm = file.abspath

        x = mtoolcmd("mcopy -o %s c:/boot/grub/%s" % (file.abspath, os.path.basename(filenm)))
        if x != 0:
            os.system("rm %s" % output_image)
            raise SCons.Errors.UserError, "Bootimage disk full"

    os.system("dd if=/dev/zero of=%s count=088704 bs=512" % output_image) # Create image
    mtoolcmd("mpartition -I c:") 
    mtoolcmd("mpartition -c -t 88 -h 16 -s 63 c:") # 
    mtoolcmd("mformat c:") # Format c:
    mtoolcmd("mmd c:/boot")
    mtoolcmd("mmd c:/boot/grub")
    mtoolcmd("mcopy %s c:/boot/grub" % GRUB_STAGE_1)
    mtoolcmd("mcopy %s c:/boot/grub" % GRUB_STAGE_2)
    mtoolcmd("mcopy %s c:/boot/grub" % GRUB_STAGE_FAT)

    os.system("""echo "geometry (hd0) 88 16 63
              root (hd0,0)
              setup (hd0)" | %s --device-map=%s --batch
              """ % (GRUB_BIN, bmap))

    for each in source:
        copy(each)

class BootImageBuilder:
    def __init__(self, arch, kernel, rest, dite_cmd="dite"):
        self.arch = arch
        self.kernel = kernel
        self.rest = rest
        self.cmd = dite_cmd

    def __call__(self, target, source, env):
        kernel_kip = get_symbol_addr(self.kernel, "kip", env)
        patch_kcp = ""
        extra_start = ""
        dite_dict = {'kernel_kip': kernel_kip, 'kernel': self.kernel}
        if self.arch == "arm":
            phy_kernel = "-p"
        elif self.arch == "ia32":
            phy_kernel = ""
            patch_kcp = "--kcp-mainmem-low=0x100000 --kcp-mainmem-high=0xff00000"
        elif self.arch == "mips64":
            phy_kernel = "-p"
            extra_start = " --elf-modify-virtaddr=0xffffffff80000000 --elf-modify-physaddr=0xffffffff80000000 --kcp-mainmem-low=0x0 --kcp-mainmem-high=0x4000000"
        elif self.arch == "alpha":
            phy_kernel = ""
            extra_start = " --elf-modify-physaddr=0xfffffc0000000000"
        elif self.arch == "powerpc":
            phy_kernel = ""
            extra_start = ""
        elif self.arch == "powerpc64":
            phy_kernel = ""
            extra_start = ""
        dite_dict["phy_kernel"] = phy_kernel
        start_cmd = " --binfo-sexec -B -k -K 0x%(kernel_kip)s -M 0x%(kernel_kip)s %(phy_kernel)s %(kernel)s " % dite_dict
        end_cmd = " %s -o %s" % (patch_kcp, target[0])

        # Finish for now.. later we need to do more
        for (app, flag) in self.rest:
            extra = ""
            

            if flag == "i":
                extra = "-i"
            if flag == 's':
                extra = '-s'
            if flag == "raw":
                name = str(app).split("/")[-1]
                start_cmd += (" -R --name %s %s") % (name, app)
            elif flag == "l":
                start_cmd += (" --name vmlinux %s.reloc") % (app)
            else:
                name = str(app).split("/")[-1].split(".")[0]
                start_cmd += (" --name %s %s %s.reloc") % (name, extra, app)

        # Make things less chatty!
        cmd = self.cmd + extra_start + start_cmd + end_cmd
        r = os.system(cmd)
        if (r != 0):
            raise "Error running dite", cmd

