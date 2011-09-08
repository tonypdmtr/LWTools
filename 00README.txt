This is LWTOOLS, a cross development system targetting the 6809 CPU.

It consists of an assembler, lwasm, a linker, lwlink, and an archiver,
lwar which should compile on any reasonably modern POSIX environment. If you
have problems building, make sure you are using GNU make. Other make
programs may work but GNU make is known to work.

To see if a quick build will work, just type "make". If it works, you're
ready to go ahead with "make install". This will install in /usr/local/bin.

If you feel adventurous, you can also run the test suite by running "make
test". However, be warned that it is likely not going to work unless you are
running on a fairly standard unix system with perl in /usr/bin/perl.

See docs/ for additional information.
