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


// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"
// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <wx/sashwin.h>
#include <wx/spinctrl.h>
#include <wx/notebook.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "FTmainwin.hpp"
#include "FTspectragram.hpp"
#include "FTioSupport.hpp"
#include "FTprocessPath.hpp"
#include "FTspectralManip.hpp"
#include "FTactiveBarGraph.hpp"
#include "FTportSelectionDialog.hpp"
#include "FTspectrumModifier.hpp"
#include "FTconfigManager.hpp"
#include "FTupdateToken.hpp"

#include "LockFreeFifo.hpp"
#include "version.h"

//#include "images/bypass.xpm"
//#include "images/bypass_active.xpm"
//#include "images/link.xpm"
//#include "images/link_active.xpm"


// ----------------------------------------------------------------------------
// event tables and other macros for wxWindows
// ----------------------------------------------------------------------------

// IDs for the controls and the menu commands
enum WindowIds
{
	// menu items
	FT_QuitMenu = 1,
	FT_AboutMenu,
	FT_InputButtonId,
	FT_OutputButtonId,
	FT_InSpecTypeId,
	FT_FreqBypassId,
	FT_DelayBypassId,
	FT_FeedbackBypassId,
	FT_ScaleBypassId,
	FT_MashBypassId,
	FT_GateBypassId,
	FT_OutSpecTypeId,

	FT_InSpecLabelId,
	FT_FreqLabelId,
	FT_DelayLabelId,
	FT_FeedbackLabelId,
	FT_ScaleLabelId,
	FT_MashLabelId,
	FT_GateLabelId,
	FT_OutSpecLabelId,

	FT_FreqLinkId,
	FT_DelayLinkId,
	FT_FeedbackLinkId,
	FT_ScaleLinkId,
	FT_MashLinkId,
	FT_GateLinkId,
	FT_FreqBinsChoiceId,
	FT_OverlapChoiceId,
	FT_WindowingChoiceId,
	FT_TimescaleSliderId,
	FT_FreqScaleChoiceId,

	FT_RowPanelId,
	FT_RowPanel,
	FT_BypassId,
	FT_MuteId,

	FT_GainSlider,
	FT_GainSpin,
	FT_MixSlider,
	FT_PathCountChoice,

	FT_StoreButton,
	FT_LoadButton,
	FT_PresetCombo,
	FT_PlotSpeedChoiceId,
	FT_PlotSuperSmoothId,
	
	FT_MixLinkedButton,
	FT_IOreconnectButton,
	FT_IOnameText,
};



// the event tables connect the wxWindows events with the functions (event
// handlers) which process them. It can be also done at run-time, but for the
// simple menu events like this the static method is much simpler.
BEGIN_EVENT_TABLE(FTmainwin, wxFrame)
//	EVT_SIZE(FTmainwin::OnSize)
	EVT_MENU(FT_QuitMenu,  FTmainwin::OnQuit)
	EVT_MENU(FT_AboutMenu, FTmainwin::OnAbout)

	EVT_IDLE(FTmainwin::OnIdle)
	EVT_BUTTON(FT_InputButtonId, FTmainwin::handleInputButton)
	EVT_BUTTON(FT_OutputButtonId, FTmainwin::handleOutputButton)
	EVT_BUTTON(FT_FreqLinkId, FTmainwin::handleLinkButtons)
	EVT_BUTTON(FT_FreqBypassId, FTmainwin::handleBypassButtons)
	EVT_BUTTON(FT_DelayLinkId, FTmainwin::handleLinkButtons)
	EVT_BUTTON(FT_DelayBypassId, FTmainwin::handleBypassButtons)
	EVT_BUTTON(FT_FeedbackLinkId, FTmainwin::handleLinkButtons)
	EVT_BUTTON(FT_FeedbackBypassId, FTmainwin::handleBypassButtons)
	EVT_BUTTON(FT_ScaleLinkId, FTmainwin::handleLinkButtons)
	EVT_BUTTON(FT_ScaleBypassId, FTmainwin::handleBypassButtons)
	EVT_BUTTON(FT_MashLinkId, FTmainwin::handleLinkButtons)
	EVT_BUTTON(FT_MashBypassId, FTmainwin::handleBypassButtons)
	EVT_BUTTON(FT_GateLinkId, FTmainwin::handleLinkButtons)
	EVT_BUTTON(FT_GateBypassId, FTmainwin::handleBypassButtons)

	EVT_CHOICE(FT_FreqBinsChoiceId, FTmainwin::handleChoices)
	EVT_CHOICE(FT_OverlapChoiceId, FTmainwin::handleChoices)
	EVT_CHOICE(FT_WindowingChoiceId, FTmainwin::handleChoices)
	EVT_CHOICE(FT_PlotSpeedChoiceId, FTmainwin::handleChoices)
	EVT_CHECKBOX(FT_PlotSuperSmoothId, FTmainwin::handleChoices)

	EVT_CHOICE(FT_PathCountChoice, FTmainwin::handlePathCount)

	
	EVT_COMMAND_SCROLL(FT_MixSlider, FTmainwin::handleMixSlider)
	EVT_SPINCTRL(FT_GainSpin, FTmainwin::handleGain)
	
	EVT_SASH_DRAGGED(FT_RowPanelId, FTmainwin::handleSashDragged)

	EVT_BUTTON(FT_InSpecLabelId, FTmainwin::handleLabelButtons)
	EVT_BUTTON(FT_FreqLabelId, FTmainwin::handleLabelButtons)
	EVT_BUTTON(FT_DelayLabelId, FTmainwin::handleLabelButtons)
	EVT_BUTTON(FT_FeedbackLabelId, FTmainwin::handleLabelButtons)
	EVT_BUTTON(FT_ScaleLabelId, FTmainwin::handleLabelButtons)
	EVT_BUTTON(FT_MashLabelId, FTmainwin::handleLabelButtons)
	EVT_BUTTON(FT_GateLabelId, FTmainwin::handleLabelButtons)
	EVT_BUTTON(FT_OutSpecLabelId, FTmainwin::handleLabelButtons)

	EVT_BUTTON(FT_InSpecTypeId, FTmainwin::handlePlotTypeButtons)
	EVT_BUTTON(FT_OutSpecTypeId, FTmainwin::handlePlotTypeButtons)

	EVT_BUTTON(FT_BypassId, FTmainwin::handleBypassButtons)
	EVT_BUTTON(FT_MuteId, FTmainwin::handleBypassButtons)

	EVT_BUTTON(FT_StoreButton, FTmainwin::handleStoreButton)
	EVT_BUTTON(FT_LoadButton, FTmainwin::handleLoadButton)

	EVT_BUTTON(FT_MixLinkedButton, FTmainwin::handleLinkButtons)
	EVT_BUTTON(FT_IOreconnectButton, FTmainwin::handleIOButtons)
	
END_EVENT_TABLE()


// ----------------------------------------------------------------------------
// main frame
// ----------------------------------------------------------------------------

FTmainwin::FTmainwin(int startpath, const wxString& title, const wxString& rcdir, const wxPoint& pos, const wxSize& size)
	: wxFrame((wxFrame *)NULL, -1, title, pos, size), _startpaths(startpath),
	  _inspecShown(true), _freqShown(true), _scaleShown(true), _mashShown(true), _gateShown(true),
	  _delayShown(true), _feedbShown(true), _outspecShown(true), _linkedMix(true),

	  _updateMS(10), _superSmooth(false),
	  _pathCount(startpath), _rowCount(7),
	  _configManager(rcdir)
{
	_eventTimer = new FTupdateTimer(this);

	_rowItems = new wxWindow* [_rowCount];
	
	for (int i=0; i < FT_MAXPATHS; i++) {
		_processPath[i] = 0;
		_updateTokens[i] = new FTupdateToken();
	}
	
	buildGui();

}

void FTmainwin::buildGui()
{
	int bwidth = 20;
	int labwidth = 74;
	int bheight = 16;
	
	// set the frame icon
	//SetIcon(wxICON(mondrian));
	
	// create a menu bar
	wxMenu *menuFile = new wxMenu("", wxMENU_TEAROFF);
	
	menuFile->Append(FT_AboutMenu, "&About...\tCtrl-A", "Show about dialog");
	menuFile->AppendSeparator();
	menuFile->Append(FT_QuitMenu, "&Quit\tCtrl-Q", "Quit this program");

	// now append the freshly created menu to the menu bar...
	wxMenuBar *menuBar = new wxMenuBar();
	menuBar->Append(menuFile, "&File");
	
	// ... and attach this menu bar to the frame
	SetMenuBar(menuBar);
	
#if wxUSE_STATUSBAR
	// create a status bar just for fun (with 3 panes)
	CreateStatusBar(3);
	SetStatusText("FreqTweak", 0);
#endif // wxUSE_STATUSBAR

	// terrible!
	_defaultBg = GetBackgroundColour();
	//_activeBg.Set (254,185,55);
	_activeBg.Set (246,229,149);
	
	wxFont titleFont(10, wxDEFAULT, wxNORMAL, wxBOLD);
	wxFont titleAltFont(10, wxDEFAULT, wxSLANT, wxBOLD);
	wxFont buttFont(10, wxDEFAULT, wxNORMAL, wxNORMAL);
	
	// get processPaths from jacksupport
	FTioSupport * iosup = FTioSupport::instance();

	
	wxBoxSizer *mainsizer = new wxBoxSizer(wxVERTICAL);
	
	// overall controls
	wxNotebook * ctrlbook = new wxNotebook(this,-1, wxDefaultPosition, wxSize(-1, 70));

	// preset panel
	wxPanel * configpanel = new wxPanel (ctrlbook, -1);
	wxBoxSizer *configSizer = new wxBoxSizer(wxHORIZONTAL);

	_presetCombo = new wxComboBox (configpanel, FT_PresetCombo, "",  wxDefaultPosition, wxSize(200,-1));
	configSizer->Add( _presetCombo, 0, wxALL|wxALIGN_CENTER, 2);

	
	wxButton *storeButt = new wxButton(configpanel, FT_StoreButton, "Store");
	storeButt->SetFont(buttFont);
	configSizer->Add( storeButt, 0, wxALL|wxALIGN_CENTER, 2);

	wxButton *loadButt = new wxButton(configpanel, FT_LoadButton, "Load");
	loadButt->SetFont(buttFont);
	configSizer->Add( loadButt, 0, wxALL|wxALIGN_CENTER, 2);

	configpanel->SetAutoLayout(TRUE);
	configSizer->Fit( configpanel );  
	configpanel->SetSizer(configSizer);

	ctrlbook->AddPage ( (wxNotebookPage *) configpanel, "Presets" , true);
	
	//mainsizer->Add (configSizer, 0, wxALL|wxEXPAND, 1);

	
	wxPanel * fftpanel = new wxPanel(ctrlbook, -1);

	wxStaticText *stattext;
	//wxBoxSizer *specCtrlSizer = new wxStaticBoxSizer(new wxStaticBox(fftpanel, -1, "FFT Params"), wxHORIZONTAL);
	wxBoxSizer *specCtrlSizer = new wxBoxSizer(wxHORIZONTAL);

	stattext =  new wxStaticText(fftpanel, -1, "Freq Bands");
	specCtrlSizer->Add (stattext, 0, wxALIGN_CENTER|wxLEFT, 4);
	_freqBinsChoice = new wxChoice(fftpanel, FT_FreqBinsChoiceId);
	const int * fftbins = FTspectralManip::getFFTSizes();
	for (int i=0; i < FTspectralManip::getFFTSizeCount(); i++) {
		_freqBinsChoice->Append(wxString::Format("%d", fftbins[i] / 2), (void *) fftbins[i]);
	}

	specCtrlSizer->Add(_freqBinsChoice, 0, wxALL|wxALIGN_CENTER, 2);

	stattext =  new wxStaticText(fftpanel, -1, "Overlap");
	specCtrlSizer->Add ( stattext, 0, wxALIGN_CENTER|wxLEFT, 4);
	_overlapChoice = new wxChoice(fftpanel, FT_OverlapChoiceId);
	_overlapChoice->Append("0 %", (void *) 1);
	_overlapChoice->Append("50 %", (void *) 2);
	_overlapChoice->Append("75 %", (void *) 4);
	_overlapChoice->Append("87.5 %", (void *) 8);
	_overlapChoice->Append("93.75 %", (void *) 16);
	_overlapChoice->SetSelection(2); // hack
	specCtrlSizer->Add(_overlapChoice, 0, wxALL|wxALIGN_CENTER, 2);

	stattext =  new wxStaticText(fftpanel, -1, "Window");
	specCtrlSizer->Add ( stattext, 0, wxALIGN_CENTER|wxLEFT, 4);
	_windowingChoice = new wxChoice(fftpanel, FT_WindowingChoiceId, wxDefaultPosition, wxSize(100,-1));
	const char ** winstrs = FTspectralManip::getWindowStrings();
	for (int i=0; i < FTspectralManip::getWindowStringsCount(); i++) {
		_windowingChoice->Append(winstrs[i]);
	}
	_windowingChoice->SetSelection(0);
	

	specCtrlSizer->Add(_windowingChoice, 0, wxALL|wxALIGN_CENTER, 2);

	fftpanel->SetAutoLayout(TRUE);
	specCtrlSizer->Fit( fftpanel );  
	fftpanel->SetSizer(specCtrlSizer);

	ctrlbook->AddPage ( (wxNotebookPage *) fftpanel, "FFT");


	// plot page
	wxPanel * plotpanel = new wxPanel (ctrlbook, -1);
	wxBoxSizer *plotSizer = new wxBoxSizer(wxHORIZONTAL);

	stattext =  new wxStaticText(plotpanel, -1, "Speed");
	plotSizer->Add ( stattext, 0, wxALIGN_CENTER|wxLEFT, 4);
	_plotSpeedChoice = new wxChoice(plotpanel, FT_PlotSpeedChoiceId, wxDefaultPosition, wxSize(100,-1));
	_plotSpeedChoice->Append("Turtle", (void *) FTspectralManip::SPEED_TURTLE);
	_plotSpeedChoice->Append("Slow", (void *) FTspectralManip::SPEED_SLOW);
	_plotSpeedChoice->Append("Medium", (void *) FTspectralManip::SPEED_MED);
	_plotSpeedChoice->Append("Fast", (void *) FTspectralManip::SPEED_FAST);
	//_plotSpeedChoice->SetSelection(2); // hack
	plotSizer->Add(_plotSpeedChoice, 0, wxALL|wxALIGN_CENTER, 2);

	_superSmoothCheck = new wxCheckBox (plotpanel, FT_PlotSuperSmoothId, "Extra smooth but expensive"); 
	_superSmoothCheck->SetValue(false);
	plotSizer->Add (_superSmoothCheck, 0, wxALL|wxALIGN_CENTER, 2);
	
	plotpanel->SetAutoLayout(TRUE);
	plotSizer->Fit( plotpanel );  
	plotpanel->SetSizer(plotSizer);

	ctrlbook->AddPage ( (wxNotebookPage *) plotpanel, "Plots" , false);
	

	// IO page
	wxPanel * iopanel = new wxPanel (ctrlbook, -1);
	wxBoxSizer *ioSizer = new wxBoxSizer(wxHORIZONTAL);

	stattext =  new wxStaticText(iopanel, -1, "JACK-Name");
	ioSizer->Add ( stattext, 0, wxALIGN_CENTER|wxLEFT, 4);
	_ioNameText = new wxTextCtrl (iopanel, FT_IOnameText, "", wxDefaultPosition, wxSize(150,-1));
	ioSizer->Add (_ioNameText, 0, wxALL|wxALIGN_CENTER, 2);
	
	wxButton * reconnButton = new wxButton(iopanel, FT_IOreconnectButton, "Reconnect As");
	ioSizer->Add (reconnButton, 0, wxALL|wxALIGN_CENTER, 2);
	iopanel->SetAutoLayout(TRUE);
	ioSizer->Fit( iopanel );  
	iopanel->SetSizer(ioSizer);
	
	ctrlbook->AddPage ( (wxNotebookPage *) iopanel, "I/O" , false);
	
	
	
	mainsizer->Add ( ctrlbook, 0, wxALL|wxEXPAND, 2 );
	

	wxBoxSizer *mainvsizer = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer  *tmpsizer, *tmpsizer2;

	_uppersizer = new wxBoxSizer(wxHORIZONTAL);
	_inspecsizer = new wxBoxSizer(wxHORIZONTAL);
	_scalesizer = new wxBoxSizer(wxHORIZONTAL);
	//_mashsizer = new wxBoxSizer(wxHORIZONTAL);
	_gatesizer = new wxBoxSizer(wxHORIZONTAL);
	_freqsizer = new wxBoxSizer(wxHORIZONTAL);
	_delaysizer = new wxBoxSizer(wxHORIZONTAL);
	_feedbsizer = new wxBoxSizer(wxHORIZONTAL);
	_outspecsizer = new wxBoxSizer(wxHORIZONTAL);
	_lowersizer = new wxBoxSizer(wxHORIZONTAL);


	int rowh = 68;
	int rowi = 0;
	
	//_rowPanel = new wxPanel(this, -1);
	_rowPanel = new wxScrolledWindow(this, FT_RowPanel, wxDefaultPosition, wxDefaultSize, wxVSCROLL|wxSUNKEN_BORDER);
	
	_inspecSash = new wxSashLayoutWindow(_rowPanel, FT_RowPanelId, wxDefaultPosition, wxSize(-1,rowh));
	_inspecPanel = new wxPanel(_inspecSash, -1);
	_inspecSash->SetSashBorder(wxSASH_BOTTOM, true);
	_inspecSash->SetSashVisible(wxSASH_BOTTOM, true);
	_rowItems[rowi++] = _inspecSash;
	
	_freqSash = new wxSashLayoutWindow(_rowPanel, FT_RowPanelId, wxDefaultPosition, wxSize(-1,rowh));
	_freqPanel = new wxPanel(_freqSash, -1);
	_freqSash->SetSashBorder(wxSASH_BOTTOM, true);
	_freqSash->SetSashVisible(wxSASH_BOTTOM, true);
	_rowItems[rowi++] = _freqSash;

	_scaleSash = new wxSashLayoutWindow(_rowPanel, FT_RowPanelId, wxDefaultPosition, wxSize(-1,rowh));
	_scalePanel = new wxPanel(_scaleSash, -1);
	_scaleSash->SetSashBorder(wxSASH_BOTTOM, true);
	_scaleSash->SetSashVisible(wxSASH_BOTTOM, true);
	_rowItems[rowi++] = _scaleSash;
	
// 	_mashSash = new wxSashLayoutWindow(_rowPanel, FT_RowPanelId, wxDefaultPosition, wxSize(-1,rowh));
// 	_mashPanel = new wxPanel(_mashSash, -1);
// 	_mashSash->SetSashBorder(wxSASH_BOTTOM, true);
// 	_mashSash->SetSashVisible(wxSASH_BOTTOM, true);
// 	_rowItems[rowi++] = _mashSash;

	_gateSash = new wxSashLayoutWindow(_rowPanel, FT_RowPanelId, wxDefaultPosition, wxSize(-1,rowh));
	_gatePanel = new wxPanel(_gateSash, -1);
	_gateSash->SetSashBorder(wxSASH_BOTTOM, true);
	_gateSash->SetSashVisible(wxSASH_BOTTOM, true);
	_rowItems[rowi++] = _gateSash;
	

	
	_delaySash = new wxSashLayoutWindow(_rowPanel, FT_RowPanelId, wxDefaultPosition, wxSize(-1,rowh));
	_delayPanel = new wxPanel(_delaySash, -1);
	_delaySash->SetSashBorder(wxSASH_BOTTOM, true);
	_delaySash->SetSashVisible(wxSASH_BOTTOM, true);
	_rowItems[rowi++] = _delaySash;

	_feedbSash = new wxSashLayoutWindow(_rowPanel, FT_RowPanelId, wxDefaultPosition, wxSize(-1,rowh));
	_feedbPanel = new wxPanel(_feedbSash, -1);
	_feedbSash->SetSashBorder(wxSASH_BOTTOM, true);
	_feedbSash->SetSashVisible(wxSASH_BOTTOM, true);
	_rowItems[rowi++] = _feedbSash;

	_outspecSash = new wxSashLayoutWindow(_rowPanel, FT_RowPanelId, wxDefaultPosition, wxSize(-1,rowh));
	_outspecPanel = new wxPanel(_outspecSash, -1);
	_outspecSash->SetSashBorder(wxSASH_BOTTOM, true);
	_outspecSash->SetSashVisible(wxSASH_BOTTOM, true);
	_rowItems[rowi++] = _outspecSash;


	
	// create master link and bypass buttons
	_inspecLabelButton = new wxButton(_inspecPanel, FT_InSpecLabelId, "In Spectra",
 					  wxDefaultPosition, wxSize(labwidth,bheight));
	_inspecLabelButton->SetFont(titleFont);
	
 	_freqLabelButton = new wxButton(_freqPanel, FT_FreqLabelId, "EQ",
 					  wxDefaultPosition, wxSize(labwidth,bheight));
	_freqLabelButton->SetFont(titleFont);

 	_delayLabelButton = new wxButton(_delayPanel, FT_DelayLabelId, "Delay",
 					   wxDefaultPosition, wxSize(labwidth,bheight));
	_delayLabelButton->SetFont(titleFont);

 	_scaleLabelButton = new wxButton(_scalePanel, FT_ScaleLabelId, "Pitch",
					   wxDefaultPosition, wxSize(labwidth,bheight));
	_scaleLabelButton->SetFont(titleFont);

//  	_mashLabelButton = new wxButton(_mashPanel, FT_MashLabelId, "Mash",
// 					   wxDefaultPosition, wxSize(labwidth,bheight));
// 	_mashLabelButton->SetFont(titleFont);

 	_gateLabelButton = new wxButton(_gatePanel, FT_GateLabelId, "Gate",
					   wxDefaultPosition, wxSize(labwidth,bheight));
	_gateLabelButton->SetFont(titleFont);

	
	_feedbLabelButton = new wxButton(_feedbPanel, FT_FeedbackLabelId, "Feedback",
					   wxDefaultPosition, wxSize(labwidth,bheight));
	_feedbLabelButton->SetFont(titleFont);

	_outspecLabelButton = new wxButton(_outspecPanel, FT_OutSpecLabelId, "Out Spectra",
						  wxDefaultPosition, wxSize(labwidth,bheight));
	_outspecLabelButton->SetFont(titleFont);

	wxLayoutConstraints * constr;
	
	// create alts
	_inspecLabelButtonAlt = new wxButton(_rowPanel, FT_InSpecLabelId, "In Spectra",
 					  wxDefaultPosition, wxSize(labwidth,bheight));
	_inspecLabelButtonAlt->SetFont(titleAltFont);
	_inspecLabelButtonAlt->Show(false);
	constr = new wxLayoutConstraints;
	constr->left.SameAs (_rowPanel, wxLeft, 2);
	constr->width.Absolute (labwidth);
	constr->top.SameAs (_rowPanel, wxTop);
	constr->height.Absolute (bheight);
	_inspecLabelButtonAlt->SetConstraints(constr);
	
 	_freqLabelButtonAlt = new wxButton(_rowPanel, FT_FreqLabelId, "EQ",
 					  wxDefaultPosition, wxSize(labwidth,bheight));
	_freqLabelButtonAlt->SetFont(titleAltFont);
	_freqLabelButtonAlt->Show(false);
	constr = new wxLayoutConstraints;
	constr->left.SameAs (_rowPanel, wxLeft, 2);
	constr->width.Absolute (labwidth);
	constr->height.Absolute (bheight);
	constr->top.SameAs (_freqSash, wxTop);
	_freqLabelButtonAlt->SetConstraints(constr);

	
 	_scaleLabelButtonAlt = new wxButton(_rowPanel, FT_ScaleLabelId, "Pitch",
					   wxDefaultPosition, wxSize(labwidth,bheight));
	_scaleLabelButtonAlt->SetFont(titleAltFont);
	_scaleLabelButtonAlt->Show(false);
	constr = new wxLayoutConstraints;
	constr->left.SameAs (_rowPanel, wxLeft, 2);
	constr->width.Absolute (labwidth);
	constr->height.Absolute (bheight);
	constr->top.SameAs (_scaleSash, wxTop);
	_scaleLabelButtonAlt->SetConstraints(constr);

//  	_mashLabelButtonAlt = new wxButton(_rowPanel, FT_MashLabelId, "Mash",
// 					   wxDefaultPosition, wxSize(labwidth,bheight));
// 	_mashLabelButtonAlt->SetFont(titleAltFont);
// 	_mashLabelButtonAlt->Show(false);
// 	constr = new wxLayoutConstraints;
// 	constr->left.SameAs (_rowPanel, wxLeft, 2);
// 	constr->width.Absolute (labwidth);
// 	constr->height.Absolute (bheight);
// 	constr->top.SameAs (_mashSash, wxTop);
// 	_mashLabelButtonAlt->SetConstraints(constr);


 	_gateLabelButtonAlt = new wxButton(_rowPanel, FT_GateLabelId, "Gate",
					   wxDefaultPosition, wxSize(labwidth,bheight));
	_gateLabelButtonAlt->SetFont(titleAltFont);
	_gateLabelButtonAlt->Show(false);
	constr = new wxLayoutConstraints;
	constr->left.SameAs (_rowPanel, wxLeft, 2);
	constr->width.Absolute (labwidth);
	constr->height.Absolute (bheight);
	constr->top.SameAs (_gateSash, wxTop);
	_gateLabelButtonAlt->SetConstraints(constr);

 	_delayLabelButtonAlt = new wxButton(_rowPanel, FT_DelayLabelId, "Delay",
 					   wxDefaultPosition, wxSize(labwidth,bheight));
	_delayLabelButtonAlt->SetFont(titleAltFont);
	_delayLabelButtonAlt->Show(false);
	constr = new wxLayoutConstraints;
	constr->left.SameAs (_rowPanel, wxLeft, 2);
	constr->width.Absolute (labwidth);
	constr->height.Absolute (bheight);
	constr->top.SameAs (_delaySash, wxTop);
	_delayLabelButtonAlt->SetConstraints(constr);
	
	
	_feedbLabelButtonAlt = new wxButton(_rowPanel, FT_FeedbackLabelId, "Feedback",
					   wxDefaultPosition, wxSize(labwidth,bheight));
	_feedbLabelButtonAlt->SetFont(titleAltFont);
	_feedbLabelButtonAlt->Show(false);
	constr = new wxLayoutConstraints;
	constr->left.SameAs (_rowPanel, wxLeft, 2);
	constr->width.Absolute (labwidth);
	constr->height.Absolute (bheight);
	constr->top.SameAs (_feedbSash, wxTop);
	_feedbLabelButtonAlt->SetConstraints(constr);

	
	_outspecLabelButtonAlt = new wxButton(_rowPanel, FT_OutSpecLabelId, "Out Spectra",
						  wxDefaultPosition, wxSize(labwidth,bheight));
	_outspecLabelButtonAlt->SetFont(titleAltFont);
	_outspecLabelButtonAlt->Show(false);
	constr = new wxLayoutConstraints;
	constr->left.SameAs (_rowPanel, wxLeft, 2);
	constr->width.Absolute (labwidth);
	constr->height.Absolute (bheight);
	constr->top.SameAs (_outspecSash, wxTop);
	_outspecLabelButtonAlt->SetConstraints(constr);


	
	
	_freqLinkAllButton = new wxButton(_freqPanel, FT_FreqLinkId, "LA",
 					  wxDefaultPosition, wxSize(bwidth,bheight));
	_freqLinkAllButton->SetFont(buttFont);
	
 	_delayLinkAllButton = new wxButton(_delayPanel, FT_DelayLinkId, "LA",
 					   wxDefaultPosition, wxSize(bwidth,bheight));
	_delayLinkAllButton->SetFont(buttFont);

 	_scaleLinkAllButton = new wxButton(_scalePanel, FT_ScaleLinkId, "LA",
					   wxDefaultPosition, wxSize(bwidth,bheight));
	_scaleLinkAllButton->SetFont(buttFont);

//  	_mashLinkAllButton = new wxButton(_mashPanel, FT_MashLinkId, "LA",
// 					   wxDefaultPosition, wxSize(bwidth,bheight));
// 	_mashLinkAllButton->SetFont(buttFont);

 	_gateLinkAllButton = new wxButton(_gatePanel, FT_GateLinkId, "LA",
					   wxDefaultPosition, wxSize(bwidth,bheight));
	_gateLinkAllButton->SetFont(buttFont);

	_feedbLinkAllButton = new wxButton(_feedbPanel, FT_FeedbackLinkId, "LA",
					   wxDefaultPosition, wxSize(bwidth,bheight));
	_feedbLinkAllButton->SetFont(buttFont);
	
	_freqBypassAllButton = new wxButton(_freqPanel, FT_FreqBypassId, "BA",
					    wxDefaultPosition, wxSize(bwidth,bheight));
	_freqBypassAllButton->SetFont(buttFont);

	_delayBypassAllButton = new wxButton(_delayPanel, FT_DelayBypassId, "BA",
					     wxDefaultPosition, wxSize(bwidth,bheight));
	_delayBypassAllButton->SetFont(buttFont);

	_scaleBypassAllButton = new wxButton(_scalePanel, FT_ScaleBypassId, "BA",
					     wxDefaultPosition, wxSize(bwidth,bheight));
	_scaleBypassAllButton->SetFont(buttFont);

// 	_mashBypassAllButton = new wxButton(_mashPanel, FT_MashBypassId, "BA",
// 					     wxDefaultPosition, wxSize(bwidth,bheight));
// 	_mashBypassAllButton->SetFont(buttFont);

	_gateBypassAllButton = new wxButton(_gatePanel, FT_GateBypassId, "BA",
					     wxDefaultPosition, wxSize(bwidth,bheight));
	_gateBypassAllButton->SetFont(buttFont);
	
	_feedbBypassAllButton = new wxButton(_feedbPanel, FT_FeedbackBypassId, "BA",
					     wxDefaultPosition, wxSize(bwidth,bheight));
	_feedbBypassAllButton->SetFont(buttFont);

	_inspecSpecTypeAllButton = new  wxButton(_inspecPanel, FT_InSpecTypeId, "SP",
					     wxDefaultPosition, wxSize(bwidth,bheight));
	_inspecSpecTypeAllButton->SetFont(buttFont);
	_inspecPlotSolidTypeAllButton = new  wxButton(_inspecPanel, FT_InSpecTypeId, "FP",
					     wxDefaultPosition, wxSize(bwidth,bheight));
	_inspecPlotSolidTypeAllButton->SetFont(buttFont);
	_inspecPlotLineTypeAllButton = new  wxButton(_inspecPanel, FT_InSpecTypeId, "LP",
					     wxDefaultPosition, wxSize(bwidth,bheight));
	_inspecPlotLineTypeAllButton->SetFont(buttFont);

	
	_outspecSpecTypeAllButton = new  wxButton(_outspecPanel, FT_OutSpecTypeId, "SP",
					     wxDefaultPosition, wxSize(bwidth,bheight));
	_outspecSpecTypeAllButton->SetFont(buttFont);
	_outspecPlotSolidTypeAllButton = new  wxButton(_outspecPanel, FT_OutSpecTypeId, "FP",
					     wxDefaultPosition, wxSize(bwidth,bheight));
	_outspecPlotSolidTypeAllButton->SetFont(buttFont);
	_outspecPlotLineTypeAllButton = new  wxButton(_outspecPanel, FT_OutSpecTypeId, "LP",
					     wxDefaultPosition, wxSize(bwidth,bheight));
	_outspecPlotLineTypeAllButton->SetFont(buttFont);


	_bypassAllButton  = new  wxButton(this, FT_BypassId, "Bypass All",
					     wxDefaultPosition, wxSize(labwidth,bheight+3));
	_bypassAllButton->SetFont(buttFont);

	_muteAllButton  = new  wxButton(this, FT_MuteId, "Mute All",
					     wxDefaultPosition, wxSize(labwidth,bheight+3));
	_muteAllButton->SetFont(buttFont);


	_linkMixButton  = new  wxButton(this, FT_MixLinkedButton, "Link Mix",
					     wxDefaultPosition, wxSize(labwidth,bheight+3));
	_linkMixButton->SetFont(buttFont);

	

	_pathCountChoice = new wxChoice(this, FT_PathCountChoice, wxDefaultPosition, wxSize(labwidth,-1));
	for (int i=0; i < FT_MAXPATHS; i++) {
		_pathCountChoice->Append(wxString::Format("%d chan", i+1), (void *) (i+1));
	}
	_pathCountChoice->SetStringSelection(wxString::Format("%d chan", _startpaths));
	
		


	// uppersizer
	tmpsizer = new wxBoxSizer(wxVERTICAL);
	tmpsizer->Add (_pathCountChoice, 0, wxALL, 1);
	tmpsizer->Add (_bypassAllButton, 1, wxALL, 1 );
	_uppersizer->Add(tmpsizer, 0, wxEXPAND);

	mainvsizer->Add(_uppersizer, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxEXPAND, 2);

	
	// input spectragram
	tmpsizer = new wxBoxSizer(wxVERTICAL);
	tmpsizer->Add (_inspecLabelButton, 0, wxALL|wxEXPAND , 0);
	tmpsizer2 = new wxBoxSizer(wxHORIZONTAL);
	tmpsizer2->Add (_inspecSpecTypeAllButton, 1, wxALL, 1);
	tmpsizer2->Add (_inspecPlotLineTypeAllButton, 1, wxALL, 1);
	tmpsizer2->Add (_inspecPlotSolidTypeAllButton, 1, wxALL, 1);
	tmpsizer->Add (tmpsizer2, 0, wxALL|wxEXPAND, 0);

	_inspecsizer->Add (tmpsizer, 0, wxALL|wxEXPAND, 0);

	_inspecPanel->SetAutoLayout(TRUE);
	_inspecsizer->Fit( _inspecPanel );  
	_inspecPanel->SetSizer(_inspecsizer);
	wxLayoutConstraints *inspecConst = new wxLayoutConstraints;
	inspecConst->top.SameAs (_rowPanel, wxTop, 2);
	inspecConst->left.SameAs (_rowPanel, wxLeft, 2);
	inspecConst->right.SameAs (_rowPanel, wxRight, 2);
	inspecConst->height.AsIs();
	_inspecSash->SetConstraints(inspecConst);
	//_inspecSash->Show(false);
	
	// freq plot
	_freqbuttsizer = new wxBoxSizer(wxVERTICAL);
	_freqbuttsizer->Add (_freqLabelButton, 0, wxALL|wxEXPAND , 0);
	tmpsizer2 = new wxBoxSizer(wxHORIZONTAL);
	tmpsizer2->Add (_freqBypassAllButton, 1, wxALL, 1);
	tmpsizer2->Add (_freqLinkAllButton, 1, wxALL, 1);
	_freqbuttsizer->Add (tmpsizer2, 0, wxALL|wxEXPAND, 0);
	_freqsizer->Add (_freqbuttsizer, 0, wxALL|wxEXPAND, 0);

	_freqPanel->SetAutoLayout(TRUE);
	_freqsizer->Fit( _freqPanel );  
	_freqPanel->SetSizer(_freqsizer);
	wxLayoutConstraints *freqConst = new wxLayoutConstraints;
	freqConst->left.SameAs (_rowPanel, wxLeft, 2);
	freqConst->right.SameAs (_rowPanel, wxRight, 2);
	freqConst->top.SameAs (_inspecSash, wxBottom,  2);
	freqConst->height.AsIs();
	_freqSash->SetConstraints(freqConst);


	
	// scale row
	tmpsizer = new wxBoxSizer(wxVERTICAL);
	tmpsizer->Add (_scaleLabelButton, 0, wxALL|wxEXPAND , 0);
	tmpsizer2 = new wxBoxSizer(wxHORIZONTAL);
	tmpsizer2->Add (_scaleBypassAllButton, 1, wxALL, 1);
	tmpsizer2->Add (_scaleLinkAllButton, 1, wxALL, 1);
	tmpsizer->Add (tmpsizer2, 0, wxALL|wxEXPAND, 0);
	_scalesizer->Add (tmpsizer, 0, wxALL|wxEXPAND, 0);

	_scalePanel->SetAutoLayout(TRUE);
	_scalesizer->Fit( _scalePanel );  
	_scalePanel->SetSizer(_scalesizer);
	wxLayoutConstraints *scaleConst  = new wxLayoutConstraints;
	scaleConst->left.SameAs (_rowPanel, wxLeft, 2);
	scaleConst->right.SameAs (_rowPanel, wxRight, 2);
	scaleConst->top.SameAs (_freqSash, wxBottom, 2);
	scaleConst->height.AsIs();
	_scaleSash->SetConstraints(scaleConst);

	

	
// 	// mash row
// 	tmpsizer = new wxBoxSizer(wxVERTICAL);
// 	tmpsizer->Add (_mashLabelButton, 0, wxALL|wxEXPAND , 0);
// 	tmpsizer2 = new wxBoxSizer(wxHORIZONTAL);
// 	tmpsizer2->Add (_mashBypassAllButton, 1, wxALL, 1);
// 	tmpsizer2->Add (_mashLinkAllButton, 1, wxALL, 1);
// 	tmpsizer->Add (tmpsizer2, 0, wxALL|wxEXPAND, 0);
// 	_mashsizer->Add (tmpsizer, 0, wxALL|wxEXPAND, 0);

// 	_mashPanel->SetAutoLayout(TRUE);
// 	_mashsizer->Fit( _mashPanel );  
// 	_mashPanel->SetSizer(_mashsizer);
// 	wxLayoutConstraints *mashConst  = new wxLayoutConstraints;
// 	mashConst->left.SameAs (_rowPanel, wxLeft, 2);
// 	mashConst->right.SameAs (_rowPanel, wxRight, 2);
// 	mashConst->top.SameAs (_scaleSash, wxBottom, 2);
// 	mashConst->height.AsIs();
// 	_mashSash->SetConstraints(mashConst);

	// gate row
	tmpsizer = new wxBoxSizer(wxVERTICAL);
	tmpsizer->Add (_gateLabelButton, 0, wxALL|wxEXPAND , 0);
	tmpsizer2 = new wxBoxSizer(wxHORIZONTAL);
	tmpsizer2->Add (_gateBypassAllButton, 1, wxALL, 1);
	tmpsizer2->Add (_gateLinkAllButton, 1, wxALL, 1);
	tmpsizer->Add (tmpsizer2, 0, wxALL|wxEXPAND, 0);
	_gatesizer->Add (tmpsizer, 0, wxALL|wxEXPAND, 0);

	_gatePanel->SetAutoLayout(TRUE);
	_gatesizer->Fit( _gatePanel );  
	_gatePanel->SetSizer(_gatesizer);
	wxLayoutConstraints *gateConst  = new wxLayoutConstraints;
	gateConst->left.SameAs (_rowPanel, wxLeft, 2);
	gateConst->right.SameAs (_rowPanel, wxRight, 2);
	gateConst->top.SameAs (_scaleSash, wxBottom, 2);
	gateConst->height.AsIs();
	_gateSash->SetConstraints(gateConst);


	
	// delay plot
	tmpsizer = new wxBoxSizer(wxVERTICAL);
	tmpsizer->Add (_delayLabelButton, 0, wxALL|wxEXPAND , 0);
	tmpsizer2 = new wxBoxSizer(wxHORIZONTAL);
	tmpsizer2->Add (_delayBypassAllButton, 1, wxALL, 1);
	tmpsizer2->Add (_delayLinkAllButton, 1, wxALL, 1);
	tmpsizer->Add (tmpsizer2, 0, wxALL|wxEXPAND, 0);
	_delaysizer->Add (tmpsizer, 0, wxALL|wxEXPAND, 0);

	_delayPanel->SetAutoLayout(TRUE);
	_delaysizer->Fit( _delayPanel );  
	_delayPanel->SetSizer(_delaysizer);
	wxLayoutConstraints *delayConst = new wxLayoutConstraints;
	delayConst->left.SameAs (_rowPanel, wxLeft, 2);
	delayConst->right.SameAs (_rowPanel, wxRight, 2);
	delayConst->top.SameAs (_gateSash, wxBottom, 2);
	delayConst->height.AsIs();
	_delaySash->SetConstraints(delayConst);

	
	// feedback plot
	tmpsizer = new wxBoxSizer(wxVERTICAL);
	tmpsizer->Add (_feedbLabelButton, 0, wxALL|wxEXPAND , 0);
	tmpsizer2 = new wxBoxSizer(wxHORIZONTAL);
	tmpsizer2->Add (_feedbBypassAllButton, 1, wxALL, 1);
	tmpsizer2->Add (_feedbLinkAllButton, 1, wxALL, 1);
	tmpsizer->Add (tmpsizer2, 0, wxALL|wxEXPAND, 0);
	_feedbsizer->Add (tmpsizer, 0, wxALL|wxEXPAND, 0);
	_feedbPanel->SetAutoLayout(TRUE);
	_feedbsizer->Fit( _feedbPanel );  
	_feedbPanel->SetSizer(_feedbsizer);
	wxLayoutConstraints *feedbConst = new wxLayoutConstraints;
	feedbConst->left.SameAs (_rowPanel, wxLeft, 2);
	feedbConst->right.SameAs (_rowPanel, wxRight, 2);
	feedbConst->top.SameAs (_delaySash, wxBottom, 2);
	feedbConst->height.AsIs();
	_feedbSash->SetConstraints(feedbConst);


	
	// output spectragram
	tmpsizer = new wxBoxSizer(wxVERTICAL);
	tmpsizer->Add (_outspecLabelButton, 0, wxALL|wxEXPAND , 0);
	tmpsizer2 = new wxBoxSizer(wxHORIZONTAL);
	tmpsizer2->Add (_outspecSpecTypeAllButton, 1, wxALL, 1);
	tmpsizer2->Add (_outspecPlotLineTypeAllButton, 1, wxALL, 1);
	tmpsizer2->Add (_outspecPlotSolidTypeAllButton, 1, wxALL, 1);
	tmpsizer->Add (tmpsizer2, 0, wxALL|wxEXPAND, 0);
	_outspecsizer->Add (tmpsizer, 0, wxALL|wxEXPAND, 0);

	_outspecPanel->SetAutoLayout(TRUE);
	_outspecsizer->Fit( _outspecPanel );  
	_outspecPanel->SetSizer(_outspecsizer);
	wxLayoutConstraints *outspecConst = new wxLayoutConstraints;
	outspecConst->left.SameAs (_rowPanel, wxLeft, 2);
	outspecConst->right.SameAs (_rowPanel, wxRight, 2);
	outspecConst->top.SameAs (_feedbSash, wxBottom, 2);
	outspecConst->height.AsIs();
	_outspecSash->SetConstraints(outspecConst);
	//_outspecSash->Show(false);


	_rowPanel->SetAutoLayout(TRUE);

	
	mainvsizer->Add (_rowPanel, 1,  wxLEFT|wxRIGHT|wxBOTTOM|wxEXPAND, 2);
	

	// lowersizer
	tmpsizer = new wxBoxSizer(wxVERTICAL);
	tmpsizer->Add (_muteAllButton, 1, wxALL, 1);
	tmpsizer->Add (_linkMixButton, 0, wxALL, 1);
	_lowersizer->Add(tmpsizer, 0, wxEXPAND);

	mainvsizer->Add(_lowersizer, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxEXPAND, 2);


	for (int i=0; i < _startpaths; i++)
	{
		// add to io support instance
		_processPath[i] = iosup->setProcessPathActive (i, true);

		if (!_processPath[i]) continue;

		FTspectralManip * manip = _processPath[i]->getSpectralManip();

		// set default freqbin
		_freqBinsChoice->SetStringSelection(wxString::Format("%d", manip->getFFTsize()/2));

		manip->setUpdateToken (_updateTokens[i]);
		
		createPathStuff (i);

		if (i > 0) {
			// link everything to first
			manip->getFreqFilter()->link (_processPath[0]->getSpectralManip()->getFreqFilter());
			manip->getScaleFilter()->link (_processPath[0]->getSpectralManip()->getScaleFilter());
			//manip->getMashPushFilter()->link (_processPath[0]->getSpectralManip()->getMashPushFilter());
			//manip->getMashLimitFilter()->link (_processPath[0]->getSpectralManip()->getMashLimitFilter());
			manip->getInverseGateFilter()->link (_processPath[0]->getSpectralManip()->getInverseGateFilter());
			manip->getGateFilter()->link (_processPath[0]->getSpectralManip()->getGateFilter());
			manip->getDelayFilter()->link (_processPath[0]->getSpectralManip()->getDelayFilter());
			manip->getFeedbackFilter()->link (_processPath[0]->getSpectralManip()->getFeedbackFilter());
		}
	}

	
	rebuildPresetCombo();

	
	mainsizer->Add(mainvsizer, 1, wxALL|wxEXPAND, 0);
	
	SetAutoLayout( TRUE );
	// set frame to minimum size
	mainsizer->Fit( this );  
	// don't allow frame to get smaller than what the sizers tell ye
	mainsizer->SetSizeHints( this );  
	SetSizer( mainsizer );

	rowpanelScrollSize();
	
	// set timer
	
	_eventTimer->Start(_updateMS, FALSE);
}


// void FTmainwin::OnSize(wxSizeEvent &event)
// {
// 	// rowpanelScrollSize();

// 	event.Skip();
// }

void FTmainwin::removePathStuff(int i)
{

	// deactivate it by the io thread
	FTioSupport::instance()->setProcessPathActive(i, false);
	_processPath[i] = 0;
	
	_uppersizer->Remove(i+1);
	_inspecsizer->Remove(i+1);
	_freqsizer->Remove(i+1);
	_scalesizer->Remove(i+1);
//	_mashsizer->Remove(i+1);
	_gatesizer->Remove(i+1);
	_delaysizer->Remove(i+1);
	_feedbsizer->Remove(i+1);
	_outspecsizer->Remove(i+1);
	_lowersizer->Remove(i+1);
	

	_upperPanels[i]->Destroy();
	_inspecPanels[i]->Destroy();
	_freqPanels[i]->Destroy();
	_scalePanels[i]->Destroy();
//	_mashPanels[i]->Destroy();
	_gatePanels[i]->Destroy();
	_delayPanels[i]->Destroy();
	_feedbPanels[i]->Destroy();
	_outspecPanels[i]->Destroy();
	_lowerPanels[i]->Destroy();

	
	
	_uppersizer->Layout();
	_inspecsizer->Layout();
	_freqsizer->Layout();
	_scalesizer->Layout();
//	_mashsizer->Layout();
	_gatesizer->Layout();
	_delaysizer->Layout();
	_feedbsizer->Layout();
	_outspecsizer->Layout();
	_lowersizer->Layout();

}

void FTmainwin::createPathStuff(int i)
{
	wxBoxSizer * buttsizer, *tmpsizer, *tmpsizer2;
	wxStaticText *stattext;

	int bwidth = 20;
	
	wxFont titleFont(10, wxDEFAULT, wxNORMAL, wxBOLD);
	wxFont titleAltFont(10, wxDEFAULT, wxSLANT, wxBOLD);
	wxFont buttFont(10, wxDEFAULT, wxNORMAL, wxNORMAL);
	
	FTspectralManip * manip = _processPath[i]->getSpectralManip();

	_upperPanels[i] = new wxPanel(this, -1);
	_lowerPanels[i] = new wxPanel(this, -1);

	_inspecPanels[i] = new wxPanel(_inspecPanel, -1);
	_freqPanels[i] = new wxPanel(_freqPanel, -1);
	_scalePanels[i] = new wxPanel(_scalePanel, -1);
//	_mashPanels[i] = new wxPanel(_mashPanel, -1);
	_gatePanels[i] = new wxPanel(_gatePanel, -1);
	_delayPanels[i] = new wxPanel(_delayPanel, -1);
	_feedbPanels[i] = new wxPanel(_feedbPanel, -1);
	_outspecPanels[i] = new wxPanel(_outspecPanel, -1);

	
	// plots and active graphs
	_inputSpectragram[i] = new FTspectragram(this, _inspecPanels[i], -1);
	_outputSpectragram[i] = new FTspectragram(this, _outspecPanels[i], -1);
			
	_freqGraph[i] = new FTactiveBarGraph(this, _freqPanels[i], -1);
	_freqGraph[i]->setSpectrumModifier(manip->getFreqFilter());
	//_freqGraph[i]->setXscale(XSCALE_LOGA);
	
	_scaleGraph[i] = new FTactiveBarGraph(this, _scalePanels[i], -1);
	_scaleGraph[i]->setSpectrumModifier(manip->getScaleFilter());
	manip->setBypassScaleFilter ( true );
	
	_gateGraph[i] = new FTactiveBarGraph(this, _gatePanels[i], -1);
	_gateGraph[i]->setSpectrumModifier(manip->getInverseGateFilter());
	_gateGraph[i]->setTopSpectrumModifier(manip->getGateFilter());
	manip->setBypassGateFilter ( true );
	manip->setBypassInverseGateFilter ( true );

// 	_mashGraph[i] = new FTactiveBarGraph(this, _mashPanels[i], -1);
// 	_mashGraph[i]->setSpectrumModifier(manip->getMashLimitFilter());
// 	_mashGraph[i]->setTopSpectrumModifier(manip->getMashPushFilter());
 	manip->setBypassMashLimitFilter ( true );
 	manip->setBypassMashPushFilter ( true );

	
	_delayGraph[i] = new FTactiveBarGraph(this, _delayPanels[i], -1);
	_delayGraph[i]->setSpectrumModifier(manip->getDelayFilter());
	
	_feedbackGraph[i] = new FTactiveBarGraph(this, _feedbPanels[i], -1);
	_feedbackGraph[i]->setSpectrumModifier(manip->getFeedbackFilter());
	
	// I/O buttons
	
	_inputButton[i] = new wxButton(_upperPanels[i], (int) FT_InputButtonId, "No Input", wxDefaultPosition, wxSize(-1,-1));
	_inputButton[i]->SetFont(buttFont);
	
	_outputButton[i] = new wxButton(_lowerPanels[i], (int) FT_OutputButtonId, "No Output", wxDefaultPosition, wxSize(-1,-1));
	_outputButton[i]->SetFont(buttFont);
	
	_bypassButton[i] = new wxButton(_upperPanels[i], (int) FT_BypassId, "Bypass", wxDefaultPosition, wxSize(-1,-1));
	_bypassButton[i]->SetFont(buttFont);
	_muteButton[i] = new wxButton(_lowerPanels[i], (int) FT_MuteId, "Mute", wxDefaultPosition, wxSize(-1,-1));
	_muteButton[i]->SetFont(buttFont);
	
	// input area
	{
		wxStaticBox *box = new wxStaticBox(_upperPanels[i], -1, wxString::Format("Input %d", i+1));
		tmpsizer = new wxStaticBoxSizer (box, wxVERTICAL);
		tmpsizer->Add (_inputButton[i], 0, wxBOTTOM|wxEXPAND, 1);
		
		tmpsizer2 = new wxBoxSizer (wxHORIZONTAL);
		tmpsizer2->Add (_bypassButton[i], 1, wxRIGHT, 3);
		
		stattext = new wxStaticText(_upperPanels[i], -1, "Gain (dB)", wxDefaultPosition, wxDefaultSize);
		stattext->SetFont(buttFont);
		tmpsizer2->Add (stattext, 0, wxALL|wxALIGN_CENTRE_VERTICAL, 0);
		//_gainSlider[i] = new wxSlider(this, FT_GainSlider, 0, -70, 10);
		//tmpsizer2->Add (_gainSlider[i], 1, wxALL|wxALIGN_CENTRE_VERTICAL, 1);
		_gainSpinCtrl[i] = new wxSpinCtrl(_upperPanels[i], FT_GainSpin, "0", wxDefaultPosition, wxSize(65,-1),
						  wxSP_ARROW_KEYS, -70, 10, 0);
		tmpsizer2->Add (_gainSpinCtrl[i], 0, wxLEFT, 4);
		
		tmpsizer->Add (tmpsizer2, 0, wxTOP|wxEXPAND, 1);

		_upperPanels[i]->SetAutoLayout(TRUE);
		tmpsizer->Fit( _upperPanels[i] );  
		_upperPanels[i]->SetSizer(tmpsizer);
		
		_uppersizer->Insert (i+1, _upperPanels[i], 1, wxLEFT, 3);
	}

	// input spec
	{		
		buttsizer = new wxBoxSizer(wxVERTICAL);
	        _inspecSpecTypeButton[i] = new wxButton(_inspecPanels[i], FT_InSpecTypeId, "SP", wxDefaultPosition, wxSize(bwidth,-1));
		_inspecSpecTypeButton[i]->SetFont(buttFont);
		buttsizer->Add ( _inspecSpecTypeButton[i], 1, 0,0);

	        _inspecPlotLineTypeButton[i] = new wxButton(_inspecPanels[i], FT_InSpecTypeId, "LP",
							    wxDefaultPosition, wxSize(bwidth,-1));
		_inspecPlotLineTypeButton[i]->SetFont(buttFont);
		buttsizer->Add ( _inspecPlotLineTypeButton[i], 1, 0,0);

	        _inspecPlotSolidTypeButton[i] = new wxButton(_inspecPanels[i], FT_InSpecTypeId, "FP",
							     wxDefaultPosition, wxSize(bwidth,-1));
		_inspecPlotSolidTypeButton[i]->SetFont(buttFont);
		buttsizer->Add ( _inspecPlotSolidTypeButton[i], 1, 0,0);

		tmpsizer = new wxBoxSizer(wxHORIZONTAL);
		tmpsizer->Add ( buttsizer, 0, wxALL|wxEXPAND, 0);
		tmpsizer->Add ( _inputSpectragram[i],  1, wxALL|wxEXPAND, 0);


		_inspecPanels[i]->SetAutoLayout(TRUE);
		tmpsizer->Fit( _inspecPanels[i] );  
		_inspecPanels[i]->SetSizer(tmpsizer);
		
		_inspecsizer->Insert (i+1, _inspecPanels[i],  1, wxLEFT|wxEXPAND, 4);
	}

	// freq
	{		
		buttsizer = new wxBoxSizer(wxVERTICAL);
		buttsizer->Add ( _freqBypassButton[i] = new wxButton(_freqPanels[i], FT_FreqBypassId, "B",
								     wxDefaultPosition, wxSize(bwidth,-1)), 1, 0,0);
		_freqBypassButton[i]->SetFont(buttFont);
		buttsizer->Add ( _freqLinkButton[i] = new wxButton(_freqPanels[i], FT_FreqLinkId, "L",
								   wxDefaultPosition, wxSize(bwidth,-1)), 1, 0,0);
		_freqLinkButton[i]->SetFont(buttFont);

		tmpsizer = new wxBoxSizer(wxHORIZONTAL);
		tmpsizer->Add ( buttsizer, 0, wxALL|wxEXPAND, 0);
		tmpsizer->Add ( _freqGraph[i],  1, wxALL|wxEXPAND, 0);


		_freqPanels[i]->SetAutoLayout(TRUE);
		tmpsizer->Fit( _freqPanels[i] );  
		_freqPanels[i]->SetSizer(tmpsizer);
		
		_freqsizer->Insert (i+1, _freqPanels[i],  1, wxLEFT|wxEXPAND, 4);
	}

	
	// scale
	{		
		buttsizer = new wxBoxSizer(wxVERTICAL);
		buttsizer->Add ( _scaleBypassButton[i] = new wxButton(_scalePanels[i], FT_ScaleBypassId, "B",
								     wxDefaultPosition, wxSize(bwidth,-1)), 1, 0,0);
		_scaleBypassButton[i]->SetFont(buttFont);
		buttsizer->Add ( _scaleLinkButton[i] = new wxButton(_scalePanels[i], FT_ScaleLinkId, "L",
								   wxDefaultPosition, wxSize(bwidth,-1)), 1, 0,0);
		_scaleLinkButton[i]->SetFont(buttFont);

		tmpsizer = new wxBoxSizer(wxHORIZONTAL);
		tmpsizer->Add ( buttsizer, 0, wxALL|wxEXPAND, 0);
		tmpsizer->Add ( _scaleGraph[i],  1, wxALL|wxEXPAND, 0);

		_scalePanels[i]->SetAutoLayout(TRUE);
		tmpsizer->Fit( _scalePanels[i] );  
		_scalePanels[i]->SetSizer(tmpsizer);

		_scalesizer->Insert (i+1, _scalePanels[i],  1, wxLEFT|wxEXPAND, 4);
	}


	
// 	// mash
// 	{		
// 		buttsizer = new wxBoxSizer(wxVERTICAL);
// 		buttsizer->Add ( _mashBypassButton[i] = new wxButton(_mashPanels[i], FT_MashBypassId, "B",
// 								     wxDefaultPosition, wxSize(bwidth,-1)), 1, 0,0);
// 		_mashBypassButton[i]->SetFont(buttFont);
// 		buttsizer->Add ( _mashLinkButton[i] = new wxButton(_mashPanels[i], FT_MashLinkId, "L",
// 								   wxDefaultPosition, wxSize(bwidth,-1)), 1, 0,0);
// 		_mashLinkButton[i]->SetFont(buttFont);

// 		tmpsizer = new wxBoxSizer(wxHORIZONTAL);
// 		tmpsizer->Add ( buttsizer, 0, wxALL|wxEXPAND, 0);
// 		tmpsizer->Add ( _mashGraph[i],  1, wxALL|wxEXPAND, 0);

// 		_mashPanels[i]->SetAutoLayout(TRUE);
// 		tmpsizer->Fit( _mashPanels[i] );  
// 		_mashPanels[i]->SetSizer(tmpsizer);

// 		_mashsizer->Insert (i+1, _mashPanels[i],  1, wxLEFT|wxEXPAND, 4);
// 	}

	// gate
	{		
		buttsizer = new wxBoxSizer(wxVERTICAL);
		buttsizer->Add ( _gateBypassButton[i] = new wxButton(_gatePanels[i], FT_GateBypassId, "B",
								     wxDefaultPosition, wxSize(bwidth,-1)), 1, 0,0);
		_gateBypassButton[i]->SetFont(buttFont);
		buttsizer->Add ( _gateLinkButton[i] = new wxButton(_gatePanels[i], FT_GateLinkId, "L",
								   wxDefaultPosition, wxSize(bwidth,-1)), 1, 0,0);
		_gateLinkButton[i]->SetFont(buttFont);

		tmpsizer = new wxBoxSizer(wxHORIZONTAL);
		tmpsizer->Add ( buttsizer, 0, wxALL|wxEXPAND, 0);
		tmpsizer->Add ( _gateGraph[i],  1, wxALL|wxEXPAND, 0);

		_gatePanels[i]->SetAutoLayout(TRUE);
		tmpsizer->Fit( _gatePanels[i] );  
		_gatePanels[i]->SetSizer(tmpsizer);

		_gatesizer->Insert (i+1, _gatePanels[i],  1, wxLEFT|wxEXPAND, 4);
	}


	
	// delay
	{		
			
		buttsizer = new wxBoxSizer(wxVERTICAL);
		buttsizer->Add ( _delayBypassButton[i] = new wxButton(_delayPanels[i], FT_DelayBypassId, "B",
								      wxDefaultPosition, wxSize(bwidth,-1)), 1, 0,0);
		_delayBypassButton[i]->SetFont (buttFont);
		buttsizer->Add ( _delayLinkButton[i] = new wxButton(_delayPanels[i], FT_DelayLinkId, "L",
					      wxDefaultPosition, wxSize(bwidth,-1)), 1, 0,0);
		_delayLinkButton[i]->SetFont (buttFont);

		tmpsizer = new wxBoxSizer(wxHORIZONTAL);
		tmpsizer->Add ( buttsizer, 0, wxALL|wxEXPAND, 0);
		tmpsizer->Add ( _delayGraph[i],  1, wxALL|wxEXPAND, 0);

		_delayPanels[i]->SetAutoLayout(TRUE);
		tmpsizer->Fit( _delayPanels[i] );  
		_delayPanels[i]->SetSizer(tmpsizer);
		
		_delaysizer->Insert (i+1,  _delayPanels[i],  1, wxLEFT|wxEXPAND, 4);
	}

	// feedback
	{		

		buttsizer = new wxBoxSizer(wxVERTICAL);
		buttsizer->Add ( _feedbBypassButton[i] = new wxButton(_feedbPanels[i], FT_FeedbackBypassId, "B",
								   wxDefaultPosition, wxSize(bwidth,-1)), 1, 0,0);
		_feedbBypassButton[i]->SetFont(buttFont);
		buttsizer->Add ( _feedbLinkButton[i] = new wxButton(_feedbPanels[i], FT_FeedbackLinkId, "L",
								    wxDefaultPosition, wxSize(bwidth,-1)), 1, 0,0);
		_feedbLinkButton[i]->SetFont(buttFont);

		tmpsizer = new wxBoxSizer(wxHORIZONTAL);
		tmpsizer->Add ( buttsizer, 0, wxALL|wxEXPAND, 0);
		tmpsizer->Add ( _feedbackGraph[i],  1, wxALL|wxEXPAND, 0);

		_feedbPanels[i]->SetAutoLayout(TRUE);
		tmpsizer->Fit( _feedbPanels[i] );  
		_feedbPanels[i]->SetSizer(tmpsizer);

		_feedbsizer->Insert (i+1, _feedbPanels[i],  1, wxLEFT|wxEXPAND, 4);
	}

	// output spec
	{				
		buttsizer = new wxBoxSizer(wxVERTICAL);
		_outspecSpecTypeButton[i] = new wxButton(_outspecPanels[i], FT_OutSpecTypeId, "SP", wxDefaultPosition, wxSize(bwidth,-1));
		_outspecSpecTypeButton[i]->SetFont(buttFont);
		buttsizer->Add ( _outspecSpecTypeButton[i], 1, 0,0);
		_outspecPlotLineTypeButton[i] = new wxButton(_outspecPanels[i], FT_OutSpecTypeId, "LP",
							 wxDefaultPosition, wxSize(bwidth,-1));
		_outspecPlotLineTypeButton[i]->SetFont(buttFont);
		buttsizer->Add ( _outspecPlotLineTypeButton[i], 1, 0,0);
		_outspecPlotSolidTypeButton[i] = new wxButton(_outspecPanels[i], FT_OutSpecTypeId, "FP",
							      wxDefaultPosition, wxSize(bwidth,-1));
		_outspecPlotSolidTypeButton[i]->SetFont(buttFont);
		buttsizer->Add ( _outspecPlotSolidTypeButton[i], 1, 0,0);

		tmpsizer = new wxBoxSizer(wxHORIZONTAL);
		tmpsizer->Add ( buttsizer, 0, wxALL|wxEXPAND, 0);
		tmpsizer->Add ( _outputSpectragram[i],  1, wxALL|wxEXPAND, 0);

		_outspecPanels[i]->SetAutoLayout(TRUE);
		tmpsizer->Fit( _outspecPanels[i] );  
		_outspecPanels[i]->SetSizer(tmpsizer);

		_outspecsizer->Insert (i+1, _outspecPanels[i],  1, wxLEFT|wxEXPAND, 4);
	}


	// output stuff
	{		
		wxStaticBox *box = new wxStaticBox(_lowerPanels[i], -1, wxString::Format("Output %d", i+1));
		tmpsizer = new wxStaticBoxSizer (box, wxVERTICAL);

		tmpsizer2 = new wxBoxSizer (wxHORIZONTAL);
		tmpsizer2->Add (_muteButton[i], 1, wxRIGHT, 5);
		stattext = new wxStaticText(_lowerPanels[i], -1, "Dry", wxDefaultPosition, wxDefaultSize);
		stattext->SetFont(buttFont);
		tmpsizer2->Add (stattext, 0, wxALL|wxALIGN_CENTRE_VERTICAL, 1);
		_mixSlider[i] = new wxSlider(_lowerPanels[i], FT_MixSlider, 1000, 0, 1000);
		tmpsizer2->Add (_mixSlider[i], 2, wxALL|wxALIGN_CENTRE_VERTICAL, 1);
		stattext = new wxStaticText(_lowerPanels[i], -1, "Wet", wxDefaultPosition, wxDefaultSize);
		stattext->SetFont(buttFont);
		tmpsizer2->Add (stattext, 0, wxALL|wxALIGN_CENTRE_VERTICAL, 1);

		tmpsizer->Add (tmpsizer2, 0, wxBOTTOM|wxEXPAND, 1);

		tmpsizer->Add(_outputButton[i], 0, wxEXPAND|wxTOP, 1);

		_lowerPanels[i]->SetAutoLayout(TRUE);
		tmpsizer->Fit( _lowerPanels[i] );  
		_lowerPanels[i]->SetSizer(tmpsizer);
		
		_lowersizer->Insert (i+1, _lowerPanels[i], 1, wxLEFT, 3);
		
	}
	
	
	manip->setUpdateToken (_updateTokens[i]);
		
	
}



void FTmainwin::rowpanelScrollSize()
{
	//int mw, mh;
	//int w, h;
	//int vw, vh;

	//GetClientSize(&mw, &mh);	
	//_rowPanel->GetClientSize(&w, &h);
	//_rowPanel->GetVirtualSize(&vw, &vh);

	// calculate virtual full height of rowpanel
	int realh = 0;

	for (int i=0; i < _rowCount; i++) {
		realh += _rowItems[i]->GetSize().GetHeight() + 2;
	}
	
	//printf ("rowpanel size %d %d   %d %d  realh %d\n", w, h, vw, vh, realh);

	_rowPanel->SetScrollbars(1, 1, 0, realh);
	

}

void FTmainwin::updateDisplay()
{
	FTioSupport *iosup = FTioSupport::instance();
	const char ** portnames = 0;

	bool freqbyp=true, scalebyp=true,  delaybyp=true, feedbbyp=true, gatebyp=true;
	bool freqlink=true, scalelink=true, delaylink=true, feedblink=true, gatelink=true;
	bool inspec=true, inplotline=true, inplotsolid=true;
	bool outspec=true, outplotline=true, outplotsolid=true;
	bool bypassed=true, muted=true, linked, active;
	
	for (int i=0; i<_pathCount; i++)
	{		
		if (!_processPath[i]) continue;
			
		// update port button labels
		portnames = iosup->getConnectedInputPorts(i);
		if (portnames) {
			_inputButton[i]->SetLabel (portnames[0]);
			free(portnames);
		}
		else {
			_inputButton[i]->SetLabel ("No Input");
		}
		
		portnames = iosup->getConnectedOutputPorts(i);
		if (portnames) {
			_outputButton[i]->SetLabel (portnames[0]);
			free(portnames);
		}
		else {
			_outputButton[i]->SetLabel ("No Output");
		}

		// update bypass and link buttons
		FTspectralManip *manip =  _processPath[i]->getSpectralManip();

		_freqBypassButton[i]->SetBackgroundColour ((manip->getBypassFreqFilter() ? (_activeBg) : (_defaultBg)));
		if (freqbyp && !manip->getBypassFreqFilter()) freqbyp = false;
		
		_delayBypassButton[i]->SetBackgroundColour ((manip->getBypassDelayFilter() ? (_activeBg) : (_defaultBg)));
		if (delaybyp && !manip->getBypassDelayFilter()) delaybyp = false;

		_feedbBypassButton[i]->SetBackgroundColour ((manip->getBypassFeedbackFilter() ? (_activeBg) : (_defaultBg)));
		if (feedbbyp && !manip->getBypassFeedbackFilter()) feedbbyp = false;

		_scaleBypassButton[i]->SetBackgroundColour ((manip->getBypassScaleFilter() ? (_activeBg) : (_defaultBg)));
		if (scalebyp && !manip->getBypassScaleFilter()) scalebyp = false;

// 		_mashBypassButton[i]->SetBackgroundColour ((manip->getBypassMashLimitFilter() ? (_activeBg) : (_defaultBg)));
// 		if (mashbyp && !manip->getBypassMashLimitFilter()) mashbyp = false;

		_gateBypassButton[i]->SetBackgroundColour ((manip->getBypassGateFilter() ? (_activeBg) : (_defaultBg)));
		if (gatebyp && !manip->getBypassGateFilter()) gatebyp = false;
		

		linked = ( manip->getFreqFilter()->getLink()!=0 || manip->getFreqFilter()->getLinkedFrom().GetCount() > 0);
		_freqLinkButton[i]->SetBackgroundColour ((linked ? (_activeBg) : (_defaultBg)));
		if (freqlink && i!=0 && !linked) freqlink = false;

		linked = ( manip->getDelayFilter()->getLink()!=0 || manip->getDelayFilter()->getLinkedFrom().GetCount() > 0);
		_delayLinkButton[i]->SetBackgroundColour ((linked ? (_activeBg) : (_defaultBg)));
		if (delaylink && i!=0 && !linked) delaylink = false;

		linked = ( manip->getFeedbackFilter()->getLink()!=0 || manip->getFeedbackFilter()->getLinkedFrom().GetCount() > 0);
		_feedbLinkButton[i]->SetBackgroundColour ((linked ? (_activeBg) : (_defaultBg)));
		if (feedblink && i!=0 && !linked) feedblink = false;

		linked = ( manip->getScaleFilter()->getLink()!=0 || manip->getScaleFilter()->getLinkedFrom().GetCount() > 0);
		_scaleLinkButton[i]->SetBackgroundColour ((linked ? (_activeBg) : (_defaultBg)));
		if (scalelink && i!=0 && !linked) scalelink = false;

// 		_mashLinkButton[i]->SetBackgroundColour ((manip->getMashLimitFilter()->getLink() ? (_activeBg) : (_defaultBg)));
// 		if (mashlink && i!=0 && !manip->getMashLimitFilter()->getLink()) mashlink = false;

		linked = ( manip->getGateFilter()->getLink()!=0 || manip->getGateFilter()->getLinkedFrom().GetCount() > 0);
		_gateLinkButton[i]->SetBackgroundColour ((linked ? (_activeBg) : (_defaultBg)));
		if (gatelink && i!=0 && !linked) gatelink = false;


		_bypassButton[i]->SetBackgroundColour(manip->getBypassed() ? (_activeBg) : (_defaultBg));
		if (!manip->getBypassed()) bypassed = false;
		
		_muteButton[i]->SetBackgroundColour(manip->getMuted() ? (_activeBg) : (_defaultBg));
		if (!manip->getMuted()) muted = false;


		// sliders and stuff
		_gainSpinCtrl[i]->SetValue ((int) (20 * FTutils::fast_log10 ( manip->getInputGain() )));
		_mixSlider[i]->SetValue ((int) ( manip->getMixRatio() * 1000 ));

		// plots
		active = _inputSpectragram[i]->getPlotType()==FTspectragram::SPECTRAGRAM;
		_inspecSpecTypeButton[i]->SetBackgroundColour ((active ? (_activeBg) : (_defaultBg)));
		if (inspec && !active) inspec = false;

		active = _inputSpectragram[i]->getPlotType()==FTspectragram::AMPFREQ_SOLID;
		_inspecPlotSolidTypeButton[i]->SetBackgroundColour ((active ? (_activeBg) : (_defaultBg)));
		if (inplotsolid && !active) inplotsolid = false;

		active = _inputSpectragram[i]->getPlotType()==FTspectragram::AMPFREQ_LINES;
		_inspecPlotLineTypeButton[i]->SetBackgroundColour ((active ? (_activeBg) : (_defaultBg)));
		if (inplotline && !active) inplotline = false;

		active = _outputSpectragram[i]->getPlotType()==FTspectragram::SPECTRAGRAM;
		_outspecSpecTypeButton[i]->SetBackgroundColour ((active ? (_activeBg) : (_defaultBg)));
		if (outspec && !active) outspec = false;

		active = _outputSpectragram[i]->getPlotType()==FTspectragram::AMPFREQ_SOLID;
		_outspecPlotSolidTypeButton[i]->SetBackgroundColour ((active ? (_activeBg) : (_defaultBg)));
		if (outplotsolid && !active) outplotsolid = false;

		active = _outputSpectragram[i]->getPlotType()==FTspectragram::AMPFREQ_LINES;
		_outspecPlotLineTypeButton[i]->SetBackgroundColour ((active ? (_activeBg) : (_defaultBg)));
		if (outplotline && !active) outplotline = false;
		
	}

	
	_freqBypassAllButton->SetBackgroundColour ((freqbyp ? (_activeBg) : (_defaultBg)));
	_delayBypassAllButton->SetBackgroundColour ((delaybyp ? (_activeBg) : (_defaultBg)));
	_feedbBypassAllButton->SetBackgroundColour ((feedbbyp ? (_activeBg) : (_defaultBg)));
	_scaleBypassAllButton->SetBackgroundColour ((scalebyp ? (_activeBg) : (_defaultBg)));
//	_mashBypassAllButton->SetBackgroundColour ((mashbyp ? (_activeBg) : (_defaultBg)));
	_gateBypassAllButton->SetBackgroundColour ((gatebyp ? (_activeBg) : (_defaultBg)));

	_freqLinkAllButton->SetBackgroundColour ((freqlink ? (_activeBg) : (_defaultBg)));
	_delayLinkAllButton->SetBackgroundColour ((delaylink ? (_activeBg) : (_defaultBg)));
	_feedbLinkAllButton->SetBackgroundColour ((feedblink ? (_activeBg) : (_defaultBg)));
	_scaleLinkAllButton->SetBackgroundColour ((scalelink ? (_activeBg) : (_defaultBg)));
//	_mashLinkAllButton->SetBackgroundColour ((mashlink ? (_activeBg) : (_defaultBg)));
	_gateLinkAllButton->SetBackgroundColour ((gatelink ? (_activeBg) : (_defaultBg)));

	_bypassAllButton->SetBackgroundColour ((bypassed ? (_activeBg) : (_defaultBg)));
	_muteAllButton->SetBackgroundColour ((muted ? (_activeBg) : (_defaultBg)));
	_linkMixButton->SetBackgroundColour ((_linkedMix ? (_activeBg) : (_defaultBg)));

	_inspecSpecTypeAllButton->SetBackgroundColour ((inspec ? (_activeBg) : (_defaultBg)));
	_inspecPlotLineTypeAllButton->SetBackgroundColour ((inplotline ? (_activeBg) : (_defaultBg)));
	_inspecPlotSolidTypeAllButton->SetBackgroundColour ((inplotsolid ? (_activeBg) : (_defaultBg)));
	_outspecSpecTypeAllButton->SetBackgroundColour ((outspec ? (_activeBg) : (_defaultBg)));
	_outspecPlotLineTypeAllButton->SetBackgroundColour ((outplotline ? (_activeBg) : (_defaultBg)));
	_outspecPlotSolidTypeAllButton->SetBackgroundColour ((outplotsolid ? (_activeBg) : (_defaultBg)));

	
	_pathCountChoice->SetSelection(_pathCount - 1);

	if (_processPath[0]->getSpectralManip()->getUpdateSpeed() == FTspectralManip::SPEED_TURTLE)
		_plotSpeedChoice->SetSelection(0);
	else if (_processPath[0]->getSpectralManip()->getUpdateSpeed() == FTspectralManip::SPEED_SLOW)
		_plotSpeedChoice->SetSelection(1);
	else if (_processPath[0]->getSpectralManip()->getUpdateSpeed() == FTspectralManip::SPEED_MED)
		_plotSpeedChoice->SetSelection(2);
	else if (_processPath[0]->getSpectralManip()->getUpdateSpeed() == FTspectralManip::SPEED_FAST)
		_plotSpeedChoice->SetSelection(3);

	_windowingChoice->SetSelection(_processPath[0]->getSpectralManip()->getWindowing());

	const int * fftbins = FTspectralManip::getFFTSizes();
	for (int i=0; i < FTspectralManip::getFFTSizeCount(); i++) {
		if (fftbins[i] == _processPath[0]->getSpectralManip()->getFFTsize()) {
			_freqBinsChoice->SetSelection(i);
			break;
		}
	}

	// hack
	for (int i=0; i < 5; i++) {
		if (_processPath[0]->getSpectralManip()->getOversamp() == 1<<i) {
			_overlapChoice->SetSelection(i);
			break;
		}
	}

	_ioNameText->SetValue (iosup->getName());
	
	
	// reset timer just in case
	_updateMS = 10; // start small
	_eventTimer->Stop();
	_eventTimer->Start(_updateMS, FALSE);
	
}


// event handlers


void FTmainwin::handleInputButton(wxCommandEvent &event)
{
   wxObject *source = event.GetEventObject();

   wxString label("No Input");
   
   for (int i=0; i < _pathCount; i++) {
	   if (!_processPath[i]) continue;
	   
	   if (source == _inputButton[i]) {
		   FTportSelectionDialog *dial = new FTportSelectionDialog(this, wxNewId(), i,
									   FTportSelectionDialog::INPUT, "Input Port Selection");
		   dial->update();
		   dial->SetSize(wxSize(190,190));
		   dial->CentreOnParent();
		   if (dial->ShowModal() == wxID_OK) {
			   const char ** pnames = dial->getSelectedPorts();
			   FTioSupport::instance()->disconnectPathInput(i, NULL); // disconnect all
			   if (pnames) {
				   for (int j=0; pnames[j]; j++) {
					   FTioSupport::instance()->connectPathInput(i, pnames[j]);
				   }
				   free(pnames);
			   }
		   }
		   dial->Close();
	   }
   }

   updateDisplay();
   
}

void FTmainwin::handleOutputButton(wxCommandEvent &event)
{
   wxObject *source = event.GetEventObject();

   wxString label("No Output");
   
   for (int i=0; i < _pathCount; i++) {
	   if (!_processPath[i]) continue;

	   if (source == _outputButton[i]) {
		   FTportSelectionDialog *dial = new FTportSelectionDialog(this, wxNewId(), i,
									   FTportSelectionDialog::OUTPUT, "Output Port Selection");
		   dial->update();
		   dial->SetSize(wxSize(190,190));
		   dial->CentreOnParent();
		   if (dial->ShowModal() == wxID_OK) {
			   const char ** pnames = dial->getSelectedPorts();
			   FTioSupport::instance()->disconnectPathOutput(i, NULL); // disconnect all
			   if (pnames) {
				   for (int j=0; pnames[j]; j++) {
					   FTioSupport::instance()->connectPathOutput(i, pnames[j]);
				   }
				   free(pnames);
			   }
		   }

		   dial->Close();
	   }
   }

   updateDisplay();

}



void FTmainwin::handleBypassButtons (wxCommandEvent &event)
{
   wxObject *source = event.GetEventObject();

   for (int i=0; i < _pathCount; i++) {
	   if (!_processPath[i]) continue;

	   FTspectralManip *manip =  _processPath[i]->getSpectralManip();

	   if (source == _freqBypassButton[i]) {
		   manip->setBypassFreqFilter ( ! manip->getBypassFreqFilter());
		   break;
	   }
	   else if (source == _delayBypassButton[i]) {
		   manip->setBypassDelayFilter ( ! manip->getBypassDelayFilter());
		   break;
	   }
	   else if (source == _feedbBypassButton[i]) {
		   manip->setBypassFeedbackFilter ( ! manip->getBypassFeedbackFilter());
		   break;
	   }
	   else if (source == _scaleBypassButton[i]) {
		   manip->setBypassScaleFilter ( ! manip->getBypassScaleFilter());
		   break;
	   }
// 	   else if (source == _mashBypassButton[i]) {
// 		   manip->setBypassMashLimitFilter ( ! manip->getBypassMashLimitFilter());
// 		   manip->setBypassMashPushFilter ( ! manip->getBypassMashPushFilter());
// 		   break;
// 	   }
	   else if (source == _gateBypassButton[i]) {
		   manip->setBypassInverseGateFilter ( ! manip->getBypassInverseGateFilter());
		   manip->setBypassGateFilter ( ! manip->getBypassGateFilter());
		   break;
	   }

	   
	   // master bypass buttons
	   else if (source == _scaleBypassAllButton) {
		   manip->setBypassScaleFilter ( _scaleBypassAllButton->GetBackgroundColour() != _activeBg);
	   }
// 	   else if (source == _mashBypassAllButton) {
// 		   manip->setBypassMashLimitFilter ( _mashBypassAllButton->GetBackgroundColour() != _activeBg);
// 		   manip->setBypassMashPushFilter ( _mashBypassAllButton->GetBackgroundColour() != _activeBg);
// 	   }
	   else if (source == _gateBypassAllButton) {
		   manip->setBypassInverseGateFilter ( _gateBypassAllButton->GetBackgroundColour() != _activeBg);
		   manip->setBypassGateFilter ( _gateBypassAllButton->GetBackgroundColour() != _activeBg);
	   }
	   else if (source == _freqBypassAllButton) {
		   manip->setBypassFreqFilter ( _freqBypassAllButton->GetBackgroundColour() != _activeBg);
	   }
	   else if (source == _delayBypassAllButton) {
		   manip->setBypassDelayFilter ( _delayBypassAllButton->GetBackgroundColour() != _activeBg);
	   }
	   else if (source == _feedbBypassAllButton) {
		   manip->setBypassFeedbackFilter ( _feedbBypassAllButton->GetBackgroundColour() != _activeBg);
	   }

	   // path bypass
	   else if (source == _bypassAllButton) {
		   manip->setBypassed( _bypassAllButton->GetBackgroundColour() != _activeBg);
		   if (manip->getBypassed()) {
			   _updateTokens[i]->setIgnore(true);
		   }
		   else {
			   _updateTokens[i]->setIgnore(false);
		   }
		   // do not break
	   }
	   else if (source == _bypassButton[i]) {
		   manip->setBypassed( !manip->getBypassed());
		   if (manip->getBypassed()) {
			   _updateTokens[i]->setIgnore(true);
		   }
		   else {
			   _updateTokens[i]->setIgnore(false);
		   }

		   break;
	   }

	   // mutes
	   else if (source == _muteAllButton) {
		   manip->setMuted( _muteAllButton->GetBackgroundColour() != _activeBg);
	   }
	   else if (source == _muteButton[i]) {
		   manip->setMuted( !manip->getMuted());
		   break;
	   }
	   
   }

   updateDisplay();
}

void FTmainwin::handlePlotTypeButtons (wxCommandEvent &event)
{
   wxObject *source = event.GetEventObject();

   for (int i=0; i < _pathCount; i++) {
	   if (!_processPath[i]) continue;

	   if (source == _inspecSpecTypeButton[i]) {
		   _inputSpectragram[i]->setPlotType (FTspectragram::SPECTRAGRAM);
		   break;
	   }
	   else if (source == _inspecPlotSolidTypeButton[i]) {
		   _inputSpectragram[i]->setPlotType (FTspectragram::AMPFREQ_SOLID);
		   break;
	   }
	   else if (source == _inspecPlotLineTypeButton[i]) {
		   _inputSpectragram[i]->setPlotType (FTspectragram::AMPFREQ_LINES);
		   break;
	   }
	   else if (source == _outspecSpecTypeButton[i]) {
		   _outputSpectragram[i]->setPlotType (FTspectragram::SPECTRAGRAM);
		   break;
	   }
	   else if (source == _outspecPlotSolidTypeButton[i]) {
		   _outputSpectragram[i]->setPlotType (FTspectragram::AMPFREQ_SOLID);
		   break;
	   }
	   else if (source == _outspecPlotLineTypeButton[i]) {
		   _outputSpectragram[i]->setPlotType (FTspectragram::AMPFREQ_LINES);
		   break;
	   }

	   // all, don't break
 	   else if (source == _inspecSpecTypeAllButton) {
		   _inputSpectragram[i]->setPlotType (FTspectragram::SPECTRAGRAM);
 	   }
 	   else if (source == _inspecPlotSolidTypeAllButton) {
		   _inputSpectragram[i]->setPlotType (FTspectragram::AMPFREQ_SOLID);
 	   }
 	   else if (source == _inspecPlotLineTypeAllButton) {
		   _inputSpectragram[i]->setPlotType (FTspectragram::AMPFREQ_LINES);
 	   }
 	   else if (source == _outspecSpecTypeAllButton) {
		   _outputSpectragram[i]->setPlotType (FTspectragram::SPECTRAGRAM);
 	   }
 	   else if (source == _outspecPlotSolidTypeAllButton) {
		   _outputSpectragram[i]->setPlotType (FTspectragram::AMPFREQ_SOLID);
 	   }
 	   else if (source == _outspecPlotLineTypeAllButton) {
		   _outputSpectragram[i]->setPlotType (FTspectragram::AMPFREQ_LINES);
 	   }

   }

   updateDisplay();
}

void FTmainwin::handleLinkButtons (wxCommandEvent &event)
{
   wxObject *source = event.GetEventObject();

   for (int i=0; i < _pathCount; i++) {
	   if (!_processPath[i]) continue;
	   
	   FTspectralManip *manip =  _processPath[i]->getSpectralManip();

	   if (source == _freqLinkButton[i]) {
		   // popup menu
		   FTlinkMenu *menu = new FTlinkMenu (_freqLinkButton[i], this, manip, FREQ_SPECMOD);
		   _freqLinkButton[i]->PopupMenu (menu, 0, 0);

		   break;
	   }
	   else if (source == _delayLinkButton[i]) {
		   // popup menu
		   FTlinkMenu *menu = new FTlinkMenu (_delayLinkButton[i], this, manip, DELAY_SPECMOD);
		   _delayLinkButton[i]->PopupMenu (menu, 0, 0);
		   break;
	   }
	   else if (source == _feedbLinkButton[i]) {
		   // popup menu
		   FTlinkMenu *menu = new FTlinkMenu (_feedbLinkButton[i], this, manip, FEEDB_SPECMOD);
		   _feedbLinkButton[i]->PopupMenu (menu, 0, 0);

		   break;
	   }
	   else if (source == _scaleLinkButton[i]) {
		   // popup menu
		   FTlinkMenu *menu = new FTlinkMenu (_scaleLinkButton[i], this, manip, SCALE_SPECMOD);
		   _scaleLinkButton[i]->PopupMenu (menu, 0, 0);
		   break;
	   }
// 	   else if (source == _mashLinkButton[i]) {
// 		   // popup menu
// 		   FTlinkMenu *menu = new FTlinkMenu (_mashLinkButton[i], this, manip, MASH_SPECMOD);
// 		   _mashLinkButton[i]->PopupMenu (menu, 0, 0);
// 		   break;
// 	   }
	   else if (source == _gateLinkButton[i]) {
		   // popup menu
		   FTlinkMenu *menu = new FTlinkMenu (_gateLinkButton[i], this, manip, GATE_SPECMOD);
		   _gateLinkButton[i]->PopupMenu (menu, 0, 0);
		   break;
	   }

	   // master link buttons link (or unlink) all to first path
	   else if (source == _freqLinkAllButton) {
		   if (_freqLinkAllButton->GetBackgroundColour() == _activeBg) {
			   // unlink
			   manip->getFreqFilter()->unlink();
		   }
		   else if (i != 0) {
			   // link to first path (unless we are the first)
			   manip->getFreqFilter()->link(_processPath[0]->getSpectralManip()->getFreqFilter());
		   }
		   else {
			   // unlink we are the master
			   manip->getFreqFilter()->unlink();
		   }
		   updateGraphs(0, FREQ_SPECMOD);
	   }
	   else if (source == _scaleLinkAllButton) {
		   if (_scaleLinkAllButton->GetBackgroundColour() == _activeBg) {
			   // unlink
			   manip->getScaleFilter()->unlink();
		   }
		   else if (i != 0) {
			   // link to first path (unless we are the first)
			   manip->getScaleFilter()->link(_processPath[0]->getSpectralManip()->getScaleFilter());
		   }
		   else {
			   // unlink we are the master
			   manip->getScaleFilter()->unlink();
		   }
		   updateGraphs(0, SCALE_SPECMOD);
	   }
// 	   else if (source == _mashLinkAllButton) {
// 		   if (_mashLinkAllButton->GetBackgroundColour() == _activeBg) {
// 			   // unlink
// 			   manip->getMashPushFilter()->unlink();
// 			   manip->getMashLimitFilter()->unlink();
// 		   }
// 		   else if (i != 0) {
// 			   // link to first path (unless we are the first)
// 			   manip->getMashPushFilter()->link(_processPath[0]->getSpectralManip()->getMashPushFilter());
// 			   manip->getMashLimitFilter()->link(_processPath[0]->getSpectralManip()->getMashLimitFilter());
// 		   }
// 		   else {
// 			   // unlink we are the master
// 			   manip->getMashLimitFilter()->unlink();
// 			   manip->getMashPushFilter()->unlink();
// 		   }
// 		   updateGraphs(0, MASH_SPECMOD);
// 	   }
	   else if (source == _gateLinkAllButton) {
		   if (_gateLinkAllButton->GetBackgroundColour() == _activeBg) {
			   // unlink
			   manip->getInverseGateFilter()->unlink();
			   manip->getGateFilter()->unlink();
		   }
		   else if (i != 0) {
			   // link to first path (unless we are the first)
			   manip->getInverseGateFilter()->link(_processPath[0]->getSpectralManip()->getInverseGateFilter());
			   manip->getGateFilter()->link(_processPath[0]->getSpectralManip()->getGateFilter());
		   }
		   else {
			   // unlink we are the master
			   manip->getInverseGateFilter()->unlink();
			   manip->getGateFilter()->unlink();
		   }
		   updateGraphs(0, GATE_SPECMOD);
	   }
	   else if (source == _delayLinkAllButton) {
		   if (_delayLinkAllButton->GetBackgroundColour() == _activeBg) {
			   // unlink
			   manip->getDelayFilter()->unlink();
		   }
		   else if (i != 0) {
			   // link to first path (unless we are the first)
			   manip->getDelayFilter()->link(_processPath[0]->getSpectralManip()->getDelayFilter());
		   }
		   else {
			   // unlink we are the master
			   manip->getDelayFilter()->unlink();
		   }
		   updateGraphs(0, DELAY_SPECMOD);
	   }
	   else if (source == _feedbLinkAllButton) {
		   if (_feedbLinkAllButton->GetBackgroundColour() == _activeBg) {
			   // unlink
			   manip->getFeedbackFilter()->unlink();
		   }
		   else if (i != 0) {
			   // link to first path (unless we are the first)
			   manip->getFeedbackFilter()->link(_processPath[0]->getSpectralManip()->getFeedbackFilter());
		   }
		   else {
			   // unlink we are the master
			   manip->getFeedbackFilter()->unlink();
		   }
		   updateGraphs(0, FEEDB_SPECMOD);
	   }

	   else if (source == _linkMixButton) {
		   // toggle it
		   _linkedMix = (_linkMixButton->GetBackgroundColour() != _activeBg);
	   }
	   
   }

   updateDisplay();
   
}

void FTmainwin::handleLabelButtons (wxCommandEvent &event)
{
	wxObject *source = event.GetEventObject();
	
	// remove sash from rowsizer and replace with button
	int itemi=-1;
	wxWindow * hidewin=0, *showwin=0;

	if (source == _inspecLabelButton || source == _inspecLabelButtonAlt) {
		if (_inspecSash->IsShown()) {
			hidewin = _inspecSash;
			showwin = _inspecLabelButtonAlt;
			itemi = 0;
		}
		else {
			hidewin = _inspecLabelButtonAlt;
			showwin = _inspecSash;
			itemi = 0;
		}
	}			
	else if (source == _freqLabelButton || source == _freqLabelButtonAlt) {
		if (_freqSash->IsShown()) {
			hidewin = _freqSash;
			showwin = _freqLabelButtonAlt;
			itemi = 1;
		}
		else {
			hidewin = _freqLabelButtonAlt;
			showwin = _freqSash;
			itemi = 1;
		}
	}		
	else if (source == _scaleLabelButton || source == _scaleLabelButtonAlt) {
		if (_scaleSash->IsShown()) {
			hidewin = _scaleSash;
			showwin = _scaleLabelButtonAlt;
			itemi = 2;
		}
		else {
			hidewin = _scaleLabelButtonAlt;
			showwin = _scaleSash;
			itemi = 2;
		}
	}		
// 	else if (source == _mashLabelButton || source == _mashLabelButtonAlt) {
// 		if (_mashSash->IsShown()) {
// 			hidewin = _mashSash;
// 			showwin = _mashLabelButtonAlt;
// 			itemi = 3;
// 		}
// 		else {
// 			hidewin = _mashLabelButtonAlt;
// 			showwin = _mashSash;
// 			itemi = 3;
// 		}
// 	}		
	else if (source == _gateLabelButton || source == _gateLabelButtonAlt) {
		if (_gateSash->IsShown()) {
			hidewin = _gateSash;
			showwin = _gateLabelButtonAlt;
			itemi = 3;
		}
		else {
			hidewin = _gateLabelButtonAlt;
			showwin = _gateSash;
			itemi = 3;
		}
	}		
	else if (source == _delayLabelButton || source == _delayLabelButtonAlt) {
		if (_delaySash->IsShown()) {
			hidewin = _delaySash;
			showwin = _delayLabelButtonAlt;
			itemi = 4;
		}
		else {
			hidewin = _delayLabelButtonAlt;
			showwin = _delaySash;
			itemi = 4;
		}
	}		
	else if (source == _feedbLabelButton || source == _feedbLabelButtonAlt) {
		if (_feedbSash->IsShown()) {
			hidewin = _feedbSash;
			showwin = _feedbLabelButtonAlt;
			itemi = 5;
		}
		else {
			hidewin = _feedbLabelButtonAlt;
			showwin = _feedbSash;
			itemi = 5;
		}
	}		
	else if (source == _outspecLabelButton || source == _outspecLabelButtonAlt) {
		if (_outspecSash->IsShown()) {
			hidewin = _outspecSash;
			showwin = _outspecLabelButtonAlt;
			itemi = 6;
		}
		else {
			hidewin = _outspecLabelButtonAlt;
			showwin = _outspecSash;
			itemi = 6;
		}
	}		


	
	if (hidewin && showwin) {
		hidewin->Show(false);

		_rowItems[itemi] = showwin;

		if (itemi > 0) {
			_rowItems[itemi]->GetConstraints()->top.Below(_rowItems[itemi-1], 2);
		}

		if (itemi < _rowCount-1) {
			_rowItems[itemi+1]->GetConstraints()->top.Below(_rowItems[itemi], 2);
		}
		
		showwin->Show(true);
		_rowPanel->Layout();
		rowpanelScrollSize();

	}

	
}



void FTmainwin::handleChoices (wxCommandEvent &event)
{
	wxObject *source = event.GetEventObject();

	FTioSupport * iosup = FTioSupport::instance();
	
	if (source == _freqBinsChoice) {
		int sel = _freqBinsChoice->GetSelection();
		iosup->stopProcessing();
		
		for (int i=0; i < _pathCount; i++) {
			if (!_processPath[i]) continue;

			_processPath[i]->getSpectralManip()->setFFTsize( (FTspectralManip::FFT_Size) (unsigned)_freqBinsChoice->GetClientData(sel) );
		}

		iosup->startProcessing();
		iosup->reinit(false);
		//updateDisplay();

		updateGraphs(0, ALL_SPECMOD);
	}
	else if (source == _overlapChoice) {
		int sel = _overlapChoice->GetSelection();
		for (int i=0; i < _pathCount; i++) {
			if (!_processPath[i]) continue;

			_processPath[i]->getSpectralManip()->setOversamp( (int )_overlapChoice->GetClientData(sel) );
		}
		
	}
	else if (source == _windowingChoice) {
		int sel = _windowingChoice->GetSelection();
		for (int i=0; i < _pathCount; i++) {
			if (!_processPath[i]) continue;

			_processPath[i]->getSpectralManip()->setWindowing( (FTspectralManip::Windowing) sel );
		}
		
	}
	else if (source == _plotSpeedChoice) {
		int sel = _plotSpeedChoice->GetSelection();
		for (int i=0; i < _pathCount; i++) {
			if (!_processPath[i]) continue;

			_processPath[i]->getSpectralManip()->setUpdateSpeed( (FTspectralManip::UpdateSpeed)
									     (int) _plotSpeedChoice->GetClientData(sel) );
			if (i==0) {
				_updateMS = 10; // start small
				_eventTimer->Stop();
				_eventTimer->Start(_updateMS, FALSE);
			}
		}
		
	}
	else if (source == _superSmoothCheck) {
		_superSmooth = _superSmoothCheck->GetValue();
		if (_superSmooth) {
			_updateMS = 10; // start small
			_eventTimer->Stop();
			_eventTimer->Start(_updateMS, FALSE);
		}
	}
	
}

void FTmainwin::handleIOButtons (wxCommandEvent &event)
{
	FTioSupport *iosup = FTioSupport::instance();

	if (event.GetId() == FT_IOreconnectButton)
	{
		if (! iosup->isInited() || (_ioNameText->GetValue() != iosup->getName()))
		{
			fprintf(stderr, "Reconnecting as %s...\n", _ioNameText->GetValue().c_str());
			
			iosup->stopProcessing();
			iosup->close();
			
			iosup->setName (_ioNameText->GetValue().c_str());
			if (iosup->init()) {
				if (iosup->startProcessing()) {
					iosup->reinit();
				}
			}

			updateDisplay();
		}
	}
}

void FTmainwin::handlePathCount (wxCommandEvent &event)
{
	int newcnt = _pathCountChoice->GetSelection() + 1;

	changePathCount (newcnt);
}

void FTmainwin::changePathCount (int newcnt)
{
	FTioSupport *iosup = FTioSupport::instance();

	// change path count
	if (newcnt < _pathCount)
	{
		// get rid of the some
		for (int i=_pathCount-1; i >= newcnt; i--)
		{
			if (!_processPath[i]) continue; // shouldnt happen

			// remove all stuff
			// processpath deactivateed in here too
			removePathStuff(i);

		}
	}
	else if (newcnt > _pathCount) {
		// add some
		for (int i=_pathCount; i < newcnt; i++)
		{
			// add to io support instance
			_processPath[i] = iosup->setProcessPathActive (i, true);

			createPathStuff(i);
		}
	}

	_pathCount = iosup->getActivePathCount();
	//printf ("new path cnt = %d\n", _pathCount);
	
	updateDisplay();
}

void FTmainwin::handleSashDragged (wxSashEvent &event)
{
	wxObject *source = event.GetEventObject();

	wxSashLayoutWindow *sash = (wxSashLayoutWindow *) source;
	
	if (event.GetDragStatus() == wxSASH_STATUS_OK)
	{
		wxLayoutConstraints *cnst = sash->GetConstraints();
		wxRect bounds = event.GetDragRect();

		int newh = bounds.height;
		if (newh < 36) newh = 36;
		
		cnst->height.Absolute (newh);
		_rowPanel->Layout();
		rowpanelScrollSize();
	}

}


void FTmainwin::handleGain (wxCommandEvent &event)
{
	wxObject *source = event.GetEventObject();

	for (int i=0; i < _pathCount; i++)
	{
		if (!_processPath[i]) continue;
		FTspectralManip *manip =  _processPath[i]->getSpectralManip();

		if (source == _gainSpinCtrl[i]) {
			// db scale
			float gain = pow(10, _gainSpinCtrl[i]->GetValue() / 20.0);
			// printf ("new gain is %g\n", gain);
			manip->setInputGain(gain);
		}
	}
}

void FTmainwin::handleMixSlider (wxCommandEvent &event)
{
	wxObject *source = event.GetEventObject();

	float newval = 1.0;
	
	for (int i=0; i < _pathCount; i++)
	{
		if (_mixSlider[i] == source) {
			newval = _mixSlider[i]->GetValue() / 1000.0;
			break;
		}
	}
		
	for (int i=0; i < _pathCount; i++)
	{
		if (!_processPath[i]) continue;
		FTspectralManip *manip =  _processPath[i]->getSpectralManip();

		if (_linkedMix || source == _mixSlider[i]) {
			// 0 -> 1000
			manip->setMixRatio( newval);
			_mixSlider[i]->SetValue( (int) (newval * 1000.0));
		}
	}
}


void FTmainwin::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	// TRUE is to force the frame to close
	FTioSupport::instance()->close();
	Close(TRUE);
}

void FTmainwin::OnAbout(wxCommandEvent& WXUNUSED(event))
{
	wxString msg(wxString::Format("FreqTweak %s brought to you by Jesse Chappell", freqtweak_version));

	wxMessageBox(msg, "About FreqTweak", wxOK | wxICON_INFORMATION, this);
}


void FTmainwin::OnIdle(wxIdleEvent &event)
{
	//printf("OnIdle\n");
	if (_superSmooth) {
		::wxWakeUpIdle();
	}
}

void FTmainwin::checkEvents()
{
	for (int i=0; i < _pathCount; i++)
	{
		if ( ! _updateTokens[i]->getIgnore()) {
			if (_updateTokens[i]->getUpdated(false)) {
				updatePlot (i);
			}
			else {
				// auto calibrate
				_eventTimer->Stop();
				_eventTimer->Start(++_updateMS, FALSE);
				//printf("%d\n", _updateMS);
			}
		}
	}
}

void FTmainwin::updatePlot (int plotid)
{
	
	FTspectralManip *manip = _processPath[plotid]->getSpectralManip();
	
	if (_inspecSash->IsShown()) {
		const float *inpower = manip->getRunningInputPower();
		_inputSpectragram[plotid]->plotNextData(inpower, manip->getFFTsize() >> 1);
	}
	
	if (_outspecSash->IsShown()) {
		const float *outpower = manip->getRunningOutputPower();
		_outputSpectragram[plotid]->plotNextData(outpower, manip->getFFTsize() >> 1);
	}
	
}


void FTmainwin::updateGraphs(FTactiveBarGraph *exclude, SpecModType smtype)
{
	for (int i=0; i < _pathCount; i++)
	{
		if (!_processPath[i]) continue; 

		switch (smtype)
		{
		case FREQ_SPECMOD:
			if (_freqGraph[i] != exclude) {
				_freqGraph[i]->Refresh(FALSE);
			}
			break;
		case DELAY_SPECMOD:
			if (_delayGraph[i] != exclude) {
				_delayGraph[i]->Refresh(FALSE);
			}
			break;
		case FEEDB_SPECMOD:
			if (_feedbackGraph[i] != exclude) {
				_feedbackGraph[i]->Refresh(FALSE);
			}
			break;
		case SCALE_SPECMOD:
			if (_scaleGraph[i] != exclude) {
				_scaleGraph[i]->Refresh(FALSE);
			}
			break;
// 		case MASH_SPECMOD:
// 			if (_mashGraph[i] != exclude) {
// 				_mashGraph[i]->Refresh(FALSE);
// 			}
// 			break;
		case GATE_SPECMOD:
			if (_gateGraph[i] != exclude) {
				_gateGraph[i]->Refresh(FALSE);
			}
			break;
		case ALL_SPECMOD:
		default:
			// do them all
			//SetSize(GetSize());
			_freqGraph[i]->recalculate();
			
			_delayGraph[i]->recalculate();

			_feedbackGraph[i]->recalculate();

			_scaleGraph[i]->recalculate();
			//_mashGraph[i]->recalculate();
			_gateGraph[i]->recalculate();

			break;
		}
		
	}

}


void FTmainwin::updatePosition(const wxString &freqstr, const wxString &valstr)
{
	wxStatusBar * statBar = GetStatusBar();

	if (statBar) {
		statBar->SetStatusText(freqstr, 1);
		statBar->SetStatusText(valstr, 2);
	}
}


FTlinkMenu::FTlinkMenu (wxWindow * parent, FTmainwin *win, FTspectralManip *specmanip, SpecModType stype)
	: wxMenu(), _mwin(win), _specmanip(specmanip), _stype(stype)
{

	FTspectrumModifier *linkedto = 0;
	int itemid = 1000;
	FTspectrumModifier *tempfilt, *thisfilt;
	FTspectralManip *tempmanip;
	
	
	switch (stype) {
		case FREQ_SPECMOD:
			thisfilt = _specmanip->getFreqFilter();
			linkedto = thisfilt->getLink();
			break;
		case DELAY_SPECMOD:
			thisfilt = _specmanip->getDelayFilter();
			linkedto = thisfilt->getLink();
			break;
		case FEEDB_SPECMOD:
			thisfilt = _specmanip->getFeedbackFilter();
			linkedto = thisfilt->getLink();
			break;
	        case SCALE_SPECMOD:
			thisfilt = _specmanip->getScaleFilter();
			linkedto = thisfilt->getLink();
			break;
// 	        case MASH_SPECMOD:
// 			linkedto = _specmanip->getMashLimitFilter()->getLink();
// 			break;
	        case GATE_SPECMOD:
			thisfilt = _specmanip->getGateFilter();
			linkedto = thisfilt->getLink();
			break;
	        default:
			// we can't handle any others
			return;
	}	
	
	for (int i=0; i < FT_MAXPATHS; i++) {
		if (!_mwin->_processPath[i] || _mwin->_processPath[i]->getSpectralManip()==_specmanip)
			continue;

		wxMenuItem * item = 0;

		tempmanip = _mwin->_processPath[i]->getSpectralManip();
		
		switch (stype) {
		  case FREQ_SPECMOD:
			  tempfilt =  tempmanip->getFreqFilter();
			  if (linkedto == tempfilt || thisfilt->getLinkedFrom().Find((unsigned) tempfilt)) {
				  item = new wxMenuItem(this, itemid, wxString::Format("* Path %d *", i+1)); 
			  } else {
				  item = new wxMenuItem(this, itemid, wxString::Format("Path %d", i+1)); 
			  }
				  
			  this->Connect( itemid,  wxEVT_COMMAND_MENU_SELECTED,
					 (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction)
					 &FTlinkMenu::OnLinkItem,
					 new SpecModObject(tempmanip));
			  break;
		  case DELAY_SPECMOD:
			  tempfilt =  tempmanip->getDelayFilter();
			  if (linkedto == tempfilt  || thisfilt->getLinkedFrom().Find((unsigned) tempfilt)) {
				  item = new wxMenuItem(this, itemid, wxString::Format("* Path %d *", i+1)); 
			  } else {
				  item = new wxMenuItem(this, itemid, wxString::Format("Path %d", i+1)); 
			  }
			  
			  this->Connect( itemid,  wxEVT_COMMAND_MENU_SELECTED,
					 (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction)
					 &FTlinkMenu::OnLinkItem,
					 new SpecModObject(tempmanip));

			  break;
		  case FEEDB_SPECMOD:

			  tempfilt =  tempmanip->getFeedbackFilter();
			  if (linkedto == tempfilt  || thisfilt->getLinkedFrom().Find((unsigned) tempfilt)) {
				  item = new wxMenuItem(this, itemid, wxString::Format("* Path %d *", i+1)); 
			  } else {
				  item = new wxMenuItem(this, itemid, wxString::Format("Path %d", i+1)); 
			  }
			  
			  this->Connect( itemid,  wxEVT_COMMAND_MENU_SELECTED,
					 (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction)
					 &FTlinkMenu::OnLinkItem,
					 new SpecModObject(tempmanip));

			  break;			  
		  case SCALE_SPECMOD:

			  tempfilt =  tempmanip->getScaleFilter();
			  if (linkedto == tempfilt  || thisfilt->getLinkedFrom().Find((unsigned) tempfilt)) {
				  item = new wxMenuItem(this, itemid, wxString::Format("* Path %d *", i+1)); 
			  } else {
				  item = new wxMenuItem(this, itemid, wxString::Format("Path %d", i+1)); 
			  }
			  
			  this->Connect( itemid,  wxEVT_COMMAND_MENU_SELECTED,
					 (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction)
					 &FTlinkMenu::OnLinkItem,
					 new SpecModObject(tempmanip));

			  break;			  
// 		  case MASH_SPECMOD:

// 			  tempfilt =  tempmanip->getMashLimitFilter();
// 			  if (linkedto == tempfilt) {
// 				  item = new wxMenuItem(this, itemid, wxString::Format("* Path %d *", i+1)); 
// 			  } else {
// 				  item = new wxMenuItem(this, itemid, wxString::Format("Path %d", i+1)); 
// 			  }
			  
// 			  this->Connect( itemid,  wxEVT_COMMAND_MENU_SELECTED,
// 					 (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction)
// 					 &FTlinkMenu::OnLinkItem,
// 					 new SpecModObject(tempmanip));

// 			  break;			  
		  case GATE_SPECMOD:

			  tempfilt =  tempmanip->getGateFilter();
			  if (linkedto == tempfilt  || thisfilt->getLinkedFrom().Find((unsigned) tempfilt)) {
				  item = new wxMenuItem(this, itemid, wxString::Format("* Path %d *", i+1)); 
			  } else {
				  item = new wxMenuItem(this, itemid, wxString::Format("Path %d", i+1)); 
			  }
			  
			  this->Connect( itemid,  wxEVT_COMMAND_MENU_SELECTED,
					 (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction)
					 &FTlinkMenu::OnLinkItem,
					 new SpecModObject(tempmanip));

			  break;			  
		default:
			break;
		}
		    
		if (item) {
			this->Append(item);
			itemid++;
		}
	}

	// add unlink item
	wxMenuItem * item = new wxMenuItem(this, itemid, "Unlink"); 
	this->Connect( itemid, wxEVT_COMMAND_MENU_SELECTED,
		       (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction)
		       &FTlinkMenu::OnUnlinkItem);

	this->AppendSeparator ();
	this->Append (item);
}


void FTlinkMenu::OnLinkItem(wxCommandEvent &event)
{
	SpecModObject * smobj = (SpecModObject *) event.m_callbackUserData;

	
	switch(_stype)
	{
	  case FREQ_SPECMOD:
		  _specmanip->getFreqFilter()->link (smobj->specm->getFreqFilter());
		break;

	  case DELAY_SPECMOD:
		  _specmanip->getDelayFilter()->link (smobj->specm->getDelayFilter());
		break;

	  case FEEDB_SPECMOD:
		  _specmanip->getFeedbackFilter()->link (smobj->specm->getFeedbackFilter());
		  break;		  
	  case SCALE_SPECMOD:
		  _specmanip->getScaleFilter()->link (smobj->specm->getScaleFilter());
		  break;		  
// 	  case MASH_SPECMOD:
// 		  _specmanip->getMashLimitFilter()->link (smobj->specm->getMashLimitFilter());
// 		  _specmanip->getMashPushFilter()->link (smobj->specm->getMashPushFilter());
// 		  break;		  
	  case GATE_SPECMOD:
		  _specmanip->getInverseGateFilter()->link (smobj->specm->getInverseGateFilter());
		  _specmanip->getGateFilter()->link (smobj->specm->getGateFilter());
		  break;		  
	  default:
		break;
	}

	wxMenuItem *item = (wxMenuItem *) event.GetEventObject();
	if (item) {
		this->Disconnect( item->GetId(),  wxEVT_COMMAND_MENU_SELECTED,
				  (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction)
				  &FTlinkMenu::OnLinkItem,
				  smobj);
		
	}

	_mwin->updateGraphs(0, _stype);
	
	delete smobj;
}
		    
void FTlinkMenu::OnUnlinkItem(wxCommandEvent &event)
{

	switch(_stype)
	{
	  case FREQ_SPECMOD:
		  _specmanip->getFreqFilter()->unlink ();
		break;

	  case DELAY_SPECMOD:
		  _specmanip->getDelayFilter()->unlink ();
		break;

	  case FEEDB_SPECMOD:
		  _specmanip->getFeedbackFilter()->unlink ();
		  break;		  
	  case SCALE_SPECMOD:
		  _specmanip->getScaleFilter()->unlink ();
		  break;		  
// 	  case MASH_SPECMOD:
// 		  _specmanip->getMashPushFilter()->unlink ();
// 		  _specmanip->getMashLimitFilter()->unlink ();
// 		  break;		  
	  case GATE_SPECMOD:
		  _specmanip->getInverseGateFilter()->unlink ();
		  _specmanip->getGateFilter()->unlink ();
		  break;		  
	   default:
		break;
	}

	wxMenuItem *item = (wxMenuItem *) event.GetEventObject();
	if (item) {
		this->Disconnect( item->GetId(),  wxEVT_COMMAND_MENU_SELECTED,
				  (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction)
				  &FTlinkMenu::OnLinkItem);
		
	}

	_mwin->updateGraphs(0, _stype);
	
}


void FTmainwin::handleStoreButton (wxCommandEvent &event)
{
	_configManager.storeSettings ( _presetCombo->GetValue().c_str());

	rebuildPresetCombo();
}

void FTmainwin::handleLoadButton (wxCommandEvent &event)
{
	loadPreset (_presetCombo->GetValue().c_str());
}


void FTmainwin::loadPreset (const wxString &name)
{
	_configManager.loadSettings ( name);

	changePathCount ( FTioSupport::instance()->getActivePathCount() );

	_presetCombo->SetValue(name);
	rebuildPresetCombo();
	
	updateGraphs(0, ALL_SPECMOD);
	
	updateDisplay();	
}


void FTmainwin::rebuildPresetCombo()
{
	FTstringList * namelist =  _configManager.getSettingsNames();

	wxString selected = _presetCombo->GetValue();
	
	_presetCombo->Clear();
	
	wxNode * node = (wxNode *) namelist->GetFirst();
	while (node)
	{
		wxString *name = (wxString *) node->GetData();

		_presetCombo->Append(*name);
		
		node = node->GetNext();
	}

	if ( _presetCombo->FindString(selected) >= 0) {
		_presetCombo->SetValue(selected);
	}
	
	delete namelist;
}
