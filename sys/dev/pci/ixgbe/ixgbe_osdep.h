/******************************************************************************

  Copyright (c) 2001-2010, Intel Corporation 
  All rights reserved.
  
  Redistribution and use in source and binary forms, with or without 
  modification, are permitted provided that the following conditions are met:
  
   1. Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer.
  
   2. Redistributions in binary form must reproduce the above copyright 
      notice, this list of conditions and the following disclaimer in the 
      documentation and/or other materials provided with the distribution.
  
   3. Neither the name of the Intel Corporation nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.
  
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

******************************************************************************/
/*$FreeBSD: src/sys/dev/ixgbe/ixgbe_osdep.h,v 1.9 2010/11/26 22:46:32 jfv Exp $*/
/*$NetBSD: ixgbe_osdep.h,v 1.1 2011/08/12 21:55:29 dyoung Exp $*/

#ifndef _IXGBE_OS_H_
#define _IXGBE_OS_H_

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <net/if.h>
#include <net/if_ether.h>

#define ASSERT(x) if(!(x)) panic("IXGBE: x")

/* The happy-fun DELAY macro is defined in /usr/src/sys/i386/include/clock.h */
#define usec_delay(x) DELAY(x)
#define msec_delay(x) DELAY(1000*(x))

#define DBG 0 
#define MSGOUT(S, A, B)     printf(S "\n", A, B)
#define DEBUGFUNC(F)        DEBUGOUT(F);
#if DBG
	#define DEBUGOUT(S)         printf(S "\n")
	#define DEBUGOUT1(S,A)      printf(S "\n",A)
	#define DEBUGOUT2(S,A,B)    printf(S "\n",A,B)
	#define DEBUGOUT3(S,A,B,C)  printf(S "\n",A,B,C)
	#define DEBUGOUT7(S,A,B,C,D,E,F,G)  printf(S "\n",A,B,C,D,E,F,G)
#else
	#define DEBUGOUT(S)		do { } while (/*CONSTCOND*/false)
	#define DEBUGOUT1(S,A)		do { } while (/*CONSTCOND*/false)
	#define DEBUGOUT2(S,A,B)	do { } while (/*CONSTCOND*/false)
	#define DEBUGOUT3(S,A,B,C)	do { } while (/*CONSTCOND*/false)
	#define DEBUGOUT6(S,A,B,C,D,E,F)	\
					do { } while (/*CONSTCOND*/false)
	#define DEBUGOUT7(S,A,B,C,D,E,F,G)	\
					do { } while (/*CONSTCOND*/false)
#endif

#define FALSE               0
#define false               0 /* shared code requires this */
#define TRUE                1
#define true                1
#define CMD_MEM_WRT_INVALIDATE          0x0010  /* BIT_4 */
#define PCI_COMMAND_REGISTER            PCIR_COMMAND
#define UNREFERENCED_PARAMETER(_p)


#define IXGBE_NTOHL(_i)	ntohl(_i)
#define IXGBE_NTOHS(_i)	ntohs(_i)

typedef uint8_t		u8;
typedef int8_t		s8;
typedef uint16_t	u16;
typedef uint32_t	u32;
typedef int32_t		s32;
typedef uint64_t	u64;

#define le16_to_cpu 

#if __FreeBSD_version < 800000
#if defined(__i386__) || defined(__amd64__)
#define mb()	__asm volatile("mfence" ::: "memory")
#define wmb()	__asm volatile("sfence" ::: "memory")
#define rmb()	__asm volatile("lfence" ::: "memory")
#else
#define mb()
#define rmb()
#define wmb()
#endif
#endif

#if defined(__i386__) || defined(__amd64__)
static __inline
void prefetch(void *x)
{
	__asm volatile("prefetcht0 %0" :: "m" (*(unsigned long *)x));
}
#else
#define prefetch(x)
#endif

struct ixgbe_osdep
{
	struct ethercom    ec;
	pci_chipset_tag_t  pc;
	pcitag_t           tag;
	bus_space_tag_t    mem_bus_space_tag;
	bus_space_handle_t mem_bus_space_handle;
	bus_size_t         mem_size;
	bus_dma_tag_t      dmat;
	device_t           dev;
	pci_intr_handle_t  ih;
	void               *intr;
};

/* These routines are needed by the shared code */
struct ixgbe_hw; 
extern u16 ixgbe_read_pci_cfg(struct ixgbe_hw *, u32);
#define IXGBE_READ_PCIE_WORD ixgbe_read_pci_cfg

extern void ixgbe_write_pci_cfg(struct ixgbe_hw *, u32, u16);
#define IXGBE_WRITE_PCIE_WORD ixgbe_write_pci_cfg

#define IXGBE_WRITE_FLUSH(a) IXGBE_READ_REG(a, IXGBE_STATUS)

#define IXGBE_READ_REG(a, reg) (\
   bus_space_read_4( ((a)->back)->mem_bus_space_tag, \
                     ((a)->back)->mem_bus_space_handle, \
                     reg))

#define IXGBE_WRITE_REG(a, reg, value) (\
   bus_space_write_4( ((a)->back)->mem_bus_space_tag, \
                     ((a)->back)->mem_bus_space_handle, \
                     reg, value))


#define IXGBE_READ_REG_ARRAY(a, reg, offset) (\
   bus_space_read_4( ((a)->back)->mem_bus_space_tag, \
                     ((a)->back)->mem_bus_space_handle, \
                     (reg + ((offset) << 2))))

#define IXGBE_WRITE_REG_ARRAY(a, reg, offset, value) (\
      bus_space_write_4( ((a)->back)->mem_bus_space_tag, \
                      ((a)->back)->mem_bus_space_handle, \
                      (reg + ((offset) << 2)), value))


#endif /* _IXGBE_OS_H_ */
