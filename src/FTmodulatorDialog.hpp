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

#ifndef __FTMODULATORDIALOG_HPP__
#define __FTMODULATORDIALOG_HPP__


#include <list>
using namespace std;

#include <wx/wx.h>

#include "FTtypes.hpp"

class FTprocI;
class FTmainwin;
class FTmodulatorI;

class wxListCtrl;

class FTmodulatorDialog : public wxFrame
{
  public:
	// ctor(s)
	FTmodulatorDialog(FTmainwin * parent, wxWindowID id, const wxString& title,
			  const wxPoint& pos = wxDefaultPosition,
			  const wxSize& size = wxSize(400,600),
			  long style = wxDEFAULT_FRAME_STYLE,
			  const wxString& name = wxT("ModulatorDialog"));


	virtual ~FTmodulatorDialog();

	void refreshState();
	
 protected:

	void init();
	
	void onClose(wxCloseEvent & ev);
	void onCommit(wxCommandEvent & ev);

	void onSize(wxSizeEvent &ev);
	void onPaint(wxPaintEvent &ev);

	void onItemSelected (wxListEvent &ev);
	void onRightClick (wxMouseEvent &ev);
	void onItemRightClick (wxListEvent &ev);

	void onAddModulator (wxCommandEvent &ev);
	
	// void onAutoCheck (wxCommandEvent &ev);

	wxListCtrl * _channelLists[FT_MAXPATHS];
	int  _channelCount;
	
	wxBoxSizer * _chanlistSizer;

	wxMenu * _popupMenu;
	wxMenuItem * _editMenuItem;
	wxMenuItem * _removeMenuItem;

	FTmodulatorI * _clickedMod;
        int _clickedChannel;
	
	FTmainwin * _mainwin;


	bool _justResized;
	int _lastSelected;
private:
	// any class wishing to process wxWindows events must use this macro
	DECLARE_EVENT_TABLE()
	
};


#endif
