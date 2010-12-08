MCHN_FLAGS  = machine=nslu2
SCONS       = scons-0.96.93 $(MCHN_FLAGS)
TLA         = baz
TFTPROOT    = /tftpboot
TARGET      = $(TFTPROOT)/bootimg.bin
SCONSRESULT = build/bootimg.bin
ELF_BASE    = build/userland/

ifeq ($(OS), Darwin)
    SERIAL_PORT = $(firstword $(wildcard /dev/cu.usbserial-*))
else
    SERIAL_PORT = $(firstword $(wildcard /dev/ttyUSB*))
endif

ifeq ($(SERIAL_PORT),)
    $(warning Warning: USB serial port not found. nslu2 commands will not be issued)
    SLUG_CMD = @true
else
    SLUG_CMD = nslu2 -p $(SERIAL_PORT)
endif


NODISTLIST = packages

all: $(TARGET) reset

.IGNORE: on off up down reset
on off up down reset:
	$(SLUG_CMD) $(patsubst on,up,$(patsubst off,down,$@))

$(TARGET): $(SCONSRESULT)
	mkdir -p $(TFTPROOT)
	cp $(SCONSRESULT) $(TARGET) || true
	cp `find $(ELF_BASE) -depth -executable -type f | grep -v ".reloc"` $(TFTPROOT) || true

$(SCONSRESULT): on tools
	$(SCONS)

.PHONY: $(SCONSRESULT)

clean:
	$(SCONS) -c

cleanconfig: 
	@$(TLA) cat-config packages | cut -f 1 | xargs -t rm -rf

distclean:
	@excl="-name 'cscope.*' -o -name '.sconsign.dblite'";		\
	excl="$$excl -o -type d -name build";				\
	eval "find . \( $$excl \) -print | xargs -t rm -rf"

tools:
	$(TLA) build-config packages

config:	tools

dist:	tools distclean
	@dstroot=$${DSTROOT:-/tmp};					\
	project=`pwd`;project=`basename $$project`;			\
	dstdir=$$dstroot/$$project/aos-2006;				\
	excl="-name '{arch}' -prune -o -name .arch-ids -prune";		\
	excl="$$excl -o -name '*.swp'";					\
	incl="\( -type f -o -type l \)";				\
	     								\
	rm -rf $$dstdir; mkdir -p $$dstdir;				\
	cmd="find * $$excl -o $$incl -print | pax -rw $$dstdir";	\
       	echo $$cmd; eval $$cmd;						\
	(								\
	    cd $$dstdir; echo cd $$dstdir;				\
	    cmd="rm $(NODISTLIST)"; echo $$cmd; eval $$cmd;		\
	    find */* -type f -print | xargs fgrep -l AOS_STRIP		\
	    | while read stripfile;					\
	    do								\
		echo Stripping $$stripfile;				\
		cmd="unifdef -DAOS_STRIP $$stripfile > $$stripfile~";	\
		eval "$$cmd; mv $$stripfile~ $$stripfile";		\
	    done || true;						\
	    cmd="cd .. && tar -cjf aos-2006.tbz2 aos-2006";		\
	    echo "$$cmd"; eval "$$cmd";					\
	)
