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


cpp/

This is the actual C preprocessor. Its specific interface is deliberately
undocumented. Do not call it directly. Ever. Just don't. Bad Things(tm) will
happen if you do.


liblwcc/

This contains any runtime libraries the compiler needs to support its
output. This is usually assembly routines to support complex operations not
directly supported by the CPU instruction set.


RUNTIME INFORMATION
===================

The compiler driver has a built in base directory where it searches for its
various components as needed. In the discussion below, BASEDIR stands for
that directory.

BASEDIR may be specified by the -B option to the driver. Care must be taken
when doing so, however, because specifying an invalid -B will cause the
compiler to fail completely. It will completely override the built in search
paths for the compiler provided files and programs.

Because BASEDIR is part of the actual compiler, it is not affected by
--sysroot or -isysroot options.

If BASEDIR does not exist, compiler component programs will be searched for
in the standard execution paths. This may lead to incorrect results so it is
important to make certain that the specified BASEDIR exists.

If -B is not specified, the default BASEDIR is
$(PREFIX)/lib/lwcc/$(VERSION)/ where PREFIX is the build prefix from the
Makefile and VERSION is the lwtools version.

The contents of BASEDIR are as follows:

BASEDIR/bin

Various binaries for the parts of the compiler system. Notably, this
includes the preprocessor and compiler proper. The specific names and
contents of this directory cannot be relied upon and these programs should
not be called directly. Ever. Don't do it.


BASEDIR/lib

This directory contains various libraries that provide support for any
portion of the compiler's output. The driver will arrange to pass the
appropriate arguments to the linker to include these as required.

The most notable file in this directory is liblwcc.a wich contains the
support routines for the compiler's code generation. Depending on ABI and
code generation options supported, there may be multiple versions of
liblwcc.a. The driver will arrange for the correct one to be referenced.


BASEDIR/include

This directory contains any C header files that the compiler provides.
Notably, this includes stdint.h, stdarg.h, and setjmp.h as these are
specific to the compiler. The driver will arrange for this directory to be
searched prior to the standard system directories so that these files will
override any present in those directories.

