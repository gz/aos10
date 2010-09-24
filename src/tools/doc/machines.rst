=======================================================
 Information on machines supported by the build system
=======================================================

:Author: Ben Leslie <ben.leslie@nicta.com.au>
:Copyright: National ICT Australia, 2005

.. contents::

ARM
====

We support a range of different embedded ARM boards, using a
range of different processors.

Pleb
-----

.. image:: http://www.ertos.nicta.com.au/hardware/pleb/images/pleb.jpg


Pleb2
-----

.. image:: http://www.ertos.nicta.com.au/hardware/pleb/images/pleb2.jpg

PLEB 2 is a single board computer designed to provide a platform on
which both academic research and application implementation can be
accomplished. It has been based on PLEB. More information is
available on the `PLEB2 website <http://www.ertos.nicta.com.au/hardware/pleb/>`_.

The Pleb 2 uses U-boot bootloader, and is simulated using the skyeye
simulator. It is based on the XScale processor.

When you build the Pleb2 image you get a bootimg.bin file. You then
need to copy this to address 0xa0008000 on the board. You can 
do this either using uploadover serial, or using the network card.
For example using the network card::

 $ tftpboot 0xa0008000 benno.bin
 $ go 0xa0008000


LN2410SBC
---------

SMDK2410
--------

This samsung board uses the S3C2410X arm920t processor.  

It comes with a bootloader that loads a binary image to 0x30040000 over USB 
then executes it.  This is not exactly compatible with our development 
setup, so we need to flash it with u-boot.

I used a standard download of u-boot 1.1.4.  This version already supports the 
smdk2410 board, so building u-boot is simply a matter of: 

make smdk2410_config
make

then flashing the board with the u-boot.bin file that is generated.

You may need to take out the apcs-gnu compiler option from the makefiles to 
avoid compile errors from gcc.

The board we have is already flashed with u-boot. To flash the board with this
you may be able to follow samsungs instructions, but they did not work for me.

I ended up Connecting the board to the lauderbach debugger and loading the u-boot 
image into memory and executing it directly.  I then loaded another u-boot image 
into memory and used the running u-boot to flash the second image into the rom.

One trick to this is that the memory control registers must be set correctly for 
trace32 to be able to actually write correctly to the smdk2410 memory.  On 
reset these registers are set to access RAM with an 8-bit bus instead of a 32 bit 
bus, so RAM cannot be written corretly by trace32.  You need to set the memory controller 
registers in trace32 which will enable the RAM to be written.  To do this, make sure 
that the CPU type is set correctly (in CPU->Options or whatever) - to S3C2410X.  Then 
open up the peripherals window and expand the memory controller section.  The values
for the registers are listed on the left hand side.  You need to set these to:

0x2211d122
0x00000700
0x00000700
0x00000700
0x00001f4c
0x00000700
0x00000700
0x00018005
0x00018005
0x008e0459
0x00000032
0x00000030
0x00000030

(these values were read out of the compiled u-boot image.)

Note that u-boot does not work fully when loaded directly from memory in this way,
     in particular it is not able to find the network card.

Once the system has been flashed with u-boot it can boot an elf file similarly to 
all of the other boards.  specifically I use a bootcmd something like

tftp 0x32000000 mwarton/kernel; bootelf 0x32000000

NOTE: I used the trick of including the line 
#define CONFIG_SKIP_LOWLEVEL_INIT 
in the file
include/configs/smdk2410.h
at various times.  This just makes u-boot skip setting up the memory controllers when it runs.
you may need to skip this when running the u-boot in memory, but make sure that the u-boot image
that is written to flash does not have this line included - ie does set up the memory controllers.
Otherwise u-boot will crash on boot.



IPAQ H3100, H3600, H3700, H3800 (SA1100-based IPAQs)
----------------------------------------------------

We currently support IPAQs with the StrongARM (sa1100) processor. To boot this machine you will need
the ARM bootloader version 2.21.12 installed on your IPAQ. See 
http://familiar.handhelds.org/releases/v0.8.2/install/bootldr.html for more information
on installing the bootloader::

 boot> load ram 0xc0008000
 (start ymodem)
 boot> jump 0xc0008000

Alternatively if you can boot from a cfcard::

 boot> vfat read 0xc0008000 boot/bootimage
 boot> jump 0xc0008000


MIPS64
=======

Currently just support U4600 out of the box.

u4600
-----

Home grown 64-bit mips machine.


IA32
=====

PC99
----

Support standard PC99 machiens using a variety of booting methods.
