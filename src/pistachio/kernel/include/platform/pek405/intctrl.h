/****************************************************************************
 *
 * Copyright (C) 2005, National ICT Australia
 *
 * File path:	platform/pek405/intctrl.h
 * Description:	The UIC interrupt driver.
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
 * $Id: opic.h,v 1.14 2003/09/24 19:04:58 skoglund Exp $
 *
 ***************************************************************************/

#ifndef __PLATFORM__PEK405__INTCTRL_H__
#define __PLATFORM__PEK405__INTCTRL_H__

class intctrl_t
{
public:
    void init_arch();
    void init_cpu(word_t cpu);

    word_t get_number_irqs(void)
    {
	return 0; // XXX IRQS;
    }

    static inline void mask(word_t irq)
    {
	UNIMPLEMENTED();
    }

    static inline bool unmask(word_t irq)
    {
	UNIMPLEMENTED();
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

    bool is_irq_available(int irq)
    { 
	return false;  
    }

    void set_cpu(word_t irq, word_t cpu) {}
};


#endif	/* __PLATFORM__PEK405__INTCTRL_H__ */

