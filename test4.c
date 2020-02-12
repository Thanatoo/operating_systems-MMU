#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>

/*
 * This program create a program that allocates a 4 KB memory buffer. 
 * Then use readMMU() to read the page table entry for the buffer. 
 * After that, create a child process with fork() and have both the parent and 
 * child process report the result of using readMMU() to read the page 
 * table entries of the allocated buffer.
 */
int main (int argc, char *argv[]) {

    /* Variables */
	unsigned long pml4e = 0;
    unsigned long pdpte = 0;
    unsigned long pde = 0;
    unsigned long pte = 0;

    unsigned long *buffer = malloc(4096);


    /* Fork a child */
    if (fork() == 0) {
    	if (syscall(181, buffer, &pml4e, &pdpte, &pde, &pte) != 0) {
        	printf("error readMMU()\n");
        	return -1;
    	}
    	printf("Child:\n");
    	printf("pml4e: 0x%016lx\n", pml4e);
		printf("pdpte: 0x%016lx\n", pdpte);
		printf("pde: 0x%016lx\n", pde);
		printf("pte: 0x%016lx\n\n", pte);
    }
    /* Parent */
    else {
    	if (syscall(181, buffer, &pml4e, &pdpte, &pde, &pte) != 0) {
        	printf("error readMMU()\n");
        	return -1;
    	}
    	printf("Parent:\n");
    	printf("pml4e: 0x%016lx\n", pml4e);
		printf("pdpte: 0x%016lx\n", pdpte);
		printf("pde: 0x%016lx\n", pde);
		printf("pte: 0x%016lx\n\n", pte);
    }

    return 0;

}