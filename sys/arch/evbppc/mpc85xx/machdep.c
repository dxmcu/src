/*	$NetBSD: machdep.c,v 1.2 2011/01/18 01:10:25 matt Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

__KERNEL_RCSID(0, "$NetSBD$");

#include "opt_mpc85xx.h"
#include "opt_altivec.h"
#include "opt_pci.h"
#include "opt_ddb.h"
#include "gpio.h"
#include "pci.h"

#define	DDRC_PRIVATE
#define	GLOBAL_PRIVATE
#define	L2CACHE_PRIVATE
#define _POWERPC_BUS_DMA_PRIVATE

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/intr.h>
#include <sys/msgbuf.h>
#include <sys/tty.h>
#include <sys/kcore.h>
#include <sys/bitops.h>
#include <sys/bus.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <prop/proplib.h>

#include <machine/stdarg.h>

#include <dev/cons.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <net/if.h>
#include <net/if_media.h>
#include <dev/mii/miivar.h>

#include <powerpc/pcb.h>
#include <powerpc/spr.h>
#include <powerpc/booke/spr.h>

#include <powerpc/booke/cpuvar.h>
#include <powerpc/booke/e500reg.h>
#include <powerpc/booke/e500var.h>
#include <powerpc/booke/etsecreg.h>
#include <powerpc/booke/openpicreg.h>
#ifdef CADMUS
#include <evbppc/mpc85xx/cadmusreg.h>
#endif
#ifdef PIXIS
#include <evbppc/mpc85xx/pixisreg.h>
#endif

void	initppc(vaddr_t, vaddr_t);

#define	MEMREGIONS	4
phys_ram_seg_t physmemr[MEMREGIONS];         /* All memory */
phys_ram_seg_t availmemr[MEMREGIONS];        /* Available memory */
static u_int nmemr;

#ifndef CONSFREQ
# define CONSFREQ	-1            /* inherit from firmware */
#endif
#ifndef CONSPEED
# define CONSPEED	115200
#endif
#ifndef CONMODE
# define CONMODE	((TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8)
#endif
#ifndef CONSADDR
# define CONSADDR	DUART2_BASE
#endif

int		comcnfreq  = CONSFREQ;
int		comcnspeed = CONSPEED;
tcflag_t	comcnmode  = CONMODE;
bus_addr_t	comcnaddr  = (bus_addr_t)CONSADDR;

#if NPCI > 0
struct extent *pcimem_ex;
struct extent *pciio_ex;
#endif

struct powerpc_bus_space gur_bst = {
	.pbs_flags = _BUS_SPACE_BIG_ENDIAN|_BUS_SPACE_MEM_TYPE,
	.pbs_offset = GUR_BASE,
	.pbs_limit = GUR_SIZE,
};

const bus_space_handle_t gur_bsh = (bus_space_handle_t)(uintptr_t)(GUR_BASE);

#ifdef CADMUS
static uint8_t cadmus_pci;
static uint8_t cadmus_csr;
static uint64_t e500_sys_clk = 33333333; /* 33.333333Mhz */
#elif defined(PIXIS)
static const uint32_t pixis_spd_map[8] = {
    [PX_SPD_33MHZ] = 33333333,
    [PX_SPD_40MHZ] = 40000000,
    [PX_SPD_50MHZ] = 50000000,
    [PX_SPD_66MHZ] = 66666666,
    [PX_SPD_83MHZ] = 83333333,
    [PX_SPD_133MHZ] = 100000000,
    [PX_SPD_133MHZ] = 133333333,
    [PX_SPD_166MHZ] = 166666667,
};
static uint8_t pixis_spd;
static uint64_t e500_sys_clk;
#elif defined(SYS_CLK)
static uint64_t e500_sys_clk = SYS_CLK;
#else
static uint64_t e500_sys_clk = 66666667; /* 66.666667Mhz */
#endif

static int e500_cngetc(dev_t);
static void e500_cnputc(dev_t, int);

static struct consdev e500_earlycons = {
	.cn_getc = e500_cngetc,
	.cn_putc = e500_cnputc,
	.cn_pollc = nullcnpollc,
};

/*
 * List of port-specific devices to attach to the processor local bus.
 */
static const struct cpunode_locators mpc8548_cpunode_locs[] = {
	{ "cpu" },	/* not a real device */
	{ "wdog" },	/* not a real device */
	{ "duart", DUART1_BASE, 2*DUART_SIZE, 0, 1,
		{ ISOURCE_DUART },
		1 + ilog2(DEVDISR_DUART) },
#if defined(MPC8548) || defined(MPC8572)
	{ "tsec", ETSEC1_BASE, ETSEC_SIZE, 1, 3,
		{ ISOURCE_ETSEC1_TX, ISOURCE_ETSEC1_RX, ISOURCE_ETSEC1_ERR },
		1 + ilog2(DEVDISR_TSEC1) },
	{ "tsec", ETSEC2_BASE, ETSEC_SIZE, 2, 3,
		{ ISOURCE_ETSEC2_TX, ISOURCE_ETSEC2_RX, ISOURCE_ETSEC2_ERR },
		1 + ilog2(DEVDISR_TSEC2) },
	{ "tsec", ETSEC3_BASE, ETSEC_SIZE, 3, 3,
		{ ISOURCE_ETSEC3_TX, ISOURCE_ETSEC3_RX, ISOURCE_ETSEC3_ERR },
		1 + ilog2(DEVDISR_TSEC3) },
	{ "tsec", ETSEC4_BASE, ETSEC_SIZE, 4, 3,
		{ ISOURCE_ETSEC4_TX, ISOURCE_ETSEC4_RX, ISOURCE_ETSEC4_ERR },
		1 + ilog2(DEVDISR_TSEC4) },
#endif
#if defined(MPC8544) || defined(MPC8536)
	{ "tsec", ETSEC1_BASE, ETSEC_SIZE, 1, 3,
		{ ISOURCE_ETSEC1_TX, ISOURCE_ETSEC1_RX, ISOURCE_ETSEC1_ERR },
		1 + ilog2(DEVDISR_TSEC1) },
	{ "tsec", ETSEC3_BASE, ETSEC_SIZE, 2, 3,
		{ ISOURCE_ETSEC3_TX, ISOURCE_ETSEC3_RX, ISOURCE_ETSEC3_ERR },
		1 + ilog2(DEVDISR_TSEC2) },
#endif
	{ "diic", I2C1_BASE, 2*I2C_SIZE, 0, 1,
		{ ISOURCE_I2C },
		1 + ilog2(DEVDISR_TSEC2) },
#ifndef MPC8572
	/* MPC8572 doesn't have any GPIO */
	{ "gpio", GLOBAL_BASE, GLOBAL_SIZE, 0, 0 },
#endif
	{ "ddrc", DDRC1_BASE, DDRC_SIZE, 0, 1,
		{ ISOURCE_DDR },
		1 + ilog2(DEVDISR_TSEC2) },
#if defined(MPC8544) || defined(MPC8536)
	{ "pcie", PCIE1_BASE, PCI_SIZE, 1, 1,
		{ ISOURCE_PCIEX },
		1 + ilog2(DEVDISR_PCIE) },
	{ "pcie", PCIE2_MPC8544_BASE, PCI_SIZE, 2, 1,
		{ ISOURCE_PCIEX2 },
		1 + ilog2(DEVDISR_PCIE3) },
	{ "pcie", PCIE3_MPC8544_BASE, PCI_SIZE, 3, 1,
		{ ISOURCE_PCIEX3 },
		1 + ilog2(DEVDISR_PCIE2) },
	{ "pci", PCIX1_MPC8544_BASE, PCI_SIZE, 1, 1,
		{ ISOURCE_PCI1 },
		1 + ilog2(DEVDISR_PCI1) },
#endif
#ifdef MPC8548
	{ "pcie", PCIE1_BASE, PCI_SIZE, 0, 1,
		{ ISOURCE_PCIEX },
		1 + ilog2(DEVDISR_PCIE) },
	{ "pci", PCIX1_MPC8548_BASE, PCI_SIZE, 1, 1,
		{ ISOURCE_PCI1 },
		1 + ilog2(DEVDISR_PCI1) },
	{ "pci", PCIX2_MPC8548_BASE, PCI_SIZE, 2, 1,
		{ ISOURCE_PCI2 },
		1 + ilog2(DEVDISR_PCI2) },
#endif
#ifdef MPC8536
	{ "ehci", USB1_BASE, USB_SIZE, 1, 1,
		{ ISOURCE_USB1 },
		1 + ilog2(DEVDISR_USB1) },
	{ "ehci", USB2_BASE, USB_SIZE, 2, 1,
		{ ISOURCE_USB2 },
		1 + ilog2(DEVDISR_USB2) },
	{ "ehci", USB3_BASE, USB_SIZE, 3, 1,
		{ ISOURCE_USB3 },
		1 + ilog2(DEVDISR_USB3) },
	{ "sata", SATA1_BASE, SATA_SIZE, 1, 1,
		{ ISOURCE_SATA1 },
		1 + ilog2(DEVDISR_SATA1) },
	{ "sata", SATA2_BASE, SATA_SIZE, 2, 1,
		{ ISOURCE_SATA2 },
		1 + ilog2(DEVDISR_SATA2) },
	{ "spi", SPI_BASE, SPI_SIZE, 0, 1,
		{ ISOURCE_SPI },
		1 + ilog2(DEVDISR_SPI) },
	{ "sdhc", ESDHC_BASE, ESDHC_SIZE, 0, 1,
		{ ISOURCE_ESDHC },
		1 + ilog2(DEVDISR_ESDHC) },
#endif
	{ "lbc", LBC_BASE, LBC_SIZE, 0, 1,
		{ ISOURCE_LBC },
		1 + ilog2(DEVDISR_LBC) },
	//{ "sec", RNG_BASE, RNG_SIZE, 0, 0, },
	{ NULL }
};

static int
e500_cngetc(dev_t dv)
{
	volatile uint8_t * const com0addr = (void *)(GUR_BASE+CONSADDR);

        if ((com0addr[com_lsr] & LSR_RXRDY) == 0)
		return -1;

	return com0addr[com_data] & 0xff;
}

static void
e500_cnputc(dev_t dv, int c)
{               
	volatile uint8_t * const com0addr = (void *)(GUR_BASE+CONSADDR);
	int timo = 150000;
		
	while ((com0addr[com_lsr] & LSR_TXRDY) == 0 && --timo > 0)
		;

	com0addr[com_data] = c;
	__asm("mbar");
			
	while ((com0addr[com_lsr] & LSR_TSRE) == 0 && --timo > 0)
		;
}

static void *
gur_tlb_mapiodev(paddr_t pa, psize_t len)
{
	if (pa < gur_bst.pbs_offset)
		return NULL;
	if (pa + len > gur_bst.pbs_offset + gur_bst.pbs_limit)
		return NULL;
	return (void *)pa;
}

static void *(* const early_tlb_mapiodev)(paddr_t, psize_t) = gur_tlb_mapiodev;

static void
e500_cpu_reset(void)
{
	__asm volatile("sync");
	cpu_write_4(GLOBAL_BASE + RSTCR, HRESET_REQ);
	__asm volatile("msync;isync");
}

static psize_t
memprobe(vaddr_t endkernel)
{
	phys_ram_seg_t *mr;

	/*
	 * First we need to find out how much physical memory we have.
	 * We could let our bootloader tell us, but it's almost as easy
	 * to ask the DDR memory controller.
	 */
	mr = physmemr;
#if 1
	for (u_int i = 0; i < 4; i++) {
		uint32_t v = cpu_read_4(DDRC1_BASE + CS_CONFIG(i));
		if (v & CS_CONFIG_EN) {
			v = cpu_read_4(DDRC1_BASE + CS_BNDS(i));
			mr->start = BNDS_SA_GET(v);
			mr->size  = BNDS_SIZE_GET(v);
			mr++;
		}
	}

	if (mr == physmemr)
		panic("no memory configured!");
#else
	mr->start = 0;
	mr->size = 32 << 20;
	mr++;
#endif

	/*
	 * Sort memory regions from low to high and coalesce adjacent regions
	 */
	u_int cnt = mr - physmemr;
	if (cnt > 1) {
		for (u_int i = 0; i < cnt - 1; i++) {
			for (u_int j = i + 1; j < cnt; j++) {
				if (physmemr[j].start < physmemr[i].start) {
					phys_ram_seg_t tmp = physmemr[i];
					physmemr[i] = physmemr[j];
					physmemr[j] = tmp;
				}
			}
		}
		mr = physmemr;
		for (u_int i = 0; i < cnt; i++, mr++) {
			if (mr->start + mr->size == mr[1].start) {
				mr->size += mr[1].size;
				for (u_int j = 1; j < cnt - i; j++)
					mr[j] = mr[j+1];
				cnt--;
			}
		}
	}

	/*
	 * Copy physical memory to available memory.
	 */
	memcpy(availmemr, physmemr, cnt * sizeof(physmemr[0]));

	/*
	 * Adjust available memory to skip kernel at start of memory.
	 */
	availmemr[0].size -= endkernel - availmemr[0].start;
	availmemr[0].start = endkernel;

	/*
	 * Steal pages at the end of memory for the kernel message buffer.
	 */
	availmemr[cnt-1].size -= round_page(MSGBUFSIZE);
	msgbuf_paddr =
	    (uintptr_t)(availmemr[cnt-1].start + availmemr[cnt-1].size);

	/*
	 * Calculate physmem.
	 */
	for (u_int i = 0; i < cnt; i++)
		physmem += atop(physmemr[i].size);

	nmemr = cnt;
	return physmemr[cnt-1].start + physmemr[cnt-1].size;
}

void
consinit(void)
{
	static bool attached = false;

	if (attached)
		return;
	attached = true;

	if (comcnfreq == -1) {
		const uint32_t porpplsr = cpu_read_4(GLOBAL_BASE + PORPLLSR);
		const uint32_t plat_ratio = PLAT_RATIO_GET(porpplsr);
		comcnfreq = e500_sys_clk * plat_ratio;
		printf(" comcnfreq=%u", comcnfreq);
	}

	comcnattach(&gur_bst, comcnaddr, comcnspeed, comcnfreq,
	    COM_TYPE_NORMAL, comcnmode);
}

void
cpu_probe_cache(void)
{
	struct cpu_info * const ci = curcpu();
	const uint32_t l1cfg0 = mfspr(SPR_L1CFG0);

	ci->ci_ci.dcache_size = L1CFG_CSIZE_GET(l1cfg0);
	ci->ci_ci.dcache_line_size = 32 << L1CFG_CBSIZE_GET(l1cfg0);

	if (L1CFG_CARCH_GET(l1cfg0) == L1CFG_CARCH_HARVARD) {
		const uint32_t l1cfg1 = mfspr(SPR_L1CFG1);

		ci->ci_ci.icache_size = L1CFG_CSIZE_GET(l1cfg1);
		ci->ci_ci.icache_line_size = 32 << L1CFG_CBSIZE_GET(l1cfg1);
	} else {
		ci->ci_ci.icache_size = ci->ci_ci.dcache_size;
		ci->ci_ci.icache_line_size = ci->ci_ci.dcache_line_size;
	}

#ifdef DEBUG
	uint32_t l1csr0 = mfspr(SPR_L1CSR0);
	if ((L1CSR_CE & l1csr0) == 0)
		printf(" DC=off");

	uint32_t l1csr1 = mfspr(SPR_L1CSR1);
	if ((L1CSR_CE & l1csr1) == 0)
		printf(" IC=off");
#endif
}

static const char *
socname(uint32_t svr)
{
	svr &= ~0x80000;
	switch (svr >> 8) {
	case SVR_MPC8548v2 >> 8: return "MPC8548";
	case SVR_MPC8547v2 >> 8: return "MPC8547";
	case SVR_MPC8545v2 >> 8: return "MPC8545";
	case SVR_MPC8543v2 >> 8: return "MPC8543";
	case SVR_MPC8544v1 >> 8: return "MPC8544";
	case SVR_MPC8536v1 >> 8: return "MPC8536";
	case SVR_MPC8572 >> 8: return "MPC8572";
	default:
		panic("%s: unknown SVR %#x", __func__, svr);
	}
}

static void
e500_tlb_print(device_t self, const char *name, uint32_t tlbcfg)
{
	static const char units[16] = "KKKKKMMMMMGGGGGT";

	const uint32_t minsize = 1U << (2 * TLBCFG_MINSIZE(tlbcfg));
	const uint32_t assoc = TLBCFG_ASSOC(tlbcfg);
	const u_int maxsize_log4k = TLBCFG_MAXSIZE(tlbcfg);
	const uint64_t maxsize = 1ULL << (2 * maxsize_log4k % 10);
	const uint32_t nentries = TLBCFG_NENTRY(tlbcfg);

	aprint_normal_dev(self, "%s:", name);

	aprint_normal(" %u", nentries);
	if (TLBCFG_AVAIL_P(tlbcfg)) {
		aprint_normal(" variable-size (%uKB..%"PRIu64"%cB)",
		    minsize, maxsize, units[maxsize_log4k]);
	} else {
		aprint_normal(" fixed-size (%uKB)", minsize);
	}
	if (assoc == 0 || assoc == nentries)
		aprint_normal(" fully");
	else
		aprint_normal(" %u-way set", assoc);
	aprint_normal(" associative entries\n");
}

static void
e500_cpu_attach(device_t self, u_int instance)
{
	struct cpu_info * const ci = &cpu_info[instance];
	
	KASSERT(instance == 0);
	self->dv_private = ci;

	ci->ci_cpuid = instance;
	ci->ci_dev = self;
        //ci->ci_idlespin = cpu_idlespin;
	if (instance > 0) {
		ci->ci_idepth = -1;
		cpu_probe_cache();
	}

	uint64_t freq = board_info_get_number("processor-frequency");
	char freqbuf[10];
	if (freq >= 999500000) {
		const uint32_t freq32 = (freq + 500000) / 10000000;
		snprintf(freqbuf, sizeof(freqbuf), "%u.%02u GHz",
		    freq32 / 100, freq32 % 100);
	} else {
		const uint32_t freq32 = (freq + 500000) / 1000000;
		snprintf(freqbuf, sizeof(freqbuf), "%u MHz", freq32);
	}

	const uint32_t pvr = mfpvr();
	const uint32_t svr = mfspr(SPR_SVR);
	const uint32_t pir = mfspr(SPR_PIR);

	aprint_normal_dev(self, "%s %s%s %u.%u with an e500%s %u.%u core, "
	   "ID %u%s\n",
	   freqbuf, socname(svr), (SVR_SECURITY_P(svr) ? "E" : ""),
	   (svr >> 4) & 15, svr & 15,
	   (pvr >> 16) == PVR_MPCe500v2 ? "v2" : "",
	   (pvr >> 4) & 15, pvr & 15,
	   pir, (pir == 0 ? " (Primary)" : ""));

	const uint32_t l1cfg0 = mfspr(SPR_L1CFG0);
	aprint_normal_dev(self,
	    "%uKB/%uB %u-way L1 %s cache\n",
	    L1CFG_CSIZE_GET(l1cfg0) >> 10,
	    32 << L1CFG_CBSIZE_GET(l1cfg0),
	    L1CFG_CNWAY_GET(l1cfg0),
	    L1CFG_CARCH_GET(l1cfg0) == L1CFG_CARCH_HARVARD
		? "data" : "unified");
	    
	if (L1CFG_CARCH_GET(l1cfg0) == L1CFG_CARCH_HARVARD) {
		const uint32_t l1cfg1 = mfspr(SPR_L1CFG1);
		aprint_normal_dev(self,
		    "%uKB/%uB %u-way L1 %s cache\n",
		    L1CFG_CSIZE_GET(l1cfg1) >> 10,
		    32 << L1CFG_CBSIZE_GET(l1cfg1),
		    L1CFG_CNWAY_GET(l1cfg1),
		    "instruction");
	}

	const uint32_t mmucfg = mfspr(SPR_MMUCFG);
	aprint_normal_dev(self,
	    "%u TLBs, %u concurrent %u-bit PIDs (%u total)\n",
	    MMUCFG_NTLBS_GET(mmucfg) + 1,
	    MMUCFG_NPIDS_GET(mmucfg),
	    MMUCFG_PIDSIZE_GET(mmucfg) + 1,
	    1 << (MMUCFG_PIDSIZE_GET(mmucfg) + 1));

	e500_tlb_print(self, "tlb0", mfspr(SPR_TLB0CFG));
	e500_tlb_print(self, "tlb1", mfspr(SPR_TLB1CFG));

	intr_cpu_init(ci);
	cpu_evcnt_attach(ci);
}

static void
calltozero(void)
{
	panic("call to 0 from %p", __builtin_return_address(0));
}

void
initppc(vaddr_t startkernel, vaddr_t endkernel)
{
	struct cpu_info * const ci = curcpu();
	struct cpu_softc * const cpu = ci->ci_softc;

	cn_tab = &e500_earlycons;
	printf(" initppc<enter>");

	const register_t hid0 = mfspr(SPR_HID0);
	mtspr(SPR_HID0, hid0 | HID0_TBEN | HID0_EMCP);
#ifdef CADMUS
	/*
	 * Need to cache this from cadmus since we need to unmap cadmus since
	 * it falls in the middle of kernel address space.
	 */
	cadmus_pci = ((uint8_t *)0xf8004000)[CM_PCI];
	cadmus_csr = ((uint8_t *)0xf8004000)[CM_CSR];
	((uint8_t *)0xf8004000)[CM_CSR] |= CM_RST_PHYRST;
	printf(" cadmus_pci=%#x", cadmus_pci);
	printf(" cadmus_csr=%#x", cadmus_csr);
	((uint8_t *)0xf8004000)[CM_CSR] = 0;
	if ((cadmus_pci & CM_PCI_PSPEED) == CM_PCI_PSPEED_66) {
		e500_sys_clk *= 2;
	}
#endif
#ifdef PIXIS
	pixis_spd = ((uint8_t *)PX_BASE)[PX_SPD];
	printf(" pixis_spd=%#x ", pixis_spd);
	e500_sys_clk = pixis_spd_map[PX_SPD_SYSCLK_GET(pixis_spd)];
#endif
	printf(" porpllsr=0x%08x",
	    *(uint32_t *)(GUR_BASE + GLOBAL_BASE + PORPLLSR));
	printf(" sys_clk=%"PRIu64, e500_sys_clk);

	/*
	 * Make sure arguments are page aligned.
	 */
	startkernel = trunc_page(startkernel);
	endkernel = round_page(endkernel);

	/*
	 * Initialize the bus space tag used to access the 85xx general
	 * utility registers.  It doesn't need to be extent protected.
	 * We know the GUR is mapped via a TLB1 entry so we add a limited
	 * mapiodev which allows mappings in GUR space.
	 */
	CTASSERT(offsetof(struct tlb_md_ops, md_tlb_mapiodev) == 0);
	cpu_md_ops.md_tlb_ops = (const void *)&early_tlb_mapiodev;
	bus_space_init(&gur_bst, NULL, NULL, 0);
	cpu->cpu_bst = &gur_bst;
	cpu->cpu_bsh = gur_bsh;

	/*
	 * Attach the console early, really early.
	 */
	consinit();

	/*
	 * Reset the PIC to a known state.
	 */
	cpu_write_4(OPENPIC_BASE + OPENPIC_GCR, GCR_RST);
	while (cpu_read_4(OPENPIC_BASE + OPENPIC_GCR) & GCR_RST)
		;
#if 0
	cpu_write_4(OPENPIC_BASE + OPENPIC_CTPR, 15);	/* IPL_HIGH */
#endif
	printf(" openpic-reset(ctpr=%u)",
	    cpu_read_4(OPENPIC_BASE + OPENPIC_CTPR));

	/*
	 * fill in with an absolute branch to a routine that will panic.
	 */
	*(int *)0 = 0x48000002 | (int) calltozero;

	/*
	 * Get the cache sizes.
	 */
	cpu_probe_cache();
		printf(" cache(DC=%u/%u,IC=%u/%u)",
		    ci->ci_ci.dcache_size >> 10,
		    ci->ci_ci.dcache_line_size,
		    ci->ci_ci.icache_size >> 10,
		    ci->ci_ci.icache_line_size);

	/*
	 * Now find out how much memory is attached
	 */
	pmemsize = memprobe(endkernel);
		printf(" memprobe=%zuMB", (size_t) (pmemsize >> 20));

	/*
	 * Now we need cleanout the TLB of stuff that we don't need.
	 */
	e500_tlb_init(endkernel, pmemsize);
		printf(" e500_tlbinit(%#lx,%zuMB)",
		    endkernel, (size_t) (pmemsize >> 20));

	/*
	 *
	 */
	printf(" hid0=%#lx/%#lx", hid0, mfspr(SPR_HID0));
	printf(" hid1=%#lx", mfspr(SPR_HID1));
	printf(" pordevsr=%#x", cpu_read_4(GLOBAL_BASE + PORDEVSR));
	printf(" devdisr=%#x", cpu_read_4(GLOBAL_BASE + DEVDISR));

	mtmsr(mfmsr() | PSL_CE | PSL_ME | PSL_DE);

	/*
	 * Initialize the message buffer.
	 */
	initmsgbuf((void *)msgbuf_paddr, round_page(MSGBUFSIZE));
	printf(" msgbuf=%p", (void *)msgbuf_paddr);

	/*
	 * Initialize exception vectors and interrupts
	 */
	exception_init(&e500_intrsw);
	printf(" exception_init=%p", &e500_intrsw);
	mtspr(SPR_TCR, TCR_WIE | mfspr(SPR_TCR));

	/*
	 * Set the page size.
	 */
	uvm_setpagesize();

	/*
	 * Initialize the pmap.
	 */
	pmap_bootstrap(startkernel, endkernel, availmemr, nmemr);

	/*
	 * Let's take all the indirect calls via our stubs and patch 
	 * them to be direct calls.
	 */
	booke_fixup_stubs();
#if 0
	/*
	 * As a debug measure we can change the TLB entry that maps all of
	 * memory to one that encompasses the 64KB with the kernel vectors.
	 * All other pages will be soft faulted into the TLB as needed.
	 */
	const uint32_t saved_mas0 = mfspr(SPR_MAS0);
	mtspr(SPR_MAS6, 0);
	__asm volatile("tlbsx\t0, %0" :: "b"(startkernel));
	uint32_t mas0 = mfspr(SPR_MAS0);
	uint32_t mas1 = mfspr(SPR_MAS1);
	uint32_t mas2 = mfspr(SPR_MAS2);
	uint32_t mas3 = mfspr(SPR_MAS3);
	KASSERT(mas3 & MAS3_SW);
	KASSERT(mas3 & MAS3_SR);
	KASSERT(mas3 & MAS3_SX);
	mas1 = (mas1 & ~MAS1_TSIZE) | MASX_TSIZE_64KB;
	pt_entry_t xpn_mask = ~0 << (10 + 2 * MASX_TSIZE_GET(mas1));
	mas2 = (mas2 & ~(MAS2_EPN        )) | (startkernel & xpn_mask);
	mas3 = (mas3 & ~(MAS3_RPN|MAS3_SW)) | (startkernel & xpn_mask);
	printf(" %#lx=<%#x,%#x,%#x,%#x>", startkernel, mas0, mas1, mas2, mas3);
#if 1
	mtspr(SPR_MAS1, mas1);
	mtspr(SPR_MAS2, mas2);
	mtspr(SPR_MAS3, mas3);
	extern void tlbwe(void);
	tlbwe();
	mtspr(SPR_MAS0, saved_mas0);
	printf("(ok)");
#endif
#endif

	/*
	 * Set some more MD helpers
	 */
	cpu_md_ops.md_cpunode_locs = mpc8548_cpunode_locs;
	cpu_md_ops.md_device_register = e500_device_register;
	cpu_md_ops.md_cpu_attach = e500_cpu_attach;
	cpu_md_ops.md_cpu_reset = e500_cpu_reset;
#if NGPIO > 0
	cpu_md_ops.md_cpunode_attach = pq3gpio_attach;
#endif

		printf(" initppc done!\n");
}

#ifdef MPC8548
static const char * const mpc8548cds_extirq_names[] = {
	[0] = "pci inta",
	[1] = "pci intb",
	[2] = "pci intc",
	[3] = "pci intd",
	[4] = "irq4",
	[5] = "gige phy",
	[6] = "atm phy",
	[7] = "cpld",
	[8] = "irq8",
	[9] = "nvram",
	[10] = "debug",
	[11] = "pci2 inta",
};
#endif

static const char * const mpc85xx_extirq_names[] = {
	[0] = "extirq 0",
	[1] = "extirq 1",
	[2] = "extirq 2",
	[3] = "extirq 3",
	[4] = "extirq 4",
	[5] = "extirq 5",
	[6] = "extirq 6",
	[7] = "extirq 7",
	[8] = "extirq 8",
	[9] = "extirq 9",
	[10] = "extirq 10",
	[11] = "extirq 11",
};

static void
mpc85xx_extirq_setup(void)
{
#ifdef MPC8548
	const char * const * names = mpc8548cds_extirq_names;
	const size_t n = __arraycount(mpc8548cds_extirq_names);
#else
	const char * const * names = mpc85xx_extirq_names;
	const size_t n = __arraycount(mpc85xx_extirq_names);
#endif
	prop_array_t extirqs = prop_array_create_with_capacity(n);
	for (u_int i = 0; i < n; i++) {
		prop_string_t ps = prop_string_create_cstring_nocopy(names[i]);
		prop_array_set(extirqs, i, ps);
		prop_object_release(ps);
	}
	board_info_add_object("external-irqs", extirqs);
	prop_object_release(extirqs);
}

static void
mpc85xx_pci_setup(const char *name, uint32_t intmask, int ist, int inta, ...)
{
	prop_dictionary_t pci_intmap = prop_dictionary_create();
	KASSERT(pci_intmap != NULL);
	prop_number_t mask = prop_number_create_unsigned_integer(intmask);
	KASSERT(mask != NULL);
	prop_dictionary_set(pci_intmap, "interrupt-mask", mask);
	prop_object_release(mask);
	prop_number_t pn_ist = prop_number_create_unsigned_integer(ist);
	KASSERT(pn_ist != NULL);
	prop_number_t pn_intr = prop_number_create_unsigned_integer(inta);
	KASSERT(pn_intr != NULL);
	prop_dictionary_t entry = prop_dictionary_create();
	KASSERT(entry != NULL);
	prop_dictionary_set(entry, "interrupt", pn_intr);
	prop_dictionary_set(entry, "type", pn_ist);
	prop_dictionary_set(pci_intmap, "000000", entry);
	prop_object_release(pn_intr);
	prop_object_release(entry);
	va_list ap;
	va_start(ap, inta);
	u_int intrinc = __LOWEST_SET_BIT(intmask);
	for (u_int i = 0; i < intmask; i += intrinc) {
		char prop_name[12];
		snprintf(prop_name, sizeof(prop_name), "%06x", i + intrinc);
		entry = prop_dictionary_create();
		KASSERT(entry != NULL);
		pn_intr = prop_number_create_unsigned_integer(va_arg(ap, u_int));
		KASSERT(pn_intr != NULL);
		prop_dictionary_set(entry, "interrupt", pn_intr);
		prop_dictionary_set(entry, "type", pn_ist);
		prop_dictionary_set(pci_intmap, prop_name, entry);
		prop_object_release(pn_intr);
		prop_object_release(entry);
	}
	va_end(ap);
	prop_object_release(pn_ist);
	board_info_add_object(name, pci_intmap);
	prop_object_release(pci_intmap);
}

void
cpu_startup(void)
{
	struct cpu_info * const ci = curcpu();

	booke_cpu_startup(socname(mfspr(SPR_SVR)));

	uint32_t v = cpu_read_4(GLOBAL_BASE + PORPLLSR);
	uint32_t plat_ratio = PLAT_RATIO_GET(v);
	uint32_t e500_ratio = E500_RATIO_GET(v);

	uint64_t ccb_freq = e500_sys_clk * plat_ratio;
	uint64_t cpu_freq = ccb_freq * e500_ratio / 2;

	ci->ci_khz = (cpu_freq + 500) / 1000;
	cpu_timebase = ci->ci_data.cpu_cc_freq = ccb_freq / 8;

	board_info_add_bool("pq3");
	board_info_add_number("mem-size", pmemsize);
	const uint32_t l2ctl = cpu_read_4(L2CACHE_BASE + L2CTL);
	uint32_t l2siz = L2CTL_L2SIZ_GET(l2ctl);
	uint32_t l2banks = l2siz >> 16;
#ifdef MPC85555
	if (e500_get_svr() == (MPC8555v1 >> 16)) {
		l2siz >>= 1;
		l2banks >>= 1;
	}
#endif
	board_info_add_number("l2-cache-size", l2siz);
	board_info_add_number("l2-cache-line-size", 32);
	board_info_add_number("l2-cache-banks", l2banks);
	board_info_add_number("l2-cache-ways", 8);

	board_info_add_number("processor-frequency", cpu_freq);
	board_info_add_number("bus-frequency", ccb_freq);
	board_info_add_number("pci-frequency", e500_sys_clk);
	board_info_add_number("timebase-frequency", ccb_freq / 8);

#ifdef CADMUS
	const uint8_t phy_base = CM_CSR_EPHY_GET(cadmus_csr) << 2;
	board_info_add_number("tsec1-phy-addr", phy_base + 0);
	board_info_add_number("tsec2-phy-addr", phy_base + 1);
	board_info_add_number("tsec3-phy-addr", phy_base + 2);
	board_info_add_number("tsec4-phy-addr", phy_base + 3);
#else
	board_info_add_number("tsec1-phy-addr", MII_PHY_ANY);
	board_info_add_number("tsec2-phy-addr", MII_PHY_ANY);
	board_info_add_number("tsec3-phy-addr", MII_PHY_ANY);
	board_info_add_number("tsec4-phy-addr", MII_PHY_ANY);
#endif

	uint64_t macstnaddr =
	    ((uint64_t)le32toh(cpu_read_4(ETSEC1_BASE + MACSTNADDR1)) << 16)
	    | ((uint64_t)le32toh(cpu_read_4(ETSEC1_BASE + MACSTNADDR2)) << 48);
	board_info_add_data("tsec-mac-addr-base", &macstnaddr, 6);

#if NPCI > 0 && defined(PCI_MEMBASE)
	pcimem_ex = extent_create("pcimem",
	    PCI_MEMBASE, PCI_MEMBASE + 4*PCI_MEMSIZE,
	    M_DEVBUF, NULL, 0, EX_WAITOK);
#endif
#if NPCI > 0 && defined(PCI_IOBASE)
	pciio_ex = extent_create("pciio",
	    PCI_IOBASE, PCI_IOBASE + 4*PCI_IOSIZE,
	    M_DEVBUF, NULL, 0, EX_WAITOK);
#endif
	mpc85xx_extirq_setup();
	/*
	 * PCI-Express virtual wire interrupts on combined with
	 * External IRQ0/1/2/3.
	 */
#if defined(MPC8548)
	mpc85xx_pci_setup("pcie0-interrupt-map", 0x001800, IST_LEVEL, 0, 1, 2, 3);
#endif
#if defined(MPC8544) || defined(MPC8572) || defined(MPC8536)
	mpc85xx_pci_setup("pcie1-interrupt-map", 0x001800, IST_LEVEL, 0, 1, 2, 3);
	mpc85xx_pci_setup("pcie2-interrupt-map", 0x001800, IST_LEVEL, 4, 5, 6, 7);
	mpc85xx_pci_setup("pcie3-interrupt-map", 0x001800, IST_LEVEL, 8, 9, 10, 11);
#endif
#if defined(MPC8544) || defined(MPC8548)
	mpc85xx_pci_setup("pci1-interrupt-map", 0x001800, IST_LEVEL, 0, 1, 2, 3);
#endif
#if defined(MPC8536)
	mpc85xx_pci_setup("pci1-interrupt-map", 0x001800, IST_LEVEL, 1, 2, 3, 4);
#endif
#if defined(MPC8548)
	mpc85xx_pci_setup("pci2-interrupt-map", 0x001800, IST_LEVEL, 11, 1, 2, 3);
#endif
}