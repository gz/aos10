/****************************************************************************
 *
 *      $Id: frames.c,v 1.3 2003/08/06 22:52:04 benjl Exp $
 *
 *      Description: Example frame table implementation
 *
 *      Author:      Ben Leslie
 *
 ****************************************************************************/

#include <stddef.h>
#include "libsos.h"

#include "frames.h"

static L4_Word_t current;

/*
  Initialise the frame table. The current implementation is
  clearly not sufficient.
*/
void
frame_init(L4_Word_t low, L4_Word_t frame)
{
    current = low;
}

/*
  Allocate a currently unused frame 
*/
L4_Word_t
frame_alloc(void)
{
    return current += PAGESIZE;
}

/*
  Add a frame to the free frame list
*/
void
frame_free(L4_Word_t frame)
{
}

