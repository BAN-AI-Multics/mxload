############################################################################
MAKE ?= make
############################################################################
SUBS  = src doc
############################################################################
.NOTPARALLEL: all clean test src doc
############################################################################
.PHONY: all src doc test
all: $(SUBS)
	+$(MAKE)  -C src $@
	-+$(MAKE) -C doc $@
############################################################################
.PHONY: clean src doc test
clean: $(SUBS)
	-+$(MAKE) -C src $@
	-+$(MAKE) -C doc $@
############################################################################
