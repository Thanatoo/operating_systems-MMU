#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/syscall.h>

/*
 * This program calls the system call readMMU() and writeMMU()
 * on the virtual address 0, after that the system prints the 
 * table entries.
 */
int main(int argc, char **argv)
{
    /* Variables */
	unsigned long pml4e = 0;
	unsigned long pdpte = 0;
	unsigned long pde = 0;
	unsigned long pte = 0;

	/* Calls the readMMU() */
    if (syscall(181, 0, &pml4e, &pdpte, &pde, &pte) != 0) {
        printf("error readMMU()\n");
    }

    /* Calls the writeMMU() */
    if (syscall(182, 0, pml4e, pdpte, pde, pte) != 0) {
    	printf("error writeMMU()\n");
    }

    /* Print the value */
    printf("pml4e: 0x%016lx\n", pml4e);
	printf("pdpte: 0x%016lx\n", pdpte);
	printf("pde: 0x%016lx\n", pde);
	printf("pte: 0x%016lx\n", pte);
	return 0;
}

