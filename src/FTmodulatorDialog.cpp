/*
** Copyright (C) 2004 Jesse Chappell <jesse@essej.net>
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

#include <wx/wx.h>

#include <wx/listctrl.h>
#include <iostream>
using namespace std;

#include "FTmodulatorDialog.hpp"
#include "FTmodulatorManager.hpp"
#include "FTioSupport.hpp"
#include "FTmainwin.hpp"
#include "FTdspManager.hpp"
#include "FTprocI.hpp"
#include "FTmodulatorI.hpp"
#include "FTprocessPath.hpp"
#include "FTspectralEngine.hpp"
#include "FTmodulatorGui.hpp"

#include <sigc++/sigc++.h>
using namespace SigC;

enum {
	ID_AddButton=8000,
	ID_PopupMenu,
	ID_EditMenuItem,
	ID_RemoveMenuItem,
	ID_ChannelList
};


enum {
	ID_AddModulatorBase = 9000,
	ID_AddModulatorChannelBase = 9100,
	ID_AddModulatorChannelMax = 9110
};

BEGIN_EVENT_TABLE(FTmodulatorDialog, wxFrame)

	EVT_CLOSE(FTmodulatorDialog::onClose)
	EVT_IDLE(FTmodulatorDialog::OnIdle)
	
	EVT_SIZE (FTmodulatorDialog::onSize)
	EVT_PAINT (FTmodulatorDialog::onPaint)


	EVT_COMMAND_RANGE(ID_AddModulatorChannelBase, ID_AddModulatorChannelMax, wxEVT_COMMAND_BUTTON_CLICKED, FTmodulatorDialog::onAddButton)
	
END_EVENT_TABLE()


FTmodulatorDialog::FTmodulatorDialog(FTmainwin * parent, wxWindowID id,
				     const wxString & title,
				     const wxPoint& pos,
				     const wxSize& size,
				     long style,
				     const wxString& name )

	: wxFrame(parent, id, title, pos, size, style, name),
	 _clickedChannel(-1), _mainwin(parent)
{

	init();
}

FTmodulatorDialog::~FTmodulatorDialog()
{
}


void FTmodulatorDialog::onSize(wxSizeEvent &ev)
{

	_justResized = true;
	ev.Skip();
}

void FTmodulatorDialog::onPaint(wxPaintEvent &ev)
{
	if (_justResized) {
		int width,height;

		_justResized = false;

// 		for (int i=0; i < _channelCount; ++i)
// 		{
// 			_channelLists[i]->GetClientSize(&width, &height);
// 			_channelLists[i]->SetColumnWidth(0, width);
// 		}
		
	}
	ev.Skip();
}

void FTmodulatorDialog::init()
{
	wxBoxSizer * mainsizer = new wxBoxSizer(wxVERTICAL);
	wxStaticText * statText;

	_chanlistSizer = new wxBoxSizer(wxHORIZONTAL);

	FTioSupport * iosup = FTioSupport::instance();
	// do this for every active process path

	
	FTprocessPath * procpath;
	_channelCount = 0;

	int addbuttid = ID_AddModulatorChannelBase;
	
	for (int i=0; i < iosup->getActivePathCount(); ++i)
	{
		procpath = iosup->getProcessPath(i);
		if (!procpath) break;
		
		wxBoxSizer * sourceSizer = new wxBoxSizer(wxVERTICAL);

		wxBoxSizer * rowsizer = new wxBoxSizer(wxHORIZONTAL);
		statText = new wxStaticText (this, -1, wxString::Format(wxT("%s %d"), wxT("Channel"), i+1),  wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE);
		rowsizer->Add(statText, 0, wxALL|wxALIGN_CENTRE_VERTICAL|wxALIGN_CENTRE, 3);

		rowsizer->Add(1,-1, 1);
		
		wxButton *addButt = new wxButton(this, addbuttid, wxT("Add..."));
		rowsizer->Add (addButt, 0, wxALL|wxALIGN_CENTRE_VERTICAL, 2);
		
		sourceSizer->Add (rowsizer, 0, wxEXPAND|wxALIGN_CENTRE|wxALIGN_CENTRE_VERTICAL, 2);
		
		_channelScrollers[i] = new wxScrolledWindow(this, -1, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
		_channelScrollers[i]->SetScrollRate (10, 30);
		_channelSizers[i] = new wxBoxSizer(wxVERTICAL);

		_channelScrollers[i]->SetSizer(_channelSizers[i]);
		_channelScrollers[i]->SetAutoLayout(true);
		
		sourceSizer->Add (_channelScrollers[i], 1, wxEXPAND|wxALL, 2);

		_chanlistSizer->Add (sourceSizer, 1, wxEXPAND|wxALL, 2);
		_channelCount++;

		addbuttid++;
	}		


	mainsizer->Add (_chanlistSizer, 1, wxEXPAND|wxALL, 2);
	
	wxMenuItem * item;
	_popupMenu = new wxMenu();

	int itemid = ID_AddModulatorBase;

	FTmodulatorManager::ModuleList mlist;
	FTmodulatorManager::instance()->getAvailableModules(mlist);

	int modnum = 0;
	for (FTmodulatorManager::ModuleList::iterator moditer = mlist.begin(); moditer != mlist.end(); ++moditer)
	{
		item = new wxMenuItem(_popupMenu, itemid, wxT("Add ") + wxString::FromAscii ((*moditer)->getName().c_str()));
		_popupMenu->Append (item);
		
		Connect( itemid,  wxEVT_COMMAND_MENU_SELECTED,
				     (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction)
				     &FTmodulatorDialog::onAddModulator,
				     (wxObject *) (modnum++));

		itemid++;
	}



	refreshState();
	
	SetAutoLayout( TRUE );
	mainsizer->Fit( this );  
	mainsizer->SetSizeHints( this );  
	SetSizer( mainsizer );

	this->SetSizeHints(200,100);

}


void FTmodulatorDialog::refreshState()
{
	// first time only
	
	FTioSupport * iosup = FTioSupport::instance();

	FTprocessPath * procpath;
	for (int i=0; i < iosup->getActivePathCount(); ++i)
	{
	
		procpath = iosup->getProcessPath(i);
		if (procpath) {
			
			FTspectralEngine *engine = procpath->getSpectralEngine();

			engine->ModulatorAdded.connect( bind (slot (*this, &FTmodulatorDialog::onModulatorAdded), i));
			
// 			_channelLists[i]->DeleteAllItems();
			
// 			// init channel list
 			vector<FTmodulatorI*> modlist;
			modlist.clear();
 			engine->getModulators (modlist);

 			int n=0;
 			for (vector<FTmodulatorI*>::iterator iter=modlist.begin(); iter != modlist.end(); ++iter)
 			{
 				FTmodulatorI * mod = (*iter);
				FTmodulatorGui *modgui = new FTmodulatorGui(engine, mod, _channelScrollers[i], -1);

				mod->GoingAway.connect ( bind (slot (*this, &FTmodulatorDialog::onModulatorDeath), i));
				
				_modulatorGuis[mod] = modgui;

				_channelSizers[i]->Add(modgui, 0, wxEXPAND|wxALL, 1);

				cerr << "add modulator: " << mod->getName() << "  channel: " << i << endl;
				
// 				item.SetText (wxString::FromAscii (mod->getName().c_str()));
// 				item.SetData ((unsigned) mod);
// 				item.SetId (n++);
				
// 				_channelLists[i]->InsertItem(item);
 			}

			_channelSizers[i]->Layout();
			_channelScrollers[i]->SetScrollRate(10,30);
		}
	}


// 	if (_lastSelected >= 0 && _lastSelected < _targetList->GetItemCount()) {
// 		_targetList->SetItemState (_lastSelected, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
// 	}
	
}


void FTmodulatorDialog::onClose(wxCloseEvent & ev)
{

	if (!ev.CanVeto()) {

		Destroy();
	}
	else {
		ev.Veto();

		Show(false);
	}
}


void FTmodulatorDialog::onAddButton (wxCommandEvent &ev)
{
	wxWindow * source = (wxWindow *) ev.GetEventObject();

	_clickedChannel = ev.GetId() - ID_AddModulatorChannelBase;
	
	
	wxRect pos = source->GetRect();
	PopupMenu(_popupMenu, pos.x, pos.y + pos.height);
				
}

void FTmodulatorDialog::onAddModulator (wxCommandEvent &ev)
{
	int modnum = (int) ev.m_callbackUserData;

	FTmodulatorI * protomod = FTmodulatorManager::instance()->getModuleByIndex(modnum);

	cerr << "add modulator: " << (unsigned) protomod << " clicked chan: " << _clickedChannel <<  "  " << (unsigned) this << endl;
	
	if (protomod && _clickedChannel >= 0)
	{
		FTprocessPath * procpath = FTioSupport::instance()->getProcessPath(_clickedChannel);
		if (procpath) {
			FTspectralEngine *engine = procpath->getSpectralEngine();
			cerr << "add modulator for real: " << _clickedChannel <<  endl;

			FTmodulatorI * newmod = protomod->clone();
			newmod->initialize();
			
			engine->appendModulator (newmod);

			// refreshState();
		}
	}
}


void FTmodulatorDialog::OnIdle(wxIdleEvent &ev)
{

	if (_deadGuis.size() > 0) {

		for (list<FTmodulatorGui*>::iterator iter = _deadGuis.begin(); iter != _deadGuis.end(); ++iter) {
			(*iter)->Destroy();
		}
		
		_deadGuis.clear();
	}

	ev.Skip();
}

void FTmodulatorDialog::onModulatorDeath (FTmodulatorI * mod, int channel)
{
	cerr << "mod death: " << mod->getName() << " for channel: " << channel << endl;

	if (_modulatorGuis.find (mod) != _modulatorGuis.end())
	{
		cerr << "deleting modgui" << endl;
		FTmodulatorGui * modgui = _modulatorGuis[mod];
		_channelSizers[channel]->Remove (modgui);
		modgui->Show(false);
		_channelSizers[channel]->Layout();
		_channelScrollers[channel]->SetScrollbars(1,1,10,30);

		_deadGuis.push_back(modgui);
		_modulatorGuis.erase(mod);

		::wxWakeUpIdle();
	}
}

void FTmodulatorDialog::onModulatorAdded (FTmodulatorI * mod, int channel)
{
	FTprocessPath * procpath = FTioSupport::instance()->getProcessPath (channel);
	if (procpath) {
		FTspectralEngine *engine = procpath->getSpectralEngine();

		cerr << "mod added: " << mod->getName() << endl;
		
		FTmodulatorGui *modgui = new FTmodulatorGui(engine, mod, _channelScrollers[channel], -1);
		
		mod->GoingAway.connect ( bind (slot (*this, &FTmodulatorDialog::onModulatorDeath), channel));
		
		_modulatorGuis[mod] = modgui;
		
		_channelSizers[channel]->Add(modgui, 0, wxEXPAND|wxALL, 1);
		_channelSizers[channel]->Layout();
		_channelScrollers[channel]->SetScrollbars(1,1,10,30);
	}
}
