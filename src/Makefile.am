# Copyright (C) 2002 Jesse Chappell <jesse@essej.net>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.


COMMON_FLAGS =  -Wall -D_REENTRANT \
		 @ARCH_CFLAGS@

AM_CXXFLAGS = $(COMMON_FLAGS) $(EXTRA_CFLAGS) $(JACK_CFLAGS) @XML_CFLAGS@

AM_CFLAGS = $(AM_CXXFLAGS)


freqtweak_LDADD = @JACK_LIBS@ @XML_LIBS@


bin_PROGRAMS = freqtweak

freqtweak_SOURCES = \
	FTapp.cpp \
	FTmainwin.cpp \
	FTioSupport.cpp \
	FTjackSupport.cpp \
	FTprocessPath.cpp \
	FTspectralEngine.cpp \
	FTspectragram.cpp \
	FTspectrumModifier.cpp \
	FTactiveBarGraph.cpp \
	FTutils.cpp \
	FTportSelectionDialog.cpp\
	FTconfigManager.cpp \
	RingBuffer.cpp \
	xml++.cpp \
	xml++.hpp \
	FTapp.hpp \
	FTmainwin.hpp \
	FTjackSupport.hpp \
	FTioSupport.hpp \
	FTprocessPath.hpp \
	FTtypes.hpp \
	FTspectralEngine.hpp \
	FTspectragram.hpp \
	FTspectrumModifier.hpp \
	FTactiveBarGraph.hpp \
	FTutils.hpp \
	FTportSelectionDialog.hpp \
	FTconfigManager.hpp \
	FTupdateToken.hpp \
	RingBuffer.hpp \
	FTprocI.cpp \
	FTprocI.hpp \
	FTprocDelay.cpp \
	FTprocDelay.hpp \
	FTprocEQ.cpp \
	FTprocEQ.hpp \
	FTprocGate.cpp \
	FTprocGate.hpp \
	FTprocPitch.cpp \
	FTprocPitch.hpp \
	FTprocOrderDialog.cpp \
	FTprocOrderDialog.hpp \
	FTdspManager.cpp \
	FTdspManager.hpp \
	FTprocLimit.cpp \
	FTprocLimit.hpp \
	FTprocTranspose.cpp \
	FTprocTranspose.hpp 



