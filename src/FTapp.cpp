/*
** Copyright (C) 2002 Jesse Chappell <jesse@essej.net>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**  
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**  
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**  
*/


#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <signal.h>
#include <stdio.h>

//#include <wx/wx.h>

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/cmdline.h>

#include "version.h"

#include "FTapp.hpp"
#include "FTtypes.hpp"
#include "FTmainwin.hpp"
#include "FTioSupport.hpp"
#include "FTprocessPath.hpp"
#include "FTspectralManip.hpp"


#ifdef HAVE_SFFTW_H
#include <sfftw.h>
#else
#include <fftw.h>
#endif


// Create a new application object: this macro will allow wxWindows to create
// the application object during program execution (it's better than using a
// static object for many reasons) and also declares the accessor function
// wxGetApp() which will return the reference of the right type (i.e. MyApp and
// not wxApp)
IMPLEMENT_APP(FTapp)

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// the application class
// ----------------------------------------------------------------------------


static const wxCmdLineEntryDesc cmdLineDesc[] =
{
	{ wxCMD_LINE_SWITCH, "h", "help", "show this help", wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
	{ wxCMD_LINE_OPTION, "c", "channels",    "# processing channels (1-4) default is 2", wxCMD_LINE_VAL_NUMBER },
	{ wxCMD_LINE_OPTION, "i", "inputs",
	  "connect inputs from these jack ports (separate each channel with commas).\n\t\t\t  Defaults to 'alsa_pcm:in_1,..." },
	{ wxCMD_LINE_OPTION, "o", "outputs",
	  "connect outputs to these jack ports (separate each channel with commas).\n\t\t\t  Defaults to 'alsa_pcm:out_1,...'" },
	{ wxCMD_LINE_OPTION, "n", "jack-name",    "jack name.   default is freqtweak-PID"},
	{ wxCMD_LINE_OPTION, "p", "preset",    "load given preset initially"},
	{ wxCMD_LINE_OPTION, "r", "rc-dir",    "what directory to use for run-control state. default is ~/.freqtweak"},
	{ wxCMD_LINE_NONE }
};	

	
FTapp::FTapp()
	: _mainwin(0)
{
}

static void onTerminate(int arg)
{
	::wxGetApp().getMainwin()->Close(TRUE);

	::wxGetApp().ExitMainLoop();
	printf ("bye bye, hope you had fun...\n");
}

// `Main program' equivalent: the program execution "starts" here
bool FTapp::OnInit()
{
	signal (SIGTERM, onTerminate);
	signal (SIGINT, onTerminate);

	wxString inputports[4];
	wxString outputports[4];
	wxString jackname;
	wxString preset;
	wxString rcdir;
	int pcnt = 2;
	
	SetExitOnFrameDelete(TRUE);

	if (sizeof(sample_t) != sizeof(fftw_real)) {
		fprintf(stderr, "FFTW Mismatch!  You need to build FreqTweak against a single-precision\n");
		fprintf(stderr, "  FFTW library.  See the INSTALL file for instructions.\n");  		
		return FALSE;
	}
	
	// use stderr as log
	wxLog *logger=new wxLogStderr();
	logger->SetTimestamp(NULL);
	wxLog::SetActiveTarget(logger);
	
	wxCmdLineParser parser(argc, argv);
	parser.SetDesc(cmdLineDesc);
	parser.SetLogo(wxString::Format("FreqTweak %s\n%s%s%s%s", freqtweak_version,
					"Copyright 2002 Jesse Chappell\n",
					"FreqTweak comes with ABSOLUTELY NO WARRANTY\n",
					"This is free software, and you are welcome to redistribute it\n",
					"under certain conditions; see the file COPYING for details\n"));

	int ret = parser.Parse();

	if (ret != 0) {
		// help or error
		return FALSE;
	}

	wxString strval;
	long longval;

	if (parser.Found ("c", &longval)) {
		if (longval < 1 || longval > 4) {
			fprintf(stderr, "Error: channel count must be in range [1-4]\n");
			parser.Usage();
			return FALSE;
		}
		pcnt = (int) longval;
	}
	
	if (parser.Found ("i", &strval))
	{
		// parse comma separated values
		wxString port = strval.BeforeFirst(',');
		wxString remain = strval.AfterFirst(',');
		int id=0;
		while (!port.IsEmpty() && id < pcnt) {
			inputports[id++] = port;			
			port = remain.BeforeFirst(',');
			remain = remain.AfterFirst(',');
		}
		
	}
	else {
		// connect default ports
		for (int id=0; id < pcnt; id++) {
			inputports[id] = wxString::Format ("alsa_pcm:in_%d", id+1);				
		}
	}

	// OUTPUT PORTS
	if (parser.Found ("o", &strval))
	{
		// parse comma separated values
		wxString port = strval.BeforeFirst(',');
		wxString remain = strval.AfterFirst(',');
		int id=0;
		while (!port.IsEmpty() && id < pcnt) {
			outputports[id++] = port;			
			port = remain.BeforeFirst(',');
			remain = remain.AfterFirst(',');
		}
		
	}
	else {
		// connect default ports
		for (int id=0; id < pcnt; id++) {
			outputports[id] = wxString::Format ("alsa_pcm:out_%d", id+1);
		}
	}

	if (parser.Found ("n", &jackname)) {
	       FTioSupport::setDefaultName (jackname);
	}

	parser.Found ("r", &rcdir);
	parser.Found ("p", &preset);

	
	// initialize jack support
	FTioSupport * iosup = FTioSupport::instance();

	if (!iosup->init()) {
		fprintf(stderr, "Error connecting to jack!\n");
		return FALSE;
	}

	
	
	// Create the main application window
	_mainwin = new FTmainwin(pcnt, "FreqTweak", rcdir,
				 wxPoint(100, 100), wxDefaultSize);

	
	// Show it and tell the application that it's our main window
	_mainwin->SetSize(654,760);
	_mainwin->Show(TRUE);

	SetTopWindow(_mainwin);


	// only start processing after building mainwin
	iosup->startProcessing();


	if ( preset.IsEmpty()) {
		// connect initial I/O
		for (int id=0; id < pcnt; id++)
		{
			iosup->connectPathInput(id, inputports[id]);
			iosup->connectPathOutput(id, outputports[id]);
		}
	}
	else {
		_mainwin->loadPreset(preset);
	}
	

	
	_mainwin->updateDisplay();
	
		
	// success: wxApp::OnRun() will be called which will enter the main message
	// loop and the application will run. If we returned FALSE here, the
	// application would exit immediately.
	return TRUE;
}
