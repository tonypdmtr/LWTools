# define anything system specific here
#
# set these variables if needed
# PROGSUFFIX: suffix added to binaries
# BUILDTPREFIX: prefix added to build utilities (cc, etc.) for xcompile
# can also set them when invoking "make"
#PROGSUFFIX := .exe
#BUILDTPREFIX=i586-mingw32msvc-

# C compiler
CC := $(BUILDTPREFIX)cc

# ar
AR := $(BUILDTPREFIX)ar

# ranlib
RANLIB := $(BUILDTPREFIX)ranlib

CPPFLAGS += -I lwlib -DPACKAGE_STRING='"lwtools 4.4+"'
LDFLAGS += -L$(PWD)/lwlib -llw

CFLAGS ?= -g -Wall

MAIN_TARGETS := lwasm/lwasm$(PROGSUFFIX) \
	lwlink/lwlink$(PROGSUFFIX) \
	lwar/lwar$(PROGSUFFIX) \
	lwlink/lwobjdump$(PROGSUFFIX)

.PHONY: all
all: $(MAIN_TARGETS)

subdirs := lwasm lwlink lwar lwlib lwbasic docs

-include $(subdirs:=/rules.make)

lwcc_srcs :=  lwcc.c

lwcc_srcs := $(addprefix lwcc/,$(lwcc_srcs))


lwasm_objs := $(lwasm_srcs:.c=.o)
lwlink_objs := $(lwlink_srcs:.c=.o)
lwar_objs := $(lwar_srcs:.c=.o)
lwlib_objs := $(lwlib_srcs:.c=.o)
lwobjdump_objs := $(lwobjdump_srcs:.c=.o)
lwcc_objs := $(lwcc_srcs:.c=.o)

lwasm_deps := $(lwasm_srcs:.c=.d)
lwlink_deps := $(lwlink_srcs:.c=.d)
lwar_deps := $(lwar_srcs:.c=.d)
lwlib_deps := $(lwlib_srcs:.c=.d)
lwobjdump_deps := $(lwobjdump_srcs:.c=.d)
lwcc_deps := $(lwcc_srcs:.c=.d)

.PHONY: lwlink lwasm lwar lwobjdump lwcc
lwlink: lwlink/lwlink$(PROGSUFFIX)
lwasm: lwasm/lwasm$(PROGSUFFIX)
lwar: lwar/lwar$(PROGSUFFIX)
lwobjdump: lwlink/lwobjdump$(PROGSUFFIX)
lwcc: lwcc/lwcc$(PROGSUFFIX)

.PHONY: lwbasic
lwbasic: lwbasic/lwbasic$(PROGSUFFIX)

lwasm/lwasm$(PROGSUFFIX): $(lwasm_objs) lwlib lwasm/rules.make
	@echo Linking $@
	@$(CC) -o $@ $(lwasm_objs) $(LDFLAGS)

lwlink/lwlink$(PROGSUFFIX): $(lwlink_objs) lwlib lwlink/rules.make
	@echo Linking $@
	@$(CC) -o $@ $(lwlink_objs) $(LDFLAGS)

lwlink/lwobjdump$(PROGSUFFIX): $(lwobjdump_objs) lwlib lwlink/rules.make
	@echo Linking $@
	@$(CC) -o $@ $(lwobjdump_objs) $(LDFLAGS)

lwar/lwar$(PROGSUFFIX): $(lwar_objs) lwlib lwar/rules.make
	@echo Linking $@
	@$(CC) -o $@ $(lwar_objs) $(LDFLAGS)

lwcc/lwcc$(PROGSUFFIX): $(lwcc_objs) lwlib
	@echo Linking $@
	@$(CC) -o $@ $(lwcc_objs) $(LDFLAGS)

#.PHONY: lwlib
.INTERMEDIATE: lwlib
lwlib: lwlib/liblw.a

lwlib/liblw.a: $(lwlib_objs) lwlib/rules.make
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
	@rm -f lwlib/liblw.a lwasm/lwasm$(PROGSUFFIX) lwlink/lwlink$(PROGSUFFIX) lwlink/lwobjdump$(PROGSUFFIX) lwar/lwar$(PROGSUFFIX) lwcc/lwcc$(PROGSUFFIX)
	@rm -f $(lwasm_objs) $(lwlink_objs) $(lwar_objs) $(lwlib_objs) $(lwobjdump_objs) $(lwcc_objs)
	@rm -f $(extra_clean)
	@rm -f */*.exe

.PHONY: realclean
realclean: clean $(realcleantargs)
	@echo "Cleaning up even more"
	@rm -f $(lwasm_deps) $(lwlink_deps) $(lwar_deps) $(lwlib_deps) $(lwobjdump_deps)
	@rm -f docs/manual/*.html docs/manual/*.pdf

print-%:
	@echo $* = $($*)

.PHONY: install
install:
	cp $(MAIN_TARGETS) /usr/local/bin/

.PHONY: test
test: all test/runtests
	@test/runtests
	