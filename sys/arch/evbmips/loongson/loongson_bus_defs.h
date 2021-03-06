/* $NetBSD: loongson_bus_defs.h,v 1.1 2011/08/27 13:42:45 bouyer Exp $ */

#ifndef _LOONGSON_BUS_H_
#define	_LOONGSON_BUS_H_

#include <machine/bus_defs.h>

extern struct extent *loongson_io_ex;
extern struct extent *loongson_mem_ex;
extern int	ex_mallocsafe;
extern struct mips_bus_space bonito_iot;
extern struct mips_bus_space bonito_memt;
extern struct mips_bus_dma_tag bonito_dmat;
extern struct mips_pci_chipset bonito_pc;

void    bonito_bus_io_init(bus_space_tag_t, void *);
void    bonito_bus_mem_init(bus_space_tag_t, void *);

#endif /* _LOONGSON_BUS_H_ */
