dirname := $(dir $(lastword $(MAKEFILE_LIST)))
lwbasic_dir := $(dirname)

lwbasic_lsrcs := main.c input.c parser.c lexer.c emit.c symtab.c

lwbasic_srcs := $(addprefix $(dirname),$(lwbasic_lsrcs))
lwbasic_objs := $(lwbasic_srcs:.c=.o)
lwbasic_deps := $(lwbasic_srcs:.c=.d)



$(lwbasic_dir)lwbasic$(PROGSUFFIX): $(lwbasic_objs) lwlib $(lwbasic_dir)rules.make
	@echo "Linking $@"
	@$(CC) -o $@ $(lwbasic_objs) $(LDFLAGS)

cleantargs := $(cleantargs) lwbasicclean
realcleantargs := $(realcleantargs) lwbasicrealclean

.PHONY: lwbasicclean lwbasicrealclean
lwbasicrealclean:
	@echo "Really cleaning up lwbasic"
	@cd $(lwbasic_dir) && rm -f *.d

lwbasicclean:
	@echo "Cleaning up lwbasic"
	@cd $(lwbasic_dir) && rm -f *.o *.exe lwbasic

-include $(lwbasic_deps)
