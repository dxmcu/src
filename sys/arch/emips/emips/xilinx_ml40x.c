/* $NetBSD: xilinx_ml40x.c,v 1.1 2011/01/26 01:18:51 pooka Exp $ */

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was written by Alessandro Forin and Neil Pittman
 * at Microsoft Research and contributed to The NetBSD Foundation
 * by Microsoft Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xilinx_ml40x.c,v 1.1 2011/01/26 01:18:51 pooka Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/sysconf.h>

#include <emips/emips/machdep.h>
#include <emips/emips/cons.h>
#include <emips/emips/emipstype.h>
#include <machine/emipsreg.h>


void		xilinx_ml40x_init (void);
static void	xilinx_ml40x_cons_init (void);

#if 0
#define NOINTS (MIPS_INT_MASK_5|MIPS_SOFT_INT_MASK_0|MIPS_SOFT_INT_MASK_1)
#else
#define NOINTS MIPS_SPLHIGH
#endif

/* BUGBUG Rewrite this to go off to the interrupt controller masks */
#if 0
	splvec.splbio = MIPS_SPL0; // 0x700
	splvec.splnet = MIPS_SPL_0_1; // 0xf00
	splvec.spltty = MIPS_SPL_0_1_2; // 0x1f00
	splvec.splvm = MIPS_SPLHIGH; // 0xff00
	splvec.splclock = MIPS_SPL_0_1_2_3; //0x3f00
	splvec.splstatclock = MIPS_SPL_0_1_2_3; //0x3f00
#endif

static const int xilinx_ml40x_ipl2spl_table[] = {
	[IPL_NONE] = 0,
	[IPL_SOFTCLOCK] = NOINTS,
	[IPL_SOFTSERIAL] = NOINTS,
	[IPL_VM] = NOINTS,
	[IPL_SCHED] = NOINTS,
	[IPL_HIGH] = NOINTS,
};

void
xilinx_ml40x_init(void)
{
	platform.iobus = "baseboard";
	platform.bus_reset = noop;
	platform.cons_init = xilinx_ml40x_cons_init;
	platform.iointr = emips_aic_intr;
	platform.intr_establish = emips_intr_establish;
	platform.memsize = memsize_pmt;
	/* no high resolution timer available (actually we do?) */

	/* calibrate cpu_mhz value */
	//cpu_mhz = 10;
	cpuspeed = 8; /* xxx */

	sprintf(cpu_model, "Xilinx ML%s (eMIPS)", (systype == XS_ML40x) ? "40x" : "50x");

	ipl2spl_table = xilinx_ml40x_ipl2spl_table;
}

static void
xilinx_ml40x_cons_init(void)
{
	/*
	 * Map the USART 1:1, we just turned on the TLB.
	 * NB: This must be a wired TLB entry lest we lose it before autoconf().
	 */
#if 0
	pmap_kenter_pa(USART_DEFAULT_ADDRESS,
	    USART_DEFAULT_ADDRESS,VM_PROT_WRITE|VM_PROT_READ);
#else
	struct mips1_tlb {
		u_int32_t tlb_hi;
		u_int32_t tlb_lo;
	} tlb;
	void mips1_TLBWrite(int, struct mips1_tlb *);

	tlb.tlb_hi = USART_DEFAULT_ADDRESS;
	tlb.tlb_lo = USART_DEFAULT_ADDRESS | 0xf02;
	mips1_TLBWrite(3, &tlb);
#endif

	dz_ebus_cnsetup(USART_DEFAULT_ADDRESS);
}
