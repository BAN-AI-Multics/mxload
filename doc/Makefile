############################################################################
ROFF   ?= groff
#ROFF  ?= nroff
RM     ?= rm
PS2PDF ?= ps2pdf
ENZ    ?= test -s
############################################################################
.PHONY: \
	all
all: \
	manual.pdf pages.pdf
############################################################################
.PHONY: \
	clean
clean:
	-$(RM) \
		manual.ps pages.ps
	-$(RM) \
		manual.pdf pages.pdf
############################################################################
manual.ps: \
	manual.tms
	$(ROFF) -t -ms -Tps \
		manual.tms \
		> manual.ps || $(RM) manual.ps
	@$(ENZ) manual.ps
############################################################################
pages.ps: \
	mxarc.1 mxascii.1 mxforum.1 mxmap.1 mxmbx.1
	$(ROFF) -t -man -Tps \
		mxarc.1 mxascii.1 mxforum.1 mxload.1 mxmap.1 mxmbx.1 \
		> pages.ps || $(RM) pages.ps
	@$(ENZ) pages.ps
############################################################################
pages.pdf: \
	pages.ps
	$(PS2PDF) \
		pages.ps pages.pdf || $(RM) pages.pdf
	@$(ENZ) pages.pdf
############################################################################
manual.pdf: \
	manual.ps
	$(PS2PDF) \
		manual.ps manual.pdf || $(RM) manual.pdf
	@$(ENZ) manual.pdf
############################################################################
