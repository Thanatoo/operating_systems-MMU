#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <fcntl.h>


/*
 * This program uses mmap() to map these two files into two different virtual addresses; 
 * call these virtual addresses A and B. Then use readMMU() and writeMMU() to map the 
 * physical page mapped to virtual address A to virtual address B.
 */
int main(int argc, char *argv[]) {

    /* Variables */
    int fd1, fd2;
    unsigned long *vaddr1, *vaddr2;
    struct stat sb1, sb2;
    unsigned long pml4e1 = 0;
    unsigned long pdpte1 = 0;
    unsigned long pde1 = 0;
    unsigned long pte1 = 0;
    unsigned long pml4e2 = 0;
    unsigned long pdpte2 = 0;
    unsigned long pde2 = 0;
    unsigned long pte2 = 0;

    /* Input Check */
    if (argc != 3) {
        printf("Usage: test3 <input file 1> <input file 2>\n");
        return -1;
    }

    /* Initialize the fd1 */
    if ((fd1 = open(argv[1], O_RDWR)) < 0) {
        printf("Cannot open %s for reading\n", argv[1]);
        return -1;
    }

    /* Initialize the stat1 */
    if (fstat(fd1, &sb1) < 0) {
        printf("fstat error\n");
        return -1;
    }

    /* Mmap the file1 */
    if ((vaddr1 = mmap(NULL, sb1.st_size, PROT_READ |
                    PROT_WRITE, MAP_SHARED, fd1, 0)) == MAP_FAILED) {
        printf("mmap error\n");
        return -1;
    }


    /* Initialize the fd1 */
    if ((fd2 = open(argv[2], O_RDWR)) < 0) {
        printf("Cannot open %s for reading\n", argv[2]);
        return -1;
    }

    /* Initialize the fd1 */
    if (fstat(fd2, &sb2) < 0) {
        printf("fstat error\n");
        return -1;
    } 

    /* Initialize the stat1 */
    if ((vaddr2 = mmap(NULL, sb2.st_size, PROT_READ |
                    PROT_WRITE, MAP_SHARED, fd2, 0)) == MAP_FAILED) {
        printf("mmap error\n");
        return -1;
    }

    /* access */
    unsigned long access1 = *vaddr1;
    unsigned long access2 = *vaddr2;


    /* Calls the readMMU() on file1 */
    if (syscall(181, vaddr1, &pml4e1, &pdpte1, &pde1, &pte1) != 0) {
        printf("error readMMU()\n");
        return -1;
    }
    /* Print the value */
    printf("vaddr1: 0x%016lx\n", (unsigned long)*vaddr1);
    printf("pml4e1: 0x%016lx\n", pml4e1);
    printf("pdpte1: 0x%016lx\n", pdpte1);
    printf("pde1: 0x%016lx\n", pde1);
    printf("pte1: 0x%016lx\n", pte1);
       
    /* Calls the readMMU() on file2 */
    if (syscall(181, vaddr2, &pml4e2, &pdpte2, &pde2, &pte2) != 0) {
        printf("error readMMU()\n");
        return -1;
    }
    /* Print the value */
    printf("vaddr2: 0x%016lx\n", (unsigned long)*vaddr2);
    printf("pml4e2: 0x%016lx\n", pml4e2);
    printf("pdpte2: 0x%016lx\n", pdpte2);
    printf("pde2: 0x%016lx\n", pde2);
    printf("pte2: 0x%016lx\n", pte2);
    
    
    /* Calls the writeMMU() on file2, write file2 to file1 */
    if (syscall(182, vaddr2, pml4e1, pdpte1, pde1, pte1) != 0) {
        printf("error write pte\n");
        return -1;
    }
    printf("pte of file 2 has been changed to pte of file 1.\n");
    
    
    /* Modify file1's vaddr */
    *vaddr1 = 1000;
    printf("vaddr1 is now changed to 0x%016lx\n", (unsigned long)*vaddr1);
    printf("vaddr2 is now 0x%016lx\n", (unsigned long)*vaddr2);

    /* Restore file2 */
    if (syscall(182, vaddr2, pml4e2, pdpte2, pde2, pte2) != 0) {
        printf("error write pte\n");
        return -1;
    }
    printf("vaddr2 is restored to 0x%016lx\n", (unsigned long)*vaddr2);

    return 0;
}

