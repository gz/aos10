/*********************************************************************
 *
 * Copyright (C) 2004-2006,  National ICT Australia (NICTA)
 *
 * File path:     platform/pxa/irq.cc
 * Description:   PXA platform demultiplexing
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
#include <linear_ptab.h>
#include INC_API(tcb.h)
#include INC_GLUE(intctrl.h)
#include INC_CPU(cpu.h)
#include INC_ARCH(special.h)

#define BIT(x) (1<<(x))

extern "C" void
handle_gpio_irq (word_t irq, arm_irq_context_t * context)
{
    word_t status;
    int i;
    word_t pin_irq;

    if (irq != GPIO_IRQ)
	printf("Got spurious call to handle_gpio_irq!\n");

    /* We don't look at the first two bits because
       the first two GPIOs have top level interrupts */
    status = PXA_GEDR0;
    status >>= 2;
    if (status != 0) {
	for (i=2; i<32; i++) {
	    if ((status & 0x1) != 0) {
		/* The IRQ associated with GPIO 2 is
		   PRIMARY_IRQS because the first
		   two interrupts have top level IRQs */

		pin_irq = i + PRIMARY_IRQS - 2;
		void (*irq_handler)(int, arm_irq_context_t *) =
		    (void (*)(int, arm_irq_context_t *))
		    interrupt_handlers[pin_irq];

		PXA_GEDR0 = BIT(i & 0x1F);
		irq_handler(pin_irq, context);
		return;
	    }
	    status >>= 1;
	}
    }

    status = PXA_GEDR1;
    if (status != 0)
    {
	for (i=32; i<64; i++) {
	    if ((status & 0x1) != 0) {
		/* The IRQ associated with GPIO 2 is
		   PRIMARY_IRQS because the first
		   two interrupts have top level IRQs */

		pin_irq = i + PRIMARY_IRQS - 2;
		void (*irq_handler)(int, arm_irq_context_t *) =
		    (void (*)(int, arm_irq_context_t *))
		    interrupt_handlers[pin_irq];
		PXA_GEDR1 = BIT(i & 0x1F);
		irq_handler(pin_irq, context);
		return;
	    }
	    status >>= 1;
	}
    }

#if defined(CONFIG_PXA270)
    status = PXA_GEDR2;
    if (status != 0)
    {
	for (i=64; i<96; i++) {
	    if ((status & 0x1) != 0) {
		/* The IRQ associated with GPIO 2 is
		   PRIMARY_IRQS because the first
		   two interrupts have top level IRQs */

		pin_irq = i + PRIMARY_IRQS - 2;
		void (*irq_handler)(int, arm_irq_context_t *) =
		    (void (*)(int, arm_irq_context_t *))
		    interrupt_handlers[pin_irq];
		PXA_GEDR2 = BIT(i & 0x1F);
		irq_handler(pin_irq, context);
		return;
	    }
	    status >>= 1;
	}
    }

    status = PXA_GEDR3;
    if (status != 0)
    {
	for (i=96; i<(GPIO_IRQS+2); i++) {
	    if ((status & 0x1) != 0) {
		/* The IRQ associated with GPIO 2 is
		   PRIMARY_IRQS because the first
		   two interrupts have top level IRQs */

		pin_irq = i + PRIMARY_IRQS - 2;
		void (*irq_handler)(int, arm_irq_context_t *) =
		    (void (*)(int, arm_irq_context_t *))
		    interrupt_handlers[pin_irq];
		PXA_GEDR3 = BIT(i & 0x1F);
		irq_handler(pin_irq, context);
		return;
	    }
	    status >>= 1;
	}
    }

#elif defined(CONFIG_PXA255)
    status = PXA_GEDR2;
    if (status != 0) {
	for (i=64; i<(GPIO_IRQS+2); i++) {
	    if ((status & 0x1) != 0) {
		/* The IRQ associated with GPIO 2 is
		   PRIMARY_IRQS because the first
		   two interrupts have top level IRQs */

		pin_irq = i + PRIMARY_IRQS - 2;
		void (*irq_handler)(int, arm_irq_context_t *) =
		    (void (*)(int, arm_irq_context_t *))
		    interrupt_handlers[pin_irq];
		PXA_GEDR2 = BIT(i & 0x1F);
		irq_handler(pin_irq, context);
		return;
	    }
	    status >>= 1;
	}
    }
#endif

    return;
}

extern "C" void arm_irq(arm_irq_context_t *context)
{
    int i;

    /* Read the state of the ICIP registers */
    word_t status = XSCALE_INT_ICIP;
#ifdef CONFIG_PXA270
    word_t status2 = XSCALE_INT_ICIP2;
#endif

    /* Handle timer first */
    if (status & (1ul << XSCALE_IRQ_OS_TIMER))
    {
	void (*irq_handler)(int, arm_irq_context_t *) =
		(void (*)(int, arm_irq_context_t *))
		interrupt_handlers[XSCALE_IRQ_OS_TIMER];
	irq_handler(XSCALE_IRQ_OS_TIMER, context);
	return;
    }

    /* Special case the GPIO interrupt for now
       FIXME: Should be installed as a regular IRQ handler
       which would make this code neater */
    if ( status & (1ul << GPIO_IRQ)) {
	handle_gpio_irq(GPIO_IRQ, context);
	return;
    }

#ifdef CONFIG_PXA270
    /* FIXME XXX: Use ICHP register to directly work out which
       interrupt is pending */

    for (i = 0; i < 32; i++)
    {
	/* XXX Use the clz instruction */
	if (status & (1ul << i)) {
	    void (*irq_handler)(int, arm_irq_context_t *) =
		(void (*)(int, arm_irq_context_t *))interrupt_handlers[i];

	    irq_handler(i, context);
	    return;
	}
    }
    for (; i < PRIMARY_IRQS; i++)
    {
	if (status2 & (1ul << (i-32))) {
	    void (*irq_handler)(int, arm_irq_context_t *) =
		(void (*)(int, arm_irq_context_t *))interrupt_handlers[i];

	    irq_handler(i, context);
	    return;
	}
    }

#elif defined(CONFIG_PXA255)
    status = status & (~0x7ful);	/* 0..6 are reserved */
    if (status)
    {
	i = msb(status);

	void (*irq_handler)(int, arm_irq_context_t *) =
	    (void (*)(int, arm_irq_context_t *))interrupt_handlers[i];

	irq_handler(i, context);

	return;
    }

#else
#error subplatform was not implemented.
#endif

    ASSERT(DEBUG, !"spurious irq");
}


void SECTION(".init") intctrl_t::init_arch()
{
    /* FIXME: This gets overrided by the init_cpu function because
       it is executed after init_arch. Therefore we special case
       above */

    /* register GPIO irq handler.*/
    get_interrupt_ctrl ()->register_interrupt_handler (GPIO_IRQ,
	    handle_gpio_irq);
}

