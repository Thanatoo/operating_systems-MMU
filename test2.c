#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <fcntl.h>


/*
 * This program uses mmap() to map a 8kb file into the mem and
 * uses readMMU() to read its entires. After that the program tries
 * access the file and uses readMMU() on it again to see the output.
 */
int main (int argc, char *argv[]) {
	/* Variables */
	int fd;
	unsigned long *vaddr;
	struct stat sb;
	unsigned long pml4e = 0;
    unsigned long pdpte = 0;
    unsigned long pde = 0;
    unsigned long pte = 0;

    /* Input Check */
	if (argc != 2) {
        printf("Usage: test2 <input file>\n");
        return -1;
    }

    /* Initialize the fd */
    if ((fd = open(argv[1], O_RDWR)) < 0) {
        printf("Cannot open %s for reading\n", argv[1]);
        return -1;
    }

    /* Initialize the stat */
    if (fstat(fd, &sb) < 0) {
        printf("fstat error\n");
        return -1;
    }

    /* Mmap the file */
    if ((vaddr = mmap(NULL, sb.st_size, PROT_READ |
                    PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) {
        printf("mmap error\n");
        return -1;
    }

	/* Calls the readMMU() */
    if (syscall(181, vaddr, &pml4e, &pdpte, &pde, &pte) != 0) {
        printf("error readMMU()\n");
        return -1;
    }

    /* Print the value */
    printf("vaddr: 0x%016lx\n", (unsigned long)*vaddr);
    printf("pml4e: 0x%016lx\n", pml4e);
	printf("pdpte: 0x%016lx\n", pdpte);
	printf("pde: 0x%016lx\n", pde);
	printf("pte: 0x%016lx\n\n", pte);

    /* access */
    unsigned long access = *vaddr;
    printf("after access\n");

	/* Calls the readMMU() */
    if (syscall(181, vaddr, &pml4e, &pdpte, &pde, &pte) != 0) {
        printf("error readMMU()\n");
        return -1;
    }

    /* Print the value after access */
    printf("vaddr: 0x%016lx\n", (unsigned long)*vaddr);
    printf("pml4e: 0x%016lx\n", pml4e);
	printf("pdpte: 0x%016lx\n", pdpte);
	printf("pde: 0x%016lx\n", pde);
	printf("pte: 0x%016lx\n", pte);

}