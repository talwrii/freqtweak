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

#include <wx/wx.h>

#include "FTmodulatorGui.hpp"
#include "FTmodulatorI.hpp"
#include "FTspectralEngine.hpp"
#include "FTprocI.hpp"

using namespace SigC;

enum
{
	ID_RemoveButton = 8000,
	ID_AttachButton,
	ID_DetachAll,
	ID_AttachAll
};


enum {
	ID_ControlBase = 10000,
	ID_SpecModBase = 11000
};

class FTmodControlObject : public wxObject
{
  public:
	FTmodControlObject(FTmodulatorI::Control *ctrl) : control(ctrl) {}
	
	FTmodulatorI::Control * control;
};

class FTspecmodObject : public wxObject
{
  public:
	FTspecmodObject(int mi, int fi) : modIndex(mi), filtIndex(fi) {}
	
	int modIndex;
	int filtIndex;
};


BEGIN_EVENT_TABLE(FTmodulatorGui, wxPanel)

	EVT_BUTTON(ID_RemoveButton, FTmodulatorGui::onRemoveButton)
	EVT_BUTTON(ID_AttachButton, FTmodulatorGui::onAttachButton)

	EVT_MENU (ID_AttachAll, FTmodulatorGui::onAttachMenu)
	EVT_MENU (ID_DetachAll, FTmodulatorGui::onAttachMenu)
	
END_EVENT_TABLE()

FTmodulatorGui::FTmodulatorGui (FTspectralEngine * engine, FTmodulatorI *mod, wxWindow *parent, wxWindowID id,
				const wxPoint& pos,
				const wxSize& size,
				long style ,
				const wxString& name)

	: wxPanel(parent, id, pos, size, style, name),
	  _modulator (mod), _engine(engine), _popupMenu(0)
{

	init();

}

FTmodulatorGui::~FTmodulatorGui()
{
	cerr << "MODGUI destructor" << endl;
}


void FTmodulatorGui::init()
{

	wxBoxSizer * mainSizer = new wxBoxSizer(wxVERTICAL);


	wxBoxSizer * topSizer = new wxBoxSizer(wxHORIZONTAL);

//	wxBoxSizer *tmpsizer, *tmpsizer2;
	wxStaticText * stattext;
	
	stattext = new wxStaticText(this, -1, wxString::FromAscii(_modulator->getName().c_str()),
				    wxDefaultPosition, wxSize(-1, -1));
	topSizer->Add (stattext, 0, wxALL|wxALIGN_CENTRE_VERTICAL, 2);


	_nameText = new wxTextCtrl (this, -1, wxString::FromAscii(_modulator->getUserName().c_str()));
	topSizer->Add (_nameText, 1, wxALL|wxALIGN_CENTRE_VERTICAL, 2);


	wxButton * attachButton = new wxButton(this, ID_AttachButton, wxT("Attach..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	topSizer->Add (attachButton, 0, wxALL|wxALIGN_CENTRE_VERTICAL, 2);

	wxButton * removeButton = new wxButton(this, ID_RemoveButton, wxT("X"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	topSizer->Add (removeButton, 0, wxALL|wxALIGN_CENTRE_VERTICAL, 2);

	
	
	mainSizer->Add (topSizer, 0, wxEXPAND|wxALL, 2);

	
	wxBoxSizer * controlSizer = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer * rowsizer;
	
	int ctrlid = ID_ControlBase;
	
	// get controls
	FTmodulatorI::ControlList controls;
	_modulator->getControls (controls);

	for (FTmodulatorI::ControlList::iterator ctrliter = controls.begin(); ctrliter != controls.end(); ++ctrliter)
	{
		FTmodulatorI::Control * ctrl = (FTmodulatorI::Control *) *ctrliter;
		
		if (ctrl->getType() == FTmodulatorI::Control::BooleanType) {
			// make a checkbox

			wxCheckBox * checkb = new wxCheckBox(this, ctrlid,
							     wxString::Format(wxT("%s [%s]"), ctrl->getName().c_str(), ctrl->getUnits().c_str()));

			Connect( ctrlid,  wxEVT_COMMAND_CHECKBOX_CLICKED,
				 (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction)
				 &FTmodulatorGui::onCheckboxChanged,
				 new FTmodControlObject(ctrl));

			controlSizer->Add (checkb, 0, wxEXPAND|wxALL, 2);
		}
		else if (ctrl->getType() == FTmodulatorI::Control::IntegerType) {
			// make a slider and spinbox for now
			rowsizer = new wxBoxSizer(wxHORIZONTAL);
			stattext = new wxStaticText(this, -1, wxString::Format(wxT("%s [%s]"),
									       ctrl->getName().c_str(),
									       ctrl->getUnits().c_str()));

			rowsizer->Add (stattext, 0, wxALL|wxALIGN_CENTRE_VERTICAL, 2);

			int currval = 0;
			int minval=0,maxval=1;
			ctrl->getValue(currval);
			ctrl->getBounds(minval, maxval);
			
			wxSlider * slider = new wxSlider(this, ctrlid, currval, minval, maxval);

			rowsizer->Add (slider, 1, wxALL|wxALIGN_CENTRE_VERTICAL, 2);
			
			Connect( ctrlid,  wxEVT_SCROLL_THUMBTRACK,
				 (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction) (wxScrollEventFunction)
				 &FTmodulatorGui::onSliderChanged,
				 new FTmodControlObject(ctrl));

			
			
			controlSizer->Add (rowsizer, 0, wxEXPAND|wxALL, 2);
			
		}
		else if (ctrl->getType() == FTmodulatorI::Control::FloatType) {
			// make a slider and spinbox for now
			rowsizer = new wxBoxSizer(wxHORIZONTAL);
			stattext = new wxStaticText(this, -1, wxString::Format(wxT("%s [%s]"),
									       ctrl->getName().c_str(),
									       ctrl->getUnits().c_str()));

			rowsizer->Add (stattext, 0, wxALL|wxALIGN_CENTRE_VERTICAL, 2);

			float currval = 0;
			float minval=0,maxval=1;
			ctrl->getValue(currval);
			ctrl->getBounds(minval, maxval);

			// we'll always have slider values between 0 and 100 for now

			currval = ((currval-minval) / (maxval - minval)) * 100;
			
			wxSlider * slider = new wxSlider(this, ctrlid, (int) currval, 0, 100);

			rowsizer->Add (slider, 1, wxALL|wxALIGN_CENTRE_VERTICAL, 2);
			
			Connect( ctrlid,  wxEVT_SCROLL_THUMBTRACK,
				 (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction) (wxScrollEventFunction)
				 &FTmodulatorGui::onSliderChanged,
				 new FTmodControlObject(ctrl));

			
			
			controlSizer->Add (rowsizer, 0, wxEXPAND|wxALL, 2);
			
		}
		else if (ctrl->getType() == FTmodulatorI::Control::EnumType) {
			// use a wxChoice

			rowsizer = new wxBoxSizer(wxHORIZONTAL);
			stattext = new wxStaticText(this, -1, wxString::Format(wxT("%s [%s]"),
									       ctrl->getName().c_str(),
									       ctrl->getUnits().c_str()));

			rowsizer->Add (stattext, 0, wxALL|wxALIGN_CENTRE_VERTICAL, 2);

		        string currval;
			list<string> vallist;

			ctrl->getValue(currval);
			ctrl->getEnumStrings (vallist);
			
			wxChoice * choice = new wxChoice(this, ctrlid);

			for (list<string>::iterator citer = vallist.begin(); citer != vallist.end(); ++citer) {
				choice->Append (wxString::FromAscii((*citer).c_str()));
			}
			choice->SetStringSelection (wxString::FromAscii(currval.c_str()));
			
			rowsizer->Add (choice, 1, wxALL|wxALIGN_CENTRE_VERTICAL, 2);
			
			Connect( ctrlid,  wxEVT_COMMAND_CHOICE_SELECTED,
				 (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction)
				 &FTmodulatorGui::onChoiceChanged,
				 new FTmodControlObject(ctrl));

			
			controlSizer->Add (rowsizer, 0, wxEXPAND|wxALL, 2);
			
		}
		
		ctrlid++;
	}

	//controlSizer->Add(new wxButton(this, -1, wxT("BIG TEST"), wxDefaultPosition, wxSize(120, 90)),
	//		  1, wxEXPAND|wxALL, 2);

	
	mainSizer->Add (controlSizer, 1, wxEXPAND|wxALL, 2);
	
	SetAutoLayout( TRUE );
	mainSizer->Fit( this );  
	mainSizer->SetSizeHints( this );  
	SetSizer( mainSizer );
}


void FTmodulatorGui::refreshMenu()
{
	if (_popupMenu) {
		delete _popupMenu;
	}
	
	_popupMenu = new wxMenu();

	int itemid = ID_SpecModBase;

	_popupMenu->Append (ID_AttachAll, wxT("Attach All"));
	_popupMenu->Append (ID_DetachAll, wxT("Detach All"));

	_popupMenu->AppendSeparator();

	// go through all the spectrum modifiers in the engine
	vector<FTprocI *> procmods;
	_engine->getProcessorModules (procmods);

	for (unsigned int n=0; n < procmods.size(); ++n)
	{
		FTprocI *pm = procmods[n];
		vector<FTspectrumModifier *> filts;
		pm->getFilters (filts);

		for (unsigned int m=0; m < filts.size(); ++m)
		{
		
			_popupMenu->AppendCheckItem (itemid, wxString::FromAscii (filts[m]->getName().c_str()));

			if (_modulator->hasSpecMod (filts[m])) {
				_popupMenu->Check (itemid, true);
			}
			
			Connect( itemid,  wxEVT_COMMAND_MENU_SELECTED,
				 (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction)
				 &FTmodulatorGui::onAttachMenu,
				 (wxObject *) new FTspecmodObject(n, m));
			
			itemid++;
		}
	}

}

void FTmodulatorGui::onAttachMenu (wxCommandEvent & ev)
{
	int id = ev.GetId();

	if (id == ID_AttachAll) {
		// go through every one and add it
		vector<FTprocI *> procmods;
		_engine->getProcessorModules (procmods);
		
		for (unsigned int n=0; n < procmods.size(); ++n)
		{
			FTprocI *pm = procmods[n];
			vector<FTspectrumModifier *> filts;
			pm->getFilters (filts);
			
			for (unsigned int m=0; m < filts.size(); ++m)
			{
				_modulator->addSpecMod (filts[m]);
			}
		}

	}
	else if (id == ID_DetachAll) {
		_modulator->clearSpecMods();
	}
	else {
		// a filter menu item
		FTspecmodObject * smo = (FTspecmodObject *) ev.m_callbackUserData;
		FTprocI * procmod;
		FTspectrumModifier * specmod;
		
		if (smo
		    && (procmod = _engine->getProcessorModule(smo->modIndex))
		    && (specmod = procmod->getFilter(smo->filtIndex)))
		{
			if (ev.IsChecked()) {
				_modulator->addSpecMod (specmod);
			}
			else {
				_modulator->removeSpecMod (specmod);
			}
		}
	}

}

void FTmodulatorGui::onRemoveButton (wxCommandEvent & ev)
{
	// remove our own dear mod
	cerr << "on remove" << endl;

	_engine->removeModulator (_modulator);

	cerr << "post remove" << endl;

	_modulator = 0;
}

void FTmodulatorGui::onAttachButton (wxCommandEvent & ev)
{

	wxWindow * source = (wxWindow *) ev.GetEventObject();
	
	wxRect pos = source->GetRect();

	refreshMenu();
	
	PopupMenu(_popupMenu, pos.x, pos.y + pos.height);
		
}



void FTmodulatorGui::onSliderChanged(wxScrollEvent &ev)
{
	wxSlider * slider = (wxSlider *) ev.GetEventObject();

	FTmodControlObject * obj = (FTmodControlObject *) ev.m_callbackUserData;
	FTmodulatorI::Control * ctrl;

	if (obj && (ctrl = obj->control)) {
	
		
		if (ctrl->getType() == FTmodulatorI::Control::IntegerType) {
			int currval = slider->GetValue();

			ctrl->setValue(currval);
			cerr << "slider int changed for " << ctrl->getName() <<  ": new val = " << currval << endl;

		}
		else if (ctrl->getType() == FTmodulatorI::Control::FloatType) {
			float minval,maxval;
			ctrl->getBounds(minval, maxval);
			float currval = (slider->GetValue() / 100.0) * (maxval - minval)  + minval;

			ctrl->setValue (currval);
			cerr << "slider float changed for " << ctrl->getName() <<  ": new val = " << currval << endl;
		}
	}
	
}

void FTmodulatorGui::onChoiceChanged(wxCommandEvent &ev)
{
	wxChoice * choice = (wxChoice *) ev.GetEventObject();

	FTmodControlObject * obj = (FTmodControlObject *) ev.m_callbackUserData;
	FTmodulatorI::Control * ctrl;

	cerr << "choice" << endl;
	
	if (obj && (ctrl = obj->control)) {

		
		ctrl->setValue (choice->GetStringSelection().c_str());

		cerr << " choice changed for " << ctrl->getName() <<  ": new val = " << choice->GetStringSelection().c_str() << endl;
		
	}

}

void FTmodulatorGui::onCheckboxChanged(wxCommandEvent &ev)
{
	

}
