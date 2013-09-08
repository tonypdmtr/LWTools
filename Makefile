# define anything system specific here
#
# set these variables if needed
# PROGSUFFIX: suffix added to binaries
# BUILDTPREFIX: prefix added to build utilities (cc, etc.) for xcompile
# can also set them when invoking "make"
#PROGSUFFIX := .exe
#BUILDTPREFIX=i586-mingw32msvc-

LWTOOLS_VERSION = 4.8
LWTOOLS_VERSION_SUFFIX = -devcc

PACKAGE_VERSION = $(LWTOOLS_VERSION)$(LWTOOLS_VERSION_SUFFIX)

ifeq ($(PREFIX),)
ifneq ($(DESTDIR),)
PREFIX = /usr
else
PREFIX = /usr/local
endif
endif

LIBDIR = $(PREFIX)/lib
BINDIR = $(PREFIX)/bin

INSTALLDIR = $(DESTDIR)$(PREFIX)
INSTALLBIN = $(DESTDIR)$(BINDIR)
INSTALLLIB = $(DESTDIR)$(LIBDIR)

LWCC_LIBDIR = $(LIBDIR)/lwcc/$(PACKAGE_VERSION)
LWCC_INSTALLLIBDIR = $(DESTDIR)$(LWCC_LIBDIR)

# this are probably pointless but they will make sure
# the variables are set without overriding the environment
# or automatic values from make itself.
CC ?= cc
AR ?= ar
RANLIB ?= ranlib

# Set variables for cross compiling
ifneq ($(BUILDTPREFIX),)
CC := $(BUILDTPREFIX)$(CC)
AR := $(BUILDTPREFIX)$(AR)
RANLIB := $(BUILDTPREFIX)$(RANLIB)
endif

CPPFLAGS += -I lwlib -DPACKAGE_STRING='"lwtools $(PACKAGE_VERSION)"'
CPPFLAGS += -DPREFIX=$(PREFIX) -DLWCC_LIBDIR=$(LWCC_LIBDIR)
CPPFLAGS += -DPROGSUFFIX=$(PROGSUFFIX)
LDFLAGS += -L$(PWD)/lwlib -llw

CFLAGS ?= -O3 -Wall

MAIN_TARGETS := lwasm/lwasm$(PROGSUFFIX) \
	lwlink/lwlink$(PROGSUFFIX) \
	lwar/lwar$(PROGSUFFIX) \
	lwlink/lwobjdump$(PROGSUFFIX) \
	lwcc/driver/lwcc$(PROGSUFFIX)

LWCC_LIBBIN_FILES =
LWCC_LIBLIB_FILES =
LWCC_LIBINC_FILES =

.PHONY: all
all: $(MAIN_TARGETS)

lwar_srcs := add.c extract.c list.c lwar.c main.c remove.c replace.c
lwar_srcs := $(addprefix lwar/,$(lwar_srcs))

lwlib_srcs := lw_alloc.c lw_realloc.c lw_free.c lw_error.c lw_expr.c \
	lw_stack.c lw_string.c lw_stringlist.c lw_cmdline.c
lwlib_srcs := $(addprefix lwlib/,$(lwlib_srcs))

lwlink_srcs := main.c lwlink.c readfiles.c expr.c script.c link.c output.c map.c
lwobjdump_srcs := objdump.c
lwlink_srcs := $(addprefix lwlink/,$(lwlink_srcs))
lwobjdump_srcs := $(addprefix lwlink/,$(lwobjdump_srcs))

lwasm_srcs :=  debug.c input.c insn_bitbit.c insn_gen.c insn_indexed.c \
	insn_inh.c insn_logicmem.c insn_rel.c insn_rlist.c insn_rtor.c insn_tfm.c \
	instab.c list.c lwasm.c macro.c main.c os9.c output.c pass1.c pass2.c \
	pass3.c pass4.c pass5.c pass6.c pass7.c pragma.c pseudo.c section.c \
	struct.c symbol.c unicorns.c
lwasm_srcs := $(addprefix lwasm/,$(lwasm_srcs))

lwasm_objs := $(lwasm_srcs:.c=.o)
lwlink_objs := $(lwlink_srcs:.c=.o)
lwar_objs := $(lwar_srcs:.c=.o)
lwlib_objs := $(lwlib_srcs:.c=.o)
lwobjdump_objs := $(lwobjdump_srcs:.c=.o)

lwasm_deps := $(lwasm_srcs:.c=.d)
lwlink_deps := $(lwlink_srcs:.c=.d)
lwar_deps := $(lwar_srcs:.c=.d)
lwlib_deps := $(lwlib_srcs:.c=.d)
lwobjdump_deps := $(lwobjdump_srcs:.c=.d)

lwcc_driver_srcs := main.c
lwcc_driver_srcs := $(addprefix lwcc/driver/,$(lwcc_driver_srcs))
lwcc_driver_objs := $(lwcc_driver_srcs:.c=.o)
lwcc_driver_deps := $(lwcc_driver_srcs:.c=.d)

.PHONY: lwlink lwasm lwar lwobjdump lwcc
lwlink: lwlink/lwlink$(PROGSUFFIX)
lwasm: lwasm/lwasm$(PROGSUFFIX)
lwar: lwar/lwar$(PROGSUFFIX)
lwobjdump: lwlink/lwobjdump$(PROGSUFFIX)
lwcc: lwcc/driver/lwcc

lwasm/lwasm$(PROGSUFFIX): $(lwasm_objs) lwlib
	@echo Linking $@
	@$(CC) -o $@ $(lwasm_objs) $(LDFLAGS)

lwlink/lwlink$(PROGSUFFIX): $(lwlink_objs) lwlib
	@echo Linking $@
	@$(CC) -o $@ $(lwlink_objs) $(LDFLAGS)

lwlink/lwobjdump$(PROGSUFFIX): $(lwobjdump_objs) lwlib
	@echo Linking $@
	@$(CC) -o $@ $(lwobjdump_objs) $(LDFLAGS)

lwar/lwar$(PROGSUFFIX): $(lwar_objs) lwlib
	@echo Linking $@
	@$(CC) -o $@ $(lwar_objs) $(LDFLAGS)

lwcc/driver/lwcc$(PROGSUFFIX): $(lwcc_driver_objs) lwlib
	@echo Linking $@
	@$(CC) -o $@ $(lwcc_driver_objs) $(LDFLAGS)

#.PHONY: lwlib
.INTERMEDIATE: lwlib
lwlib: lwlib/liblw.a

lwlib/liblw.a: $(lwlib_objs)
	@echo Linking $@
	@$(AR) rc $@ $(lwlib_objs)
	@$(RANLIB) $@

alldeps := $(lwasm_deps) $(lwlink_deps) $(lwar_deps) $(lwlib_deps) ($lwobjdump_deps) $(lwcc_deps)

-include $(alldeps)

extra_clean := $(extra_clean) *~ */*~

%.o: %.c
	@echo "Building dependencies for $@"
	@$(CC) -MM $(CPPFLAGS) -o $*.d $<
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o $*.d:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp
	@echo Building $@
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<
	

.PHONY: clean
clean: $(cleantargs)
	@echo "Cleaning up"
	@rm -f lwlib/liblw.a lwasm/lwasm$(PROGSUFFIX) lwlink/lwlink$(PROGSUFFIX) lwlink/lwobjdump$(PROGSUFFIX) lwar/lwar$(PROGSUFFIX)
	@rm -f lwcc/driver/lwcc$(PROGSUFFIX)
	@rm -f $(lwcc_driver_ojbs)
	@rm -f $(lwasm_objs) $(lwlink_objs) $(lwar_objs) $(lwlib_objs) $(lwobjdump_objs)
	@rm -f $(extra_clean)
	@rm -f */*.exe

.PHONY: realclean
realclean: clean $(realcleantargs)
	@echo "Cleaning up even more"
	@rm -f $(lwasm_deps) $(lwlink_deps) $(lwar_deps) $(lwlib_deps) $(lwobjdump_deps)
	@rm -f $(lwcc_driver_deps)
	@rm -f docs/manual/*.html docs/manual/*.pdf

print-%:
	@echo $* = $($*)

.PHONY: install
install:
	install -d $(INSTALLBIN)
	install $(MAIN_TARGETS) $(INSTALLBIN)
	install -d $(LWCC_INSTALLLIBDIR)
	install -d $(LWCC_INSTALLLIBDIR)/bin
	install -d $(LWCC_INSTALLLIBDIR)/lib
	install -d $(LWCC_INSTALLLIBDIR)/include
ifneq ($(LWCC_LIBBIN_FILES),)
	install $(LWCC_LIBBIN_FILES) $(LIBCC_INSTALLLIBDIR)/bin
endif
ifneq ($(LWCC_LIBLIB_FILES),)
	install $(LWCC_LIBLIB_FILES) $(LIBCC_INSTALLLIBDIR)/lib
endif
ifneq ($(LWCC_LIBINC_FILES),)
	install $(LWCC_LIBINC_FILES) $(LIBCC_INSTALLLIBDIR)/include
endif

.PHONY: test
test: all test/runtests
	@test/runtests

.PHONY: manual
manual: manual-html manual-htmlm manual-pdf

.PHONY: manual-html
manual-html: docs/manual/manual.html

.PHONY: manual-htmlm
manual-htmlm: docs/manual/index.html

.PHONY: manual-pdf
manual-pdf: docs/manual/manual.pdf

docs/manual/manual.html: docs/manual.docbook.sgml
	docbook2html -o docs -u docs/manual.docbook.sgml && mv docs/manual.docbook.html docs/manual/manual.html

docs/manual/index.html: docs/manual.docbook.sgml
	docbook2html -o docs/manual docsmanual.docbook.sgml

docs/manual/manual.pdf: docs/manual.docbook.sgml
	docbook2pdf -o docs -u docsmanual.docbook.sgml && mv docs/manual.docbook.pdf docs/manual/manual.pdf && rm -f docs/manual.docbook.html

