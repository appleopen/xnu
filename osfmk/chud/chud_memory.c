/*
 * Copyright (c) 2003-2007 Apple Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 * 
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#include <mach/vm_param.h>
#include <chud/chud_xnu.h>
#include <machine/machine_routines.h>

extern unsigned int IODefaultCacheBits(addr64_t pa);
extern unsigned int vm_page_free_count;
extern unsigned int vm_page_inactive_count;

__private_extern__ uint64_t 
chudxnu_avail_memory_size(void)
{
    return max_mem;
}

__private_extern__ uint64_t 
chudxnu_phys_memory_size(void)
{
    return mem_actual;
}

/* 
 * This function is not intended to be valid for any amount of time,
 * it is just an instantaneous snapshot of the current free memory size.
 */
__private_extern__
uint64_t chudxnu_free_memory_size(void)
{
    return (uint64_t)vm_page_free_count * (uint64_t)page_size;
}

/*
 * This function is not intended to be valid for any amount of time,
 * it is just an instantaneous snapshot of the current inactive memory size.
 */
__private_extern__
uint64_t chudxnu_inactive_memory_size(void)
{
	return (uint64_t)vm_page_inactive_count * (uint64_t)page_size;
}

#pragma mark *** DEPRECATED ***

// DEPRECATED
__private_extern__
vm_offset_t chudxnu_io_map(uint64_t phys_addr, vm_size_t size)
{
    return ml_io_map(phys_addr, size); // XXXXX limited to first 2GB XXXXX
}

// DEPRECATED
__private_extern__ uint32_t
chudxnu_phys_addr_wimg(uint64_t phys_addr)
{
    return IODefaultCacheBits(phys_addr);
}
