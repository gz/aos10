from simulators import *

class Machine:
    virtual = True
    as_flags = []
    c_flags = []
    cpp_flags = []
    cxx_flags = []
    link_flags = []
    drivers = []
    wordsize = None
    timer_driver = None
    serial_driver = "drv_l4_kdb"
    elfloader = False
    endian = None
    # arm v4/v5 PID relocation
    pidreloc = None
    wombat_config = None

############################################################################
# ia32 based machines
############################################################################
class ia32(Machine):
    wordsize = 32
    arch = "ia32"
    endian = "little"
    as_flags = ["-m32"]
    c_flags = ["-m32"]
    link_flags = ["-melf_i386", "-N"]
    toolchain = ""
    qemu_args = []
    sim = staticmethod(qemu_sim)
    wombat_config = "l4linux_config_ia32"

class pc99_vga(ia32):
    virtual = False
    cpu = "i686"
    l4conf = "ia32-qemu-graphic"
    platform = "pc99"
    qemu_args = [] # ia32.qemu_args + ["-nographic"]
    serial_console = False
    timer_driver = "drv_8254_timer"

class pc99(ia32):
    virtual = False
    cpu = "i686"
    l4conf = "ia32-qemu-serial"
    platform = "pc99"
    qemu_args = ia32.qemu_args + ["-nographic"]
    serial_console = True
    timer_driver = "drv_8254_timer"

############################################################################
# arm based machines
############################################################################
class arm(Machine):
    wordsize = 32
    arch = "arm"
    toolchain = "arm-linux-"
    endian = "little"
    sim = staticmethod(skyeye_sim)
    pidreloc = True
    wombat_config = "l4linux_config_arm"

class sa1100(arm):
    c_flags = arm.c_flags + ["-march=armv4", "-mtune=strongarm1100"]
    l4conf = "sa1100"
    cpu = "sa1100"
    timer_driver = "drv_sa1100_timer"
    wombat_config = "l4linux_config_sa1100"

class pleb(sa1100):
    virtual = False
    serial_driver = "drv_sa1100_uart"
    skyeye = "pleb.skyeye"
    platform = "pleb"

class ipaq_h3800(sa1100):
    virtual = False
    skyeye = "pleb.skyeye"
    elfloader = True
    elf_entry = 0xC0008000L
    platform = "pleb"

class xscale(arm):
    c_flags = arm.c_flags + ["-march=armv5te", "-mtune=xscale"]
    serial_driver = "drv_8250_uart"
    cpu = "xscale"

class pxa(xscale):
    platform = "pxa"
    wombat_config = "l4linux_config_pxa"
    drivers = ["pxa"]

class ixp4xx(xscale):
    platform = "ixp4xx"
    wombat_config = "l4linux_config_ixp4xx"

class pleb2(pxa):
    virtual = False
    c_flags = xscale.c_flags
    timer_driver = "drv_pxa250_timer"
    skyeye = "pleb2.skyeye"
    elfloader = True
    elf_entry = 0xa2008000L
    subplatform = "pxa255"
    ramstart = 0xa0000000L
    ramend = 0xa4000000L
    uart = "FFUART"
    wombat_config = "l4linux_config_pleb2"

class pleb2_thumb(pleb2):
    cpp_flags = ["-mthumb", "-mthumb-interwork"]
    toolchain = "arm-elf-"

class gumstix(pxa):
    virtual = False
    c_flags = xscale.c_flags
    timer_driver = "drv_pxa250_timer"
    skyeye = "gumstix.skyeye"
    elfloader = True
    elf_entry = 0xa2008000L
    subplatform = "pxa255"
    ramstart = 0xa0000000L
    ramend = 0xa4000000L
    uart = "FFUART"
    wombat_config="l4linux_config_gumstix"

class ibox(pxa):
    virtual = False
    c_flags = xscale.c_flags
    timer_driver = "drv_pxa250_timer"
    skyeye = "ibox.skyeye"
    elfloader = True
    elf_entry = 0xa2008000L
    subplatform = "pxa270"
    ramstart = 0xa0000000L
    ramend = 0xa4000000L
    uart = "FFUART"
    wombat_config="l4linux_config_ibox"

class hx4700(pxa):
    virtual = False
    c_flags = xscale.c_flags
    timer_driver = "drv_pxa250_timer"
    skyeye = "hx4700.skyeye"
    elfloader = True
    elf_entry = 0xa2008000L
    subplatform = "pxa270"
    ramstart = 0xa0000000L
    ramend = 0xa4000000L
    uart = "FFUART"

class ln2410sbc(arm):
    virtual = False
    l4conf = "ln2410sbc"
    skyeye = "ln2410sbc.skyeye"
    platform = "ln2410sbc"
    cpu = "arm920t"
    timer_driver = "drv_s3c2410_timer"
#    drivers = ["drv_ln2410sbc_lcd"]
    ramstart = 0x30000000
    ramend = 0x38000000

class smdk2410(arm):
    virtual = False
    l4conf = "ln2410sbc"
#    skyeye = "ln2410sbc.skyeye"
    platform = "ln2410sbc"
    cpu = "arm920t"
    timer_driver = "drv_s3c2410_timer"
    ramstart = 0x30000000
    ramend = 0x34000000

class nslu2(ixp4xx):
    c_flags = ixp4xx.c_flags + ["-DBIG_ENDIAN"]
    elf_entry = 0x01d00000L
    elfloader = True
    endian = "big"
    ramstart = 0x00000000
    ramend = 0x02000000
    skyeye = "nslu2.skyeye"
    subplatform = "ixp420"
    toolchain = "armv5b-softfloat-linux-"
    uart = "HSUART"
    virtual = False

class at91rm9200(arm):
    c_flags = arm.c_flags + ["-march=armv4", "-mtune=arm920t"]
    cpu = "arm920t"
    platform = "at91rm9200"
    timer_driver = "drv_at91rm9200_timer"

class at9200dev(at91rm9200):
    virtual = False
    elfloader = True
    elf_entry = 0x21800000L
    ramstart = 0x20000000
    ramend = 0x24000000
    uart = "USART1"

class csb337(at91rm9200):
    virtual = False
    elfloader = True
    elf_entry = 0x21800000L
    ramstart = 0x20000000
    ramend = 0x22000000
    uart = "DBGU"


############################################################################
# mips64 based machines
############################################################################
class mips64(Machine):
    wordsize = 64
    arch = "mips64"
    c_flags = ["-mlong64", "-mabi=64", "-G",  "0", "-Wa,-mabi=o64",
               "-mno-abicalls", "-non_shared", "-msplit-addresses",
               "-mips3"]
    toolchain = "mips64-elf-"
    sim = staticmethod(sulima_sim)

class u4600(mips64):
    virtual = False
    platform = "u4600"
    cpu = "r4x00"
    l4conf = "u4600"
    endian = "big"
    timer_driver = "drv_gt64xxx_timer"

############################################################################
# Alpha based machines
############################################################################
class alpha(Machine):
    wordsize = 64
    arch = "alpha"
    endian = "little"
    c_flags = ["-mno-fp-regs"]
    toolchain = "alpha-unknown-linux-gnu-"
    elfloader = True
    elf_entry = 0x20000000 # Alpha console expects apps here.
    sim = staticmethod(m5_sim)
    
class ds20(alpha):
    virtual = False
    l4conf = "tsunami"
    # awiggins (2005-10-05): elf_entry override is for M5, needs cleaning up.
    elf_entry = 0xfffffc0000200000

############################################################################
# powerpc based machines
############################################################################
class powerpc(Machine):
    wordsize = 32
    arch = "powerpc"
    c_flags = []
    toolchain = "powerpc-linux-"
#    sim = staticmethod(_sim)

class ppc405(powerpc):
    cpu = "ppc405"

class pek405(ppc405):
    virtual = False
    c_flags = []
    l4conf = "pek405"
    platform = "pek405"
    cpu = "405"
    endian = "big"

