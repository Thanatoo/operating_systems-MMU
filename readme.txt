******      Files Modified or Added to The Kernel Source Code    ******
> Modified "linux-5.3.2/kernel/csc256.c"
> Modified "linux-5.3.2/arch/x86/entry/syscalls/syscall_64.tbl"

---- added readMMU, writeMMU, allocPage and freePage system calls.
---- in my test programs I did not use allocPage or freePage.
---- test programs are test1.c, test2.c, test3.c and test4.c and should be placed under directory ./floppy.

***************             Test Programs             *****************
> See "answers.pdf"