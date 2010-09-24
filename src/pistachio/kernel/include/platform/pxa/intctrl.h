/*********************************************************************
 *                
 * Copyright (C) 2004-2006,  National ICT Australia (NICTA)
 *                
 * File path:     platform/pxa/intctrl.h
 * Description:   
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
#ifndef __PLATFORM__PXA__INTCTRL_H__
#define __PLATFORM__PXA__INTCTRL_H__

#include <intctrl.h>
#include INC_GLUE(hwspace.h)
#include INC_ARCH(thread.h)
#include INC_API(space.h)
#include INC_PLAT(timer.h)
#include INC_CPU(cpu.h)

#if defined(CONFIG_PXA255)

#define PRIMARY_IRQS            32
#define GPIO_IRQS               (85 - 2) /* The first two IRQs have level
					    one interrupts */ 
#define GPIO_IRQ                10

#elif defined(CONFIG_PXA270)

#define PRIMARY_IRQS	        34 /* 35-39 are reserved */
#define GPIO_IRQS               (121 - 2) /* The first two IRQs have level
					     one interrupts */ 
#define GPIO_IRQ                10

#else
#error Invalid PXA sub-architecture
#endif

#define IRQS			(PRIMARY_IRQS + GPIO_IRQS)

/* Interrupt Controller */
#define INTERRUPT_POFFSET	0xd00000
#define INTERRUPT_VOFFSET	0x003000
#define XSCALE_IRQ_OS_TIMER	26

#define XSCALE_INT		(IODEVICE_VADDR + INTERRUPT_VOFFSET)

#define XSCALE_INT_ICMR		(*(volatile word_t *)(XSCALE_INT + 0x04))   /* Mask register */
#define XSCALE_INT_ICLR		(*(volatile word_t *)(XSCALE_INT + 0x08))   /* FIQ / IRQ selection */
#define XSCALE_INT_ICCR		(*(volatile word_t *)(XSCALE_INT + 0x14))   /* Control register */
#define XSCALE_INT_ICIP		(*(volatile word_t *)(XSCALE_INT + 0x00))   /* IRQ pending */
#define XSCALE_INT_ICFP		(*(volatile word_t *)(XSCALE_INT + 0x0c))   /* FIQ pending */
#define XSCALE_INT_ICPR		(*(volatile word_t *)(XSCALE_INT + 0x10))   /* Pending (unmasked) */

#if defined(CONFIG_PXA270)
/* PXA270 Extras */
#define XSCALE_INT_ICIP2	(*(volatile word_t *)(XSCALE_INT + 0x9c))   /* IRQ pending 2 */
#define XSCALE_INT_ICMR2	(*(volatile word_t *)(XSCALE_INT + 0xa0))   /* Mask register 2 */
#define XSCALE_INT_ICLR2	(*(volatile word_t *)(XSCALE_INT + 0xa4))   /* FIQ / IRQ select 2 */
#define XSCALE_INT_ICFP2	(*(volatile word_t *)(XSCALE_INT + 0xa8))   /* FIQ pending 2 */
#define XSCALE_INT_ICPR2	(*(volatile word_t *)(XSCALE_INT + 0xac))   /* Pending 2 (unmasked) */
#endif

/* FIXME: These should be defined somewhere central */ 
#define GPIO_POFFSET            0xe00000
#define GPIO_VOFFSET            0x004000
#define PXA_GPIO                (IODEVICE_VADDR + GPIO_VOFFSET)

#define PXA_GEDR0	(*(volatile word_t *)(PXA_GPIO + 0x48))   /* GPIO edge detect 0 */
#define PXA_GEDR1	(*(volatile word_t *)(PXA_GPIO + 0x4C))   /* GPIO edge detect 1 */
#define PXA_GEDR2	(*(volatile word_t *)(PXA_GPIO + 0x50))   /* GPIO edge detect 2 */
#ifdef CONFIG_PXA270
#define PXA_GEDR3	(*(volatile word_t *)(PXA_GPIO + 0x148))   /* GPIO edge detect 3 */
#endif

extern word_t arm_high_vector;
extern word_t interrupt_handlers[IRQS];

class intctrl_t : public generic_intctrl_t {

public:
    void init_arch();
    void init_cpu();

    word_t get_number_irqs(void)
    {
	return IRQS;
    }

    void register_interrupt_handler (word_t vector, void (*handler)(word_t,
		arm_irq_context_t *))
    {
	ASSERT(DEBUG, vector < IRQS);
	interrupt_handlers[vector] = (word_t) handler;
	TRACE_INIT("interrupt vector[%d] = %p\n", vector,
		interrupt_handlers[vector]);
    }

    static inline void mask(word_t irq)
    {
	ASSERT(DEBUG, irq < IRQS);
#if defined(CONFIG_PXA255)
	if(irq < PRIMARY_IRQS)
		XSCALE_INT_ICMR &= ~(1 << irq);
	else /* This is a GPIO IRQ and we should mask GPIO_IRQ */ 
		XSCALE_INT_ICMR &= ~(1 << GPIO_IRQ); 

#elif defined(CONFIG_PXA270)
	if(irq < 32)
		XSCALE_INT_ICMR &= ~(1 << irq); 
	else if( irq < PRIMARY_IRQS)
		XSCALE_INT_ICMR2 &= ~(1 << (irq-32)); 
	else /* This is a GPIO IRQ and we should mask GPIO_IRQ */
		XSCALE_INT_ICMR &= ~(1 << GPIO_IRQ); 

#else
#error implement
#endif
    }

    static inline bool unmask(word_t irq)
    {
	ASSERT(DEBUG, irq < IRQS);
#if defined(CONFIG_PXA255)
	if(irq < PRIMARY_IRQS)
		XSCALE_INT_ICMR |= (1 << irq);
	else /* This is a GPIO IRQ, and we should unmask GPIO_IRQ */
		XSCALE_INT_ICMR |= (1 << GPIO_IRQ); 

#elif defined(CONFIG_PXA270)
	if(irq < 32)
		XSCALE_INT_ICMR |= (1<<irq); 
	else if(irq < PRIMARY_IRQS)
		XSCALE_INT_ICMR2 |= (1 << (irq-32)); 
	else /* This is a GPIO IRQ, and we should unmask GPIO_IRQ */
		XSCALE_INT_ICMR |= (1<< GPIO_IRQ); 

#else
#error implement
#endif
	return false;
    }

    static inline void disable(word_t irq)
    {
	mask(irq);
    }

    static inline bool enable(word_t irq)
    {
	return unmask(irq);
    }

    void disable_fiq(void)
    {
	XSCALE_INT_ICLR = 0x00; /* No FIQs for now */
#if defined(CONFIG_PXA270)
	XSCALE_INT_ICLR2 = 0x00;
#endif
    }

    bool is_irq_available(int irq)
    {
	return (irq < IRQS)
		&& (irq != XSCALE_IRQ_OS_TIMER)
		&& (irq != GPIO_IRQ);
    }

    void set_cpu(word_t irq, word_t cpu) {}
};

#endif /*__PLATFORM__PXA__INTCTRL_H__ */
