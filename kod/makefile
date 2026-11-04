#
# Makefile for compiling the Blakod. This is the 'main' makefile, so kod.mak
# is reproduced here with some extra commands to run after the makefile
# recurse is done (notably creating the client rsb file). We need to do this
# here instead of the root directory makefile in case anyone builds the blakod
# from this directory.
#

TOPDIR=..
!include $(TOPDIR)\common.mak

BOFS = util.bof object.bof

.SUFFIXES : .kod

BCFLAGS = -d -R -I $(KODINCLUDEDIR) -K $(KODDIR)\kodbase.txt -O $(BLAKSERVRUNDIR)

all :
	$(BC) $(BCFLAGS) $(KODDIR)

	@echo Copying kodbase.txt and kod include files
	-@$(CP) $(KODDIR)\kodbase.txt $(BLAKSERVRUNDIR) 2>&1
	-@$(CP) $(KODDIR)\include\*.khd $(BLAKSERVRUNDIR) 2>&1
	@echo Copying .rsc files, creating server and client .rsb files
	@-$(RM) $(BLAKSERVRUNDIR)\rsc\*.rsb >nul 2>&1
	@-$(RM) $(CLIENTRUNDIR)\resource\*.rsb >nul 2>&1
	@$(RSCMERGE) $(BLAKSERVRUNDIR)\rsc\rsc0000.rsb $(BLAKSERVRUNDIR)\rsc
	-@$(CP) $(BLAKSERVRUNDIR)\rsc\rsc0000.rsb $(CLIENTRUNDIR)\resource\ 2>&1

clean :
	@-$(RM) *.bof *.rsc kodbase.txt >nul 2>&1
	@-clean
	@-$(RM) $(BLAKSERVRUNDIR)\rsc\*.rsc >nul 2>&1
	@-$(RM) $(BLAKSERVRUNDIR)\loadkod\*.bof >nul 2>&1
	@-$(RM) $(BLAKSERVRUNDIR)\memmap\*.bof >nul 2>&1

