/****************************************************************************
 *
 * Copyright (C) 2005,  National ICT Australia (NICTA)
 *
 * File path:	platform/pek/console.cc
 * Description:	IBM PEK IO Console
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
 * $Id: console.cc,v 1.1 2004/12/02 22:04:08 cvansch Exp $
 *
 ***************************************************************************/

#include <l4.h>
#include <debug.h>

#include INC_GLUE(config.h)
#include INC_GLUE(hwspace.h)
#include INC_PLAT(console.h)
#include <kdb/console.h>

#define SERIAL_NAME	"pek"

/****************************************************************************
 *
 *    IO Serial console support
 *
 ****************************************************************************/

#define IO_CMD_LEN		8
#define IO_FILE_NAME_LEN        32
#define IO_BUFFER_LEN		240

/*
 * IBM PEK IO device layout:
 *
 *  0x00    -	cmd register
 *  0x04    -	rc register
 *  0x08    -	error register
 *  0x10    -	buffer start (240 bytes)
 */

typedef struct {
    volatile u32_t cmd;
    volatile u32_t rc;
    volatile u32_t errno;
    u32_t reserved;
    volatile u32_t buffer[IO_BUFFER_LEN];
} io_device_t;

io_device_t *io_phys = (io_device_t*)PEK_IO_POFFSET;

typedef struct
{
    char cmd[IO_CMD_LEN];
    u32_t int1;
    u32_t int2;
    u32_t int3;
    char filename1[IO_FILE_NAME_LEN];
    char filename2[IO_FILE_NAME_LEN];
} pek_command;

void io_execute(io_device_t * dev, pek_command * cmd)
{
    word_t i;

    for (i = 0; i < sizeof(pek_command) / sizeof(u32_t); i++)
    {
	dev->cmd = ((u32_t*)cmd)[i];
    }

    /* block by reading 'errno' from the io device */
    volatile word_t errno = dev->errno;
    errno;
}


void puts_phys( char *c )
{
    pek_command cmd_stack;
    pek_command * cmd = &cmd_stack;
    io_device_t * io = *(io_device_t**)virt_to_phys((word_t)&io_phys);

    cmd->cmd[0] = 'w';
    cmd->cmd[1] = 'r';
    cmd->cmd[2] = 'i';
    cmd->cmd[3] = 't';
    cmd->cmd[4] = 'e';
    cmd->cmd[5] = 0;

    cmd->int1 = 1;

    while (*c)
    {
	char *d = (char*)io->buffer;
	int i = 0;

	while ((*c) && (i < IO_BUFFER_LEN))
	{
	    *d++ = *c++;
	    i++;
	}
	*d = 0;
	cmd->int2 = i+1;

	io_execute(io, cmd);
    }
}

static void putc_io( char c )
{
}

static char getc_io( bool block )
{
    if (block)
	while(1);
    return 0;
}


#if defined(CONFIG_KDB_BREAKIN)
void kdebug_check_breakin (void)

{
//	if (== 27)
//	    enter_kdebug("breakin");
}
#endif

static void init(void) 
{
    UNIMPLEMENTED();
}

/****************************************************************************
 *
 *    Console registration
 *
 ****************************************************************************/

kdb_console_t kdb_consoles[] = {
    { SERIAL_NAME, init, putc_io, getc_io},
    KDB_NULL_CONSOLE
};

word_t kdb_current_console = 0;

