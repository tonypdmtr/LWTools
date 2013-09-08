This is the lwcc C compiler for lwtools. It was written using various other
C compilers as guides. Special thanks to the developers of the PCC compiler.
While none of the actual code from PCC was actually used, much of compiler
itself served as a template for creating lwcc.

This directory is arranged as follows:

driver/

This contains the source for the front end driver program which will be
called "lwcc" and is the public face of the compiler. The lwcc program
itself provides various options that are largely compatible with unix C
compilers like gcc. It should be noted that the internal interface between
the lwcc driver and its back end programs (the preprocessor and compiler
proper) is unspecified and subject to change without notice. The assembler
and linker (lwasm, lwlink) do have defined public interfaces are are not
likely to change substantially.


liblwcc/

This contains any runtime libraries the compiler needs to support its
output. This is usually assembly routines to support complex operations not
directly supported by the CPU instruction set.

