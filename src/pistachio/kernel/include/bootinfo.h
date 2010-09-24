/*********************************************************************
 *                
 * Copyright (C) 2002,  Karlsruhe University
 *
 * Copyright (c) 2004 Qualcomm Incorporated
 *                
 * File path:     bootinfo.h
 * Description:   boot info definitions
 *                
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *                
 * $Id: //depot/asic/msmshared/platform/kernel/l4/pistachio/branch/2.1/platform/pistachio/kernel/include/bootinfo.h#1 $
 *                
 ********************************************************************/
#ifndef __BOOTINFO_H__
#define __BOOTINFO_H__

#include <types.h>

#define L4_BOOTINFO_MAGIC               ((word_t) 0x14b0021d)
#define L4_BOOTINFO_VERSION             1

#define L4_BOOTINFO_SIMPLEEXEC          0x0002
#define L4_BOOTINFO_SIMPLEEXEC_VERSION  1

/**
 * Generic bootinfo record.
 */
class bootrec_t
{
    word_t      _type;
    word_t      _version;
    word_t      _offset_next;

public:

    enum type_e {
        module          = 0x0001,
        simple_exec     = 0x0002,
        efitables       = 0x0101,
        multiboot       = 0x0102,
    };

    type_e type (void)
        { return (type_e) _type; }

    word_t version (void)
        { return _version; }

    bootrec_t * next (void)
        { return (bootrec_t *) ((word_t) this + _offset_next); }
};


/**
 * Bootinfo record for simple binary file.
 */
class boot_module_t
{
public:
    word_t      type;                   // 0x01
    word_t      version;                // 1
    word_t      offset_next;

    word_t      start;
    word_t      size;
    word_t      cmdline_offset;

    const char * commandline (void)
        { return cmdline_offset ? (const char *) this + cmdline_offset : ""; }
};


/**
 * Bootinfo record for simple executable image loaded and relocated by
 * the bootloader.
 */
class boot_simpleexec_t
{
public:
    word_t      type;                   // 0x02
    word_t      version;                // 1
    word_t      offset_next;

    word_t      text_pstart;
    word_t      text_vstart;
    word_t      text_size;
    word_t      data_pstart;
    word_t      data_vstart;
    word_t      data_size;
    word_t      bss_pstart;
    word_t      bss_vstart;
    word_t      bss_size;
    word_t      initial_ip;
    word_t      flags;
    word_t      label;
    word_t      cmdline_offset;

    const char * commandline (void)
        { return cmdline_offset ? (const char *) this + cmdline_offset : ""; }
};


/**
 * Bootinfo record for EFI table information.
 */
class boot_efi_t
{
public:
    word_t      type;                   // 0x101
    word_t      version;                // 1
    word_t      offset_next;

    word_t      systab;
    word_t      memmap;
    word_t      memmap_size;
    word_t      memdesc_size;
    word_t      memdesc_version;
};


/**
 * Bootinfo record for multiboot info.
 */
class boot_mbi_t
{
public:
    word_t      type;                   // 0x102
    word_t      version;                // 1
    word_t      offset_next;

    word_t      address;
};


/**
 * Main structure for generic bootinfo.
 */
class bootinfo_t
{
    word_t      _magic;
    word_t      _version;
    word_t      _size;
    word_t      _first_entry;
    word_t      _num_entries;
    word_t      __reserved[3];

public:

    bool is_valid (void)
        { return ((_magic   == L4_BOOTINFO_MAGIC) &&
                  (_version == L4_BOOTINFO_VERSION)); }

    word_t magic (void)         { return _magic; }
    word_t version (void)       { return _version; }
    word_t size (void)          { return _size; }
    word_t entries (void)       { return _num_entries; }
    bootrec_t * first_entry (void)
        { return (bootrec_t *) ((word_t) this + _first_entry); }
};

#endif /* !__BOOTINFO_H__ */
