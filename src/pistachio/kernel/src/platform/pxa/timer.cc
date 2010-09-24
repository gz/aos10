/*********************************************************************
 *
 * Copyright (C) 2004-2006,  National ICT Australia (NICTA)
 *
 * File path:     platform/pxa/timer.cc
 * Description:   PXA Periodic timer handling
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
 ********************************************************************/

#include <l4.h>
#include <debug.h>
#include INC_API(schedule.h)
#include INC_GLUE(timer.h)
#include INC_GLUE(intctrl.h)
#include INC_PLAT(timer.h)
#include INC_API(processor.h)

timer_t timer;
volatile word_t xscale_match;

extern "C" void handle_timer_interrupt(word_t irq, arm_irq_context_t *context)
{
    word_t value = XSCALE_OS_TIMER_TCR;
    xscale_match += TIMER_RATE / (1000000/TIMER_TICK_LENGTH);

    if (((s32_t)(xscale_match - value)) < 100)
    {
	xscale_match = value + TIMER_RATE / (1000000/TIMER_TICK_LENGTH);
    }

    XSCALE_OS_TIMER_MR0 = xscale_match;
    XSCALE_OS_TIMER_TSR = 0x01;  /* Clear interrupt */

    get_current_scheduler()->handle_timer_interrupt();
}

void timer_t::init_global(void)
{
}

void timer_t::init_cpu(void)
{
#ifdef CONFIG_PXA255
    /* Turn on TURBO mode */
    __asm__  __volatile__ (
	    "   mov	    r0,	    #3		    \n"
	    "   mcr	    p14, 0, r0, c6, c0, 0   \n"
	    ::: "r0"
	    );
#endif

    get_interrupt_ctrl()->register_interrupt_handler(XSCALE_IRQ_OS_TIMER,
	    handle_timer_interrupt);

    /* Setup initial timer interrupt */
    XSCALE_OS_TIMER_TSR = 0x0f;
    XSCALE_OS_TIMER_MR0 = xscale_match = TIMER_RATE / (1000000/TIMER_TICK_LENGTH);
    XSCALE_OS_TIMER_TCR = 0x00000000;
    XSCALE_OS_TIMER_IER = 0x01;	    /* Enable timer channel 0 */

    get_interrupt_ctrl()->unmask(XSCALE_IRQ_OS_TIMER);

    {
	word_t cccr = XSCALE_CLOCKS_CCCR;
	word_t bus, cpu;
#if defined(CONFIG_PXA270)
	word_t l, n;
	word_t clkcfg;
#elif defined(CONFIG_PXA255)
	word_t x, y, z;

	cccr &= ~ ((0x7) << 7);
	cccr |= (0x4 << 7);
	XSCALE_CLOCKS_CCCR = cccr;
#endif

	/* Print out the clock settings.
	 * This is processor specific */

#ifdef CONFIG_PXA270
	/* Fetch CP14 CLK_CFG */
	__asm__ __volatile__ (
		"	mrc		p14, 0, %0, c6, c0, 0	\n"
		: "=r"(clkcfg) ::
		);

	/* Can never be less than 2; else equal to L */
	if (cccr & 0x1c)
	    l = (cccr & 0x1f);
	else
	    l = 2;
	n = ((cccr >> 7) & 0xf) / 2; /* 2n / 2 */

	bus = l * 13000000;
	cpu = bus;

	/* Bus config */
	if (clkcfg & CLKCFG_B)
	    printf("Fast Bus Mode Enabled\n");
	else
	    bus = bus / 2; /* bus = run-speed /2 */

	/* CPU config */
	if (clkcfg & CLKCFG_HT){
	    printf("Half Turbo Mode\n");
	    cpu = (cpu * n) / 2;
	}
	else if (clkcfg * CLKCFG_T) {
	    printf("Turbo Mode\n");
	    cpu = cpu * n;
	}

	printf("Mem Speed = %dkHz\n", bus);
	printf("CPU Speed = %dkHz\n", cpu);

#elif defined CONFIG_PXA255
	switch (cccr & 0x1f)
	{
	case 0x1:   x = 27; break;
	case 0x3:   x = 36; break;
	case 0x5:   x = 45; break;
	default:    x = 0;  break;
	}
	bus = x * 3686400 / 1000;
	printf("Mem Speed = %dkHz\n", x * 3686400 / 1000);

	switch ((cccr >> 5) & 0x3)
	{
	case 0x1:   y = 1; break;
	case 0x2:   y = 2; break;
	case 0x3:   y = 4; break;
	default:    y = 0;  break;
	}
	printf("Run Speed = %dkHz\n", x * 3686400 * y / 1000);

	switch ((cccr >> 7) & 0x7)
	{
	case 0x2:   z = 10; break;
	case 0x3:   z = 15; break;
	case 0x4:   z = 20; break;
	case 0x6:   z = 30; break;
	default:    z = 0;  break;
	}
	cpu = x * 36864 * y * z / 100;
	printf("Turbo Speed = %dkHz\n", x * 36864 * y * z / 100);
#else
#error Invalid sub-architecture defined.
#endif

	/* initialize V4 processor info */
	init_processor (0, bus, cpu);
    }
}
