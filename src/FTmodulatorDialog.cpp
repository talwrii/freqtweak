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

enum {
	ID_AddButton=8000,
	ID_PopupMenu,
	ID_EditMenuItem,
	ID_RemoveMenuItem,
	ID_ChannelList
};


enum {
	ID_AddModulatorBase = 9000
};

BEGIN_EVENT_TABLE(FTmodulatorDialog, wxFrame)

	EVT_CLOSE(FTmodulatorDialog::onClose)

	
	EVT_SIZE (FTmodulatorDialog::onSize)
	EVT_PAINT (FTmodulatorDialog::onPaint)

	EVT_LIST_ITEM_SELECTED(ID_ChannelList, FTmodulatorDialog::onItemSelected)			
	EVT_LIST_ITEM_RIGHT_CLICK(ID_ChannelList, FTmodulatorDialog::onItemRightClick)
	
	
END_EVENT_TABLE()


FTmodulatorDialog::FTmodulatorDialog(FTmainwin * parent, wxWindowID id,
				     const wxString & title,
				     const wxPoint& pos,
				     const wxSize& size,
				     long style,
				     const wxString& name )

	: wxFrame(parent, id, title, pos, size, style, name),
	 _clickedMod(0), _clickedChannel(-1), _mainwin(parent)
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

		for (int i=0; i < _channelCount; ++i)
		{
			_channelLists[i]->GetClientSize(&width, &height);
			_channelLists[i]->SetColumnWidth(0, width);
		}
		
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
	
	for (int i=0; i < iosup->getActivePathCount(); ++i)
	{
		procpath = iosup->getProcessPath(i);
		if (!procpath) break;
		
		wxBoxSizer * sourceSizer = new wxBoxSizer(wxVERTICAL);

		statText = new wxStaticText (this, -1, wxString::Format(wxT("%s %d"), wxT("Channel"), i+1),  wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE);
		sourceSizer->Add(statText, 0, wxALL|wxEXPAND|wxALIGN_CENTRE_VERTICAL|wxALIGN_CENTRE, 3);
		
		
		_channelLists[i] = new wxListCtrl (this, ID_ChannelList, wxDefaultPosition, wxSize(-1,200), wxLC_REPORT|wxSUNKEN_BORDER|wxLC_SINGLE_SEL);
		_channelLists[i]->InsertColumn(0, wxT("Modulators"));

		// we want to get his mouse up events
		Connect( ID_ChannelList,  wxEVT_RIGHT_UP,
				     (wxObjectEventFunction) (wxEventFunction) (wxMouseEventFunction)
				     &FTmodulatorDialog::onRightClick,
				     (wxObject *) _channelCount);

		
		sourceSizer->Add (_channelLists[i], 1, wxEXPAND|wxALL, 2);

		_chanlistSizer->Add (sourceSizer, 1, wxEXPAND|wxALL, 4);
		_channelCount++;
	}		


	mainsizer->Add (_chanlistSizer, 1, wxEXPAND|wxALL, 2);
	
	wxMenuItem * item;
	_popupMenu = new wxMenu();

	_editMenuItem = new wxMenuItem(_popupMenu, ID_EditMenuItem, wxT("Edit..."));

	_removeMenuItem = new wxMenuItem(_popupMenu, ID_RemoveMenuItem, wxT("Remove"));

	_popupMenu->AppendSeparator();

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

	wxListItem item;
	item.SetColumn(0);
	item.SetMask (wxLIST_MASK_TEXT|wxLIST_MASK_DATA);
	
	FTioSupport * iosup = FTioSupport::instance();

	FTprocessPath * procpath;
	for (int i=0; i < iosup->getActivePathCount(); ++i)
	{
	
		procpath = iosup->getProcessPath(i);
		if (procpath) {
			
			FTspectralEngine *engine = procpath->getSpectralEngine();
			
			_channelLists[i]->DeleteAllItems();
			
			// init channel list
			vector<FTmodulatorI*> modlist;
			engine->getModulators (modlist);

			int n=0;
			for (vector<FTmodulatorI*>::iterator iter=modlist.begin(); iter != modlist.end(); ++iter)
			{
				FTmodulatorI * mod = (*iter);
				
				item.SetText (wxString::FromAscii (mod->getName().c_str()));
				item.SetData ((unsigned) mod);
				item.SetId (n++);
				
				_channelLists[i]->InsertItem(item);
			}
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


void FTmodulatorDialog::onItemSelected (wxListEvent &ev)
{
	wxObject *source = ev.GetEventObject(); 

	for (int i=0; i < _channelCount; ++i)
	{
		if (source == _channelLists[i]) {

			FTmodulatorI * mod = (FTmodulatorI *) ev.GetData();
			if (mod) {
				cerr << "Item selected " << mod->getName() << endl;

			}
			
		}
		else {
			// deselect all
			int itemi = -1;
			for ( ;; )
			{
				itemi = _channelLists[i]->GetNextItem(itemi, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
				if ( itemi == -1 )
					break;
				_channelLists[i]->SetItemState(itemi, 0, wxLIST_STATE_SELECTED);
			}
		}
	}

	ev.Skip();
}

void FTmodulatorDialog::onRightClick (wxMouseEvent &ev)
{
	int channum = (int) ev.m_callbackUserData;

	cerr << "On reigt click: " << channum << endl;
	
	// popup basic menu
// 	_clickedChannel = channum;
// 	_channelLists[channum]->PopupMenu(_popupMenu, 0, 0);

// 	_clickedChannel = -1;
}


void FTmodulatorDialog::onItemRightClick (wxListEvent &ev)
{
	wxObject *source = ev.GetEventObject(); 

	for (int i=0; i < _channelCount; ++i)
	{
		if (source == _channelLists[i]) {

			// _channelLists[i]->SetItemState(ev.GetIndex(), 1, wxLIST_STATE_SELECTED);
			
			FTmodulatorI * mod = (FTmodulatorI *) ev.GetData();
			if (mod) {

				_clickedMod = mod;
				_clickedChannel = i;
				
				// add stuff to menu
				_popupMenu->Insert (0, _removeMenuItem);
				_popupMenu->Insert (0, _editMenuItem);

				wxPoint pos = ::wxGetMousePosition();
				pos = _channelLists[i]->ScreenToClient(pos);
				
				cerr << "Item Right Clicked: " << mod->getName() << "  " << _clickedChannel << "  " << (unsigned) this << endl;

				_channelLists[i]->PopupMenu(_popupMenu, pos.x, pos.y - 12);
				
				_clickedMod  = 0;
				// _clickedChannel = -1;
				_popupMenu->Remove (_editMenuItem);
				_popupMenu->Remove (_removeMenuItem);
			}

			break;
		}
	}
	ev.Skip();
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

			engine->appendModulator (protomod->clone());

			refreshState();
		}
	}
}


/*
void FTmodulatorDialog::onAddButton(wxCommandEvent & ev)
{
	// append selected procmods from source
//
	wxListItem item;
	item.SetColumn(0);
	item.SetMask (wxLIST_MASK_TEXT|wxLIST_MASK_DATA);
	
	
	long itemi = -1;
	bool didsomething = false;

	for ( ;; )
	{
		itemi = _sourceList->GetNextItem(itemi,
					     wxLIST_NEXT_ALL,
					     wxLIST_STATE_SELECTED);
		if ( itemi == -1 )
			break;

		FTprocI * proc = (FTprocI *) _sourceList->GetItemData(itemi); 

		if (proc) {
			item.SetText (wxString::FromAscii (proc->getName().c_str()));
			item.SetData ((unsigned)proc);
			item.SetId (_targetList->GetItemCount());
			
			_targetList->InsertItem(item);

			_actions.push_back (ModAction (proc, -1 , _targetList->GetItemCount(), false));

			didsomething = true;
		}
	}	

	if (didsomething) {
		if (_autoCheck->GetValue()) {
			onCommit(ev);
		}
		else {
			_modifiedText->SetLabel (wxT("* modified *"));
		}
	}
}
*/
