##############################################################################
CC           ?= gcc
##############################################################################
ISO_ANSI_CX   = -DANSI_FUNC=1
##############################################################################
MORE_CFLAGS   = -Os
MORE_CFLAGS  += -Wall
MORE_CFLAGS  += -Wno-unused-label
MORE_CFLAGS  += -Wno-unused-function
MORE_CFLAGS  += -Wno-unused-variable
MORE_CFLAGS  += -Wno-unused-but-set-variable
MORE_CFLAGS  += -DHAVE_MKSTEMP=1
#MORE_CFLAGS += -DWORDALIGN=1
##############################################################################
RM           ?= rm -f
MV           ?= mv -f
MAKE         ?= make
EXISTS       ?= test -f
##############################################################################
MX_CFLAGS     = $(ISO_ANSI_CX)
MX_CFLAGS    += $(MORE_CFLAGS)
##############################################################################
CFLAGS       += $(MX_CFLAGS)
##############################################################################
.PHONY: all
.NOTPARALLEL: all clean mxload mxascii mxarc mxmap mxmbx mxforum pr8bit
all: mxload mxascii mxarc mxmap mxmbx mxforum pr8bit
##############################################################################
mxload: copybits.o cvpath.o getopt.o gettype.o mapprint.o multtape.o         \
        mxbitio.o mxdearc.o mxmseg.o optptr.o parsctl.o preamble.o           \
        rdbkrcd.o tempfile.o timestr.o
	@$(EXISTS) mxload || $(RM) mxload.o
	@$(EXISTS) mxload || $(MAKE) mxload.bin \
		"CFLAGS += -DMXLOAD $(MX_CFLAGS)"
	@$(EXISTS) mxload || $(MV) mxload.bin mxload
##############################################################################
mxascii: copybits.o cvpath.o getopt.o gettype.o mapprint.o multtape.o        \
         mxbitio.o mxdearc.o mxmseg.o optptr.o parsctl.o preamble.o          \
         rdbkrcd.o tempfile.o timestr.o
	@$(EXISTS) mxascii || $(RM) mxload.o mxascii.o
	@$(EXISTS) mxascii || $(MAKE) mxascii.bin \
		"CFLAGS += -DMXASCII $(MX_CFLAGS)"
	@$(EXISTS) mxascii || $(MV) mxascii.bin mxascii
##############################################################################
mxarc: copybits.o cvpath.o getopt.o gettype.o mapprint.o multtape.o          \
       mxbitio.o mxdearc.o mxmseg.o optptr.o parsctl.o preamble.o            \
       rdbkrcd.o tempfile.o timestr.o
	@$(EXISTS) mxarc || $(RM) mxload.o mxarc.o
	@$(EXISTS) mxarc || $(MAKE) mxarc.bin \
		"CFLAGS += -DMXARC $(MX_CFLAGS)"
	@$(EXISTS) mxarc || $(MV) mxarc.bin mxarc
##############################################################################
mxmap: copybits.o cvpath.o getopt.o gettype.o mapprint.o multtape.o          \
       mxbitio.o mxdearc.o mxload.o mxmap.o mxmseg.o optptr.o parsctl.o      \
       preamble.o rdbkrcd.o tempfile.o timestr.o
	@$(EXISTS) mxmap || $(RM) mxload.o mxmap.o
	@$(EXISTS) mxmap || $(MAKE) mxmap.bin \
		"CFLAGS += -DMXMAP $(MX_CFLAGS)"
	@$(EXISTS) mxmap || $(MV) mxmap.bin mxmap
##############################################################################
mxmbx: copybits.o cvpath.o getopt.o gettype.o mapprint.o multtape.o          \
       mxbitio.o mxdearc.o mxload.o mxmbx.o mxmseg.o optptr.o parsctl.o      \
       preamble.o rdbkrcd.o tempfile.o timestr.o
	@$(EXISTS) mxmbx || $(RM) mxload.o mxmbx.o
	@$(EXISTS) mxmbx || $(MAKE) mxmbx.bin \
		"CFLAGS += -DMXMBX $(MX_CFLAGS)"
	@$(EXISTS) mxmbx || $(MV) mxmbx.bin mxmbx
##############################################################################
mxforum: copybits.o cvpath.o getopt.o gettype.o mapprint.o multtape.o        \
         mxbitio.o mxdearc.o mxforum.o mxload.o mxmseg.o optptr.o parsctl.o  \
         preamble.o rdbkrcd.o tempfile.o timestr.o
	@$(EXISTS) mxforum || $(RM) mxload.o mxforum.o
	@$(EXISTS) mxforum || $(MAKE) mxforum.bin \
		"CFLAGS += -DMXFORUM $(MX_CFLAGS)"
	@$(EXISTS) mxforum || $(MV) mxforum.bin mxforum
##############################################################################
pr8bit: pr8bit.o mxbitio.o multtape.o
	@$(EXISTS) pr8bit || $(MAKE) pr8bit.bin \
		"CFLAGS += -DPR8BIT $(MX_CFLAGS)"
	@$(EXISTS) pr8bit || $(MV) pr8bit.bin pr8bit
##############################################################################
.PHONY: clean
clean:
	-$(RM) \
		mxarc mxascii mxload mxforum mxmap mxmbx pr8bit
	-$(RM) \
		mxarc.bin mxascii.bin mxload.bin mxforum.bin mxmap.bin        \
		mxmbx.bin pr8bit.bin
	-$(RM) \
		copybits.o cvpath.o getopt.o gettype.o mapprint.o multtape.o  \
		mxarc.o mxascii.o mxbitio.o mxdearc.o mxforum.o mxload.o      \
		mxmap.o mxmbx.o mxmseg.o optptr.o parsctl.o preamble.o        \
		rdbkrcd.o tempfile.o timestr.o pr8bit.o
##############################################################################
.PHONY: mxload.bin
mxload.bin: copybits.o cvpath.o getopt.o gettype.o mapprint.o multtape.o     \
            mxbitio.o mxdearc.o mxload.o mxmseg.o optptr.o parsctl.o         \
            preamble.o rdbkrcd.o tempfile.o timestr.o
	$(EXISTS) mxload || $(CC) $(CFLAGS) $(MX_CFLAGS) -o mxload.bin \
		copybits.o cvpath.o getopt.o gettype.o mapprint.o            \
		multtape.o mxbitio.o mxdearc.o mxload.o mxmseg.o optptr.o    \
		parsctl.o preamble.o rdbkrcd.o tempfile.o timestr.o
##############################################################################
.PHONY: mxascii.bin
mxascii.bin: copybits.o cvpath.o getopt.o gettype.o mapprint.o multtape.o    \
             mxascii.o mxbitio.o mxdearc.o mxload.o mxmseg.o optptr.o        \
             parsctl.o preamble.o rdbkrcd.o tempfile.o timestr.o
	$(EXISTS) mxascii || $(CC) $(CFLAGS) $(MX_CFLAGS) -o mxascii.bin \
		copybits.o cvpath.o getopt.o gettype.o mapprint.o            \
		multtape.o mxascii.o mxbitio.o mxdearc.o mxload.o mxmseg.o   \
		optptr.o parsctl.o preamble.o rdbkrcd.o tempfile.o timestr.o
##############################################################################
.PHONY: mxarc.bin
mxarc.bin: copybits.o cvpath.o getopt.o gettype.o mapprint.o multtape.o      \
           mxarc.o mxbitio.o mxdearc.o mxload.o mxmseg.o optptr.o parsctl.o  \
           preamble.o rdbkrcd.o tempfile.o timestr.o
	$(EXISTS) mxarc || $(CC) $(CFLAGS) $(MX_CFLAGS) -o mxarc.bin \
		copybits.o cvpath.o getopt.o gettype.o mapprint.o            \
		multtape.o mxarc.o mxbitio.o mxdearc.o mxload.o mxmseg.o     \
		optptr.o parsctl.o preamble.o rdbkrcd.o tempfile.o timestr.o
##############################################################################
.PHONY: mxmap.bin
mxmap.bin: copybits.o cvpath.o getopt.o gettype.o mapprint.o multtape.o      \
           mxbitio.o mxdearc.o mxload.o mxmap.o mxmseg.o optptr.o parsctl.o  \
           preamble.o rdbkrcd.o tempfile.o timestr.o
	$(EXISTS) mxmap || $(CC) $(CFLAGS) $(MX_CFLAGS) -o mxmap.bin \
		copybits.o cvpath.o getopt.o gettype.o mapprint.o            \
		multtape.o mxbitio.o mxdearc.o mxload.o mxmap.o mxmseg.o     \
		optptr.o parsctl.o preamble.o rdbkrcd.o tempfile.o timestr.o
##############################################################################
.PHONY: mxmbx.bin
mxmbx.bin: copybits.o cvpath.o getopt.o gettype.o mapprint.o multtape.o      \
           mxbitio.o mxdearc.o mxload.o mxmbx.o mxmseg.o optptr.o parsctl.o  \
           preamble.o rdbkrcd.o tempfile.o timestr.o
	$(EXISTS) mxmbx || $(CC) $(CFLAGS) $(MX_CFLAGS) -o mxmbx.bin \
		copybits.o cvpath.o getopt.o gettype.o mapprint.o            \
		multtape.o mxbitio.o mxdearc.o mxload.o mxmbx.o mxmseg.o     \
		optptr.o parsctl.o preamble.o rdbkrcd.o tempfile.o timestr.o
##############################################################################
.PHONY: mxforum.bin
mxforum.bin: copybits.o cvpath.o getopt.o gettype.o mapprint.o multtape.o    \
             mxbitio.o mxdearc.o mxforum.o mxload.o mxmseg.o optptr.o        \
             parsctl.o preamble.o rdbkrcd.o tempfile.o timestr.o
	$(EXISTS) mxforum || $(CC) $(CFLAGS) $(MX_CFLAGS) -o mxforum.bin \
		copybits.o cvpath.o getopt.o gettype.o mapprint.o multtape.o \
		mxbitio.o mxdearc.o mxforum.o mxload.o mxmseg.o optptr.o     \
		parsctl.o preamble.o rdbkrcd.o tempfile.o timestr.o
##############################################################################
.PHONY: pr8bit.bin
pr8bit.bin: pr8bit.o mxbitio.o multtape.o
	$(EXISTS) pr8bit || $(CC) $(CFLAGS) $(MX_CFLAGS) \
		-o pr8bit.bin pr8bit.o mxbitio.o multtape.o
##############################################################################
copybits.o: copybits.c copybits.h mxbitio.h mxload.h tempfile.h
##############################################################################
cvpath.o: cvpath.c cvpath.h dirsep.h mxload.h mxlopts.h
##############################################################################
getopt.o: getopt.c
##############################################################################
gettype.o: gettype.c gettype.h mxbitio.h mxload.h mxlopts.h preamble.h
##############################################################################
mapprint.o: mapprint.c aclmodes.h mapprint.h mxlargs.h mxload.h preamble.h   \
            timestr.h
##############################################################################
multtape.o: multtape.c multtape.h mxbitio.h mxload.h
##############################################################################
mxarc.o: mxarc.c aclmodes.h copybits.h cvpath.h dirsep.h getopt.h gettype.h  \
         mapprint.h mxbitio.h mxdearc.h mxlargs.h mxload.h mxlopts.h         \
         mxmseg.h optptr.h parsctl.h preamble.h rdbkrcd.h tempfile.h utime.h
##############################################################################
mxascii.o: mxascii.c copybits.h mxbitio.h mxload.h
##############################################################################
mxbitio.o: mxbitio.c multtape.h mxbitio.h mxload.h
##############################################################################
mxdearc.o: mxdearc.c copybits.h dirsep.h mxbitio.h mxdearc.h mxload.h        \
           mxlopts.h optptr.h preamble.h tempfile.h timestr.h
##############################################################################
mxforum.o: mxforum.c aclmodes.h copybits.h cvpath.h dirsep.h getopt.h        \
           gettype.h mapprint.h mxbitio.h mxlargs.h mxload.h mxlopts.h       \
           optptr.h parsctl.h preamble.h rdbkrcd.h tempfile.h timestr.h      \
           utime.h
##############################################################################
mxload.o: mxload.c aclmodes.h copybits.h cvpath.h dirsep.h getopt.h          \
          gettype.h mapprint.h mxbitio.h mxdearc.h mxlargs.h mxload.h        \
          mxlopts.h mxmseg.h optptr.h parsctl.h preamble.h rdbkrcd.h         \
          tempfile.h utime.h
##############################################################################
mxmap.o: mxmap.c dirsep.h getopt.h mapprint.h mxbitio.h mxlargs.h mxload.h   \
         preamble.h rdbkrcd.h tempfile.h
##############################################################################
mxmbx.o: mxmbx.c aclmodes.h copybits.h cvpath.h dirsep.h getopt.h gettype.h  \
         mapprint.h mxbitio.h mxlargs.h mxload.h mxlopts.h mxmseg.h optptr.h \
         parsctl.h preamble.h rdbkrcd.h tempfile.h utime.h
##############################################################################
mxmseg.o: mxmseg.c copybits.h dirsep.h mapprint.h mxbitio.h mxload.h         \
          mxlopts.h mxmseg.h optptr.h preamble.h tempfile.h timestr.h
##############################################################################
optptr.o: optptr.c mxbitio.h mxload.h mxlopts.h optptr.h preamble.h
##############################################################################
parsctl.o: parsctl.c dirsep.h mxload.h mxlopts.h mxloptsi.h parsctl.h
##############################################################################
pr8bit.o: pr8bit.c mxbitio.h mxload.h
##############################################################################
preamble.o: preamble.c mxbitio.h mxload.h preamble.h timestr.h
##############################################################################
rdbkrcd.o: rdbkrcd.c copybits.h mxbitio.h mxload.h preamble.h
##############################################################################
tempfile.o: tempfile.c dirsep.h mxbitio.h mxload.h
##############################################################################
timestr.o: timestr.c mxload.h timestr.h tmchart.h
##############################################################################
