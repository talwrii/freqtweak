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

#ifndef __FTMAINWIN_HPP__
#define __FTMAINWIN_HPP__


#include <wx/wx.h>

//#include <wx/sashwin.h>
#include <wx/laywin.h>

#include "FTtypes.hpp"
#include "FTspectragram.hpp"
#include "FTconfigManager.hpp"

#include "LockFreeFifo.hpp"

class wxSpinCtrl;

class FTprocessPath;
class FTactiveBarGraph;
class FTspectralManip;
class FTspectrumModifier;

class FTupdateToken;
class FTupdateTimer;
class FTlinkMenu;



class FTmainwin : public wxFrame
{
  public:
	// ctor(s)
	FTmainwin(int startpaths, const wxString& title, const wxString &rcdir , const wxPoint& pos, const wxSize& size);
	
	// event handlers (these functions should _not_ be virtual)
	void OnQuit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void OnIdle(wxIdleEvent & event);
	void OnClose(wxCloseEvent &event);
	
	//void OnSize(wxSizeEvent &event);

	

	void updateDisplay();
	void updateGraphs(FTactiveBarGraph *exclude, SpecModType smtype);

	void updatePosition(const wxString &freqstr, const wxString &valstr); 

	void loadPreset (const wxString & name);

	void cleanup ();
	
  protected:

	
	void buildGui();
	void updatePlot(int plotnum);
	void checkEvents();

	// per path handlers
	
	void handleInputButton (wxCommandEvent &event);
	void handleOutputButton (wxCommandEvent &event);

	void handleBypassButtons (wxCommandEvent &event);
	void handleLinkButtons (wxCommandEvent &event);
	void handleLabelButtons (wxCommandEvent &event);
	
	void handlePlotTypeButtons (wxCommandEvent &event);
	void handleGridButtons (wxCommandEvent &event);
	
	void handleChoices (wxCommandEvent &event);
	void handleSashDragged (wxSashEvent &event);

	void handleMixSlider (wxCommandEvent &event);
	void handleGain (wxCommandEvent &event);
	void handlePathCount (wxCommandEvent &event);
	void changePathCount (int newcnt);
	

	void handleStoreButton (wxCommandEvent &event);
	void handleLoadButton (wxCommandEvent &event);

	void handleIOButtons (wxCommandEvent &event);
	
	void rowpanelScrollSize();

	void removePathStuff(int i);
	void createPathStuff(int i);
	void rebuildPresetCombo();

	
	FTprocessPath * _processPath[FT_MAXPATHS];

	int _startpaths;
	
	// overall controls
	wxChoice * _freqBinsChoice;
	wxChoice * _overlapChoice;
	wxChoice * _windowingChoice;
	wxChoice * _freqScaleChoice;
	wxSlider * _timescaleSlider;
	wxChoice * _pathCountChoice;
	wxTextCtrl * _ioNameText;
	
	// array of spectragrams
	FTspectragram * _inputSpectragram[FT_MAXPATHS];
	FTspectragram * _outputSpectragram[FT_MAXPATHS];

	
	FTactiveBarGraph * _scaleGraph[FT_MAXPATHS];
	FTactiveBarGraph * _freqGraph[FT_MAXPATHS];
	FTactiveBarGraph * _delayGraph[FT_MAXPATHS];
	FTactiveBarGraph * _feedbackGraph[FT_MAXPATHS];
	FTactiveBarGraph * _mashGraph[FT_MAXPATHS];
	FTactiveBarGraph * _gateGraph[FT_MAXPATHS];


	wxButton * _bypassAllButton;
	wxButton * _muteAllButton;
	
	wxButton * _scaleBypassAllButton;
	wxButton * _mashBypassAllButton;
	wxButton * _gateBypassAllButton;
	wxButton * _freqBypassAllButton;
	wxButton * _delayBypassAllButton;
	wxButton * _feedbBypassAllButton;

	wxButton * _scaleLinkAllButton;
	wxButton * _mashLinkAllButton;
	wxButton * _gateLinkAllButton;
	wxButton * _freqLinkAllButton;
	wxButton * _delayLinkAllButton;
	wxButton * _feedbLinkAllButton;

	wxButton * _scaleGridButton;
	wxButton * _gateGridButton;
	wxButton * _freqGridButton;
	wxButton * _delayGridButton;
	wxButton * _feedbGridButton;

	wxButton * _scaleGridSnapButton;
	wxButton * _gateGridSnapButton;
	wxButton * _freqGridSnapButton;
	wxButton * _delayGridSnapButton;
	wxButton * _feedbGridSnapButton;
	
	
	wxButton * _inspecLabelButton;
	wxButton * _scaleLabelButton;
	wxButton * _mashLabelButton;
	wxButton * _gateLabelButton;
	wxButton * _freqLabelButton;
	wxButton * _delayLabelButton;
	wxButton * _feedbLabelButton;
	wxButton * _outspecLabelButton;

	wxButton * _inspecLabelButtonAlt;
	wxButton * _scaleLabelButtonAlt;
	wxButton * _mashLabelButtonAlt;
	wxButton * _gateLabelButtonAlt;
	wxButton * _freqLabelButtonAlt;
	wxButton * _delayLabelButtonAlt;
	wxButton * _feedbLabelButtonAlt;
	wxButton * _outspecLabelButtonAlt;


	wxButton * _inspecSpecTypeAllButton;
	wxButton * _inspecPlotSolidTypeAllButton;
	wxButton * _inspecPlotLineTypeAllButton;
	wxButton * _outspecSpecTypeAllButton;
	wxButton * _outspecPlotSolidTypeAllButton;
	wxButton * _outspecPlotLineTypeAllButton;


	
	wxButton * _linkMixButton;

	
	
	// per path panels
	wxPanel * _upperPanels[FT_MAXPATHS];
	wxPanel * _inspecPanels[FT_MAXPATHS];
	wxPanel * _freqPanels[FT_MAXPATHS];
	wxPanel * _scalePanels[FT_MAXPATHS];
	wxPanel * _mashPanels[FT_MAXPATHS];
	wxPanel * _gatePanels[FT_MAXPATHS];
	wxPanel * _delayPanels[FT_MAXPATHS];
	wxPanel * _feedbPanels[FT_MAXPATHS];
	wxPanel * _outspecPanels[FT_MAXPATHS];
	wxPanel * _lowerPanels[FT_MAXPATHS];

	
	// per path buttons
	
	wxButton * _inputButton[FT_MAXPATHS];
	wxButton * _outputButton[FT_MAXPATHS];

	wxSpinCtrl *_gainSpinCtrl[FT_MAXPATHS];
	wxSlider * _mixSlider[FT_MAXPATHS];
	
	wxButton * _bypassButton[FT_MAXPATHS];
	wxButton * _muteButton[FT_MAXPATHS];

    
	wxButton * _inspecSpecTypeButton[FT_MAXPATHS];
	wxButton * _inspecPlotSolidTypeButton[FT_MAXPATHS];
	wxButton * _inspecPlotLineTypeButton[FT_MAXPATHS];
	wxButton * _outspecSpecTypeButton[FT_MAXPATHS];
	wxButton * _outspecPlotSolidTypeButton[FT_MAXPATHS];
	wxButton * _outspecPlotLineTypeButton[FT_MAXPATHS];
	
	wxButton * _scaleBypassButton[FT_MAXPATHS];
	wxButton * _mashBypassButton[FT_MAXPATHS];
	wxButton * _gateBypassButton[FT_MAXPATHS];
	wxButton * _freqBypassButton[FT_MAXPATHS];
	wxButton * _delayBypassButton[FT_MAXPATHS];
	wxButton * _feedbBypassButton[FT_MAXPATHS];

	wxButton * _scaleLinkButton[FT_MAXPATHS];
	wxButton * _mashLinkButton[FT_MAXPATHS];
	wxButton * _gateLinkButton[FT_MAXPATHS];
	wxButton * _freqLinkButton[FT_MAXPATHS];
	wxButton * _delayLinkButton[FT_MAXPATHS];
	wxButton * _feedbLinkButton[FT_MAXPATHS];

	// sizers
	wxBoxSizer *_inspecsizer, *_freqsizer, *_delaysizer, *_feedbsizer, *_outspecsizer, *_scalesizer, *_mashsizer, *_gatesizer;
	wxBoxSizer *_inspecbuttsizer, *_freqbuttsizer, *_delaybuttsizer, *_feedbbuttsizer, *_outspecbuttsizer,
		*_scalebuttsizer, *_mashbuttsizer, *_gatebuttsizer;
	wxBoxSizer *_lowersizer, *_uppersizer;

	wxScrolledWindow *_rowPanel;
	
	wxPanel *_inspecPanel, *_freqPanel, *_delayPanel, *_feedbPanel, *_outspecPanel, *_scalePanel, *_mashPanel, *_gatePanel;
	wxSashLayoutWindow *_inspecSash, *_freqSash, *_delaySash, *_feedbSash, *_outspecSash, *_scaleSash, *_mashSash, *_gateSash;


	// shown flags
	bool _inspecShown;
	bool _freqShown;
	bool _scaleShown;
	bool _mashShown;
	bool _gateShown;
	bool _delayShown;
	bool _feedbShown;
	bool _outspecShown;

	bool _linkedMix;
	
	// bitmap data
	wxBitmap * _bypassBitmap;
	wxBitmap * _bypassActiveBitmap;
	wxBitmap * _linkBitmap;
	wxBitmap  *_linkActiveBitmap;

	wxColour _defaultBg;
	wxColour _activeBg;
	
	
	FTupdateTimer *_eventTimer;
	int _updateMS;
	bool _superSmooth;
	
	
	wxWindow ** _rowItems;
	int _pathCount;
	int _rowCount;

	FTconfigManager _configManager;

	wxComboBox * _presetCombo;
	wxChoice * _plotSpeedChoice;
	wxCheckBox *_superSmoothCheck;
	
	FTupdateToken * _updateTokens[FT_MAXPATHS];


	friend class FTupdateTimer;
	friend class FTlinkMenu;

	
  private:
	// any class wishing to process wxWindows events must use this macro
	DECLARE_EVENT_TABLE()
};


class FTupdateTimer
	: public wxTimer
{
  public:
	FTupdateTimer(FTmainwin *win) : mwin(win) {}

	void Notify() { mwin->checkEvents(); }

	FTmainwin *mwin;
};



class FTlinkMenu
	: public wxMenu
{
  public:
	FTlinkMenu (wxWindow *parent, FTmainwin *win, FTspectralManip *specmod, SpecModType stype);

	void OnLinkItem(wxCommandEvent &event);
	void OnUnlinkItem(wxCommandEvent &event);

	FTmainwin *_mwin;
	FTspectralManip *_specmanip;
	SpecModType _stype;

	class SpecModObject : public wxObject
	{
	   public:
		SpecModObject(FTspectralManip *sm) : specm(sm) {;}

		FTspectralManip *specm;
	};
	
  private:
	// any class wishing to process wxWindows events must use this macro
//	DECLARE_EVENT_TABLE()
};



#endif
