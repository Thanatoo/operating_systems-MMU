// SPDX-License-Identifier: GPL-2.0-only
/*
 *  linux/kernel/fork.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <asm/uaccess.h>
#include <asm/page.h>
#include <asm/tlbflush.h>
#include <linux/errno.h>
#include <asm/special_insns.h>
#include <linux/gfp.h>

/*
 * A function that reads cr3 from the register and
 * fetch the 51:12 bits out of it
 */
static inline uintptr_t fetch_cr3(void) {
	uintptr_t cr3;
	cr3 = __read_cr3();

	cr3 = cr3 & 0x000ffffffffff000u;
	return cr3;
}



/* 
 * Allocate Page: 
 * Allocate an empty frame and return the physical address of
 * the first byte of the frame. The system call should return
 * the physical address of the first byte of the frame allocated
 * on success and -1 on error
 */
SYSCALL_DEFINE0 (allocPage) {
  unsigned long vaddr = get_zeroed_page(GFP_KERNEL);

  if (vaddr == 0) {
      return -1;
  }
  return __pa(vaddr);
}



/* 
 * Free Page:
 * Free a frame that was allocated by allocPage()
 * return 0 on success and -1 on error
 */
SYSCALL_DEFINE1 (freePage, unsigned long, physaddr) {
  unsigned long vaddr = (unsigned long)__va(physaddr);

  if (!virt_addr_valid(vaddr)) {
      return -EINVAL;
  }
  free_page(vaddr);
  return 0;
}



/* 
 * Read MMU: 
 * Store the value of the page table entries for virtual address vaddr 
 * into the memory pointed to by pml4e, pdpte, pde, and pte.
 * Walk the page table and fill in the values that are present in each level 
 * of the page table.
 * If an entry does not exist because that level of the page table is missing, 
 * a 0 is written into the memory location.
 */
SYSCALL_DEFINE5(readMMU, unsigned long , vaddr, unsigned long * , pml4ep, unsigned long * , pdptep, unsigned long * , pdep, unsigned long * , ptep) {
	
	/* Variables */
	uintptr_t cr3;
	unsigned long pml4e, pdpte, pde, pte;
	unsigned long pml4, dirp, dir, table;
	unsigned long *_pml4e = 0, *_pdpte = 0, *_pde = 0, *_pte = 0, *_final = 0;

	/* If any of the entry point is not valid, return with error */
	if (pml4ep == NULL || pdptep == NULL || pdep == NULL || ptep == NULL) {
      return -EINVAL;
    }

	/* pml4: fetch 47:39 of the linear address */
	pml4 = (unsigned long)(vaddr & /* 47:39 */0x0000ff8000000000u);

	/* cr3: fetch 51:12 of cr3 */
	cr3 = fetch_cr3();/* 51:12 */

	/* Physical address of pml4e */
    _pml4e = (unsigned long *)((pml4 >> 36/* 47:39 -> 11:3 */) 
    			| cr3); /* 2:0 are 0 */

    /* Virtual address of pml4e */
    pml4e = *((unsigned long *)(__va(_pml4e)));

    /* 
     * Check present bit
     * If pml4e is not present, then fill other entry points to 0 and 
     * copy all values back to user space
     */
    if ((pml4e & 0x0000000000000001u) == 0x0000000000000000u) {
        printk("pml4e: not present, stop\n");
        pdpte = 0;
        pde = 0;
        pte = 0;
        goto COPY_ALL;
    }

    /* pdpte: fetch 38:30 of the linear address */
    dirp = vaddr & /* 38:30 */0x0000007fc0000000u;

    /* Physical address of pdpte */
    _pdpte = (unsigned long *)((dirp >> 27/* 38:30 -> 11:3 */) 
    			| (pml4e & /* 51:12 of pml4e */0x000ffffffffff000u)); /* 2:0 are 0 */

    /* Virtual address of pdpte */
    pdpte = *((unsigned long *)(__va(_pdpte)));

	/* 
	 * Check present bit and ps flag
	 * If pdpte is not present or its ps is set to 1, then fill it following
	 * entry points to 0 and copy all values back to user space 
	 */
    if ((pdpte & 0x0000000000000001u) == 0x0000000000000000u || (pdpte & 0x0000000000000080u) == 0x0000000000000080u) {

    	printk("pdpte: not present or ps is 1, stop\n");
    	pde = 0;
        pte = 0;
    	goto COPY_ALL;
    }

	/* pde: fetch 29:21 of the linear address */
    dir = vaddr & /* 29:21 */0x000000003fe00000u;

    /* Physical address of pde */
    _pde = (unsigned long *)((/* 11:3 -> 29:21 */dir >> 18) 
    		| (pdpte & 0x000ffffffffff000u/* 51:12 of pdpte */)); /* 2:0 are 0 */
    
    /* Virtual address of pde */
    pde = *((unsigned long *)(__va(_pde)));

    /* 
	 * Check present bit and ps flag
	 * If pde is not present or its ps is set to 1, then fill it following
	 * entry points to 0 and copy all values back to user space 
	 */
    if ((pde & 0x0000000000000001u) == 0x0000000000000000u || (pde & 0x0000000000000080u) == 0x0000000000000080u) {

        printk("pde: not present or ps is 1, stop\n");
        pte = 0;
    	goto COPY_ALL;

    }

    /* pte: fetch 20:12 of the linear address */
    table = vaddr & /* 20:12 */0x00000000001ff000u;

    /* Physical address of pte */
    _pte = (unsigned long *)((/* 20:12 -> 11:3 */table >> 9) 
    		| (pde & /* 51:12 of pde */0x000ffffffffff000u));

    /* Virtual address of pte */
    pte = *((unsigned long *)(__va(_pte)));

    /* 
	 * Check present bit
	 * If pte is not present, copy all values back to user space 
	 */
    if ((pte & 0x0000000000000001u) == 0x0000000000000000u) {
        printk("pte: not present, stop\n");
        goto COPY_ALL;
    }

    /* Copy all values to user space, return -1 if copy fails */
    COPY_ALL: ;
    if(
    	(copy_to_user(pml4ep, &pml4e, sizeof(unsigned long)) + 
    	copy_to_user(pdptep, &pdpte, sizeof(unsigned long)) +
    	copy_to_user(pdep, &pde, sizeof(unsigned long))+
    	copy_to_user(ptep, &pte, sizeof(unsigned long))
    	) > 0) {
    	printk("error copy_to_user\n");
      	return -1;
    }

    /* 
     * Calculate the final physical address
     * Print the physical address for each entry and the final 
     * physical address
     *
     * Could be turn on/off by changing the switch bit
     */
    #if 1
    _final = (unsigned long*)((0x0000000000000fffu/* 11:0 */ & vaddr) | (pte & /* 51:12 of pte */0x000ffffffffff000u));
    printk("pml4e physical addr: %p\n", _pml4e);
    printk("pdpte physical addr: %p\n", _pdpte);
    printk("pde physical addr: %p\n", _pde);
    printk("pte physical addr: %p\n", _pte);
    printk("final physical addr: %p\n\n", _final);
    #endif
    
	return 0;
}



/* 
 * Write MMU:
 * Modify the page table entries that map vaddr to the values specified by 
 * pml4e, pdpte, pde, and pte.
 */
SYSCALL_DEFINE5(writeMMU, unsigned long , vaddr, unsigned long , upml4e, unsigned long , updpte, unsigned long , upde, unsigned long , upte) {
	
	/* Variables */
	uintptr_t cr3;
	unsigned long pml4e, pdpte, pde, pte;
	unsigned long pml4, dirp, dir, table;
	unsigned long *_pml4e, *_pdpte, *_pde, *_pte;

	/* pml4: fetch 47:39 of the linear address */
	pml4 = (unsigned long)(vaddr & /* 47:39 */0x0000ff8000000000u);

	/* cr3: fetch 51:12 of cr3 */
	cr3 = fetch_cr3();/* 51:12 */

	/* Physical address of pml4e */
    _pml4e = (unsigned long *)((pml4 >> 36/* 47:39 -> 11:3 */) 
    			| cr3); /* 2:0 are 0 */

    /* Virtual address of pml4e */
    pml4e = *((unsigned long *)(__va(_pml4e)));

	/* Check pml4e present bit, if not present return with error */
    if ((pml4e & 0x0000000000000001u) == 0x0000000000000000u) {
        printk("Unable to write MMU: pml4e not present\n");
        return -1;
    }

    /* Write the virtual address */
    *((unsigned long *)(__va(_pml4e))) = upml4e;

    /* pdpte: fetch 38:30 of the linear address */
    dirp = vaddr & /* 38:30 */0x0000007fc0000000u;

    /* Physical address of pdpte */
    _pdpte = (unsigned long *)((dirp >> 27/* 38:30 -> 11:3 */) 
    			| (pml4e & /* 51:12 of pml4e */0x000ffffffffff000u)); /* 2:0 are 0 */

    /* Virtual address of pdpte */
    pdpte = *((unsigned long *)(__va(_pdpte)));

	/* Check pdpte present bit, if not present return with error */
    if ((pdpte & 0x0000000000000001u) == 0x0000000000000000u) {
        printk("Unable to write MMU: pdpte not present\n");
        return -1;
    }

    /* Write the virtual address */
    *((unsigned long *)(__va(_pdpte))) = updpte;
    flush_tlb_all();
    
    /* Check pdpte ps flag, if ps is set to 1 then return directly */
    if ((pdpte & 0x0000000000000080u) == 0x0000000000000080u) {
    	return 0;
    }

	/* pde: fetch 29:21 of the linear address */
    dir = vaddr & /* 29:21 */0x000000003fe00000u;

    /* Physical address of pde */
    _pde = (unsigned long *)((/* 11:3 -> 29:21 */dir >> 18) 
    		| (pdpte & 0x000ffffffffff000u/* 51:12 of pdpte */)); /* 2:0 are 0 */

    /* Virtual address of pde */
    pde = *((unsigned long *)(__va(_pde)));

    /* Sanity check */
    if ((pde & 0x0000000000000001u) == 0x0000000000000000u){
		printk("Writing MMU error: pde not present\n");
		return -1;
    }

    /* Write the virtual address */
    *((unsigned long *)(__va(_pde))) = upde;
    flush_tlb_all();

    /* Check pde ps flag, if ps is set to 1 then return directly */
    if ((pde & 0x0000000000000080u) == 0x0000000000000080u) {
    	return 0;
    }

    /* pte: fetch 20:12 of the linear address */
    table = vaddr & /* 20:12 */0x00000000001ff000u;

    /* Physical address of pte */
    _pte = (unsigned long *)((/* 20:12 -> 11:3 */table >> 9) 
    		| (pde & /* 51:12 of pde */0x000ffffffffff000u));

    /* Virtual address of pte */
    pte = *((unsigned long *)(__va(_pte)));

    /* Sanity check */
    if ((pte & 0x0000000000000001u) == 0x0000000000000000u) {
        printk("Writing MMU error: pte not present\n");
        return -1;
    }

    /* Write to virtual address */
    *((unsigned long *)(__va(_pte))) = upte;
    flush_tlb_all();

	return 0;
}
