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

#ifndef __FTCONFIGMANAGER_HPP__
#define __FTCONFIGMANAGER_HPP__


#include <wx/wx.h>
#include <wx/textfile.h>
#include "FTtypes.hpp"

class FTspectralManip;
class FTspectrumModifier;

class FTstringList;

class FTconfigManager
{
   public:
	
	FTconfigManager(const char *basedir = 0);
	virtual ~FTconfigManager();


	bool storeSettings (const char * name);

	bool loadSettings (const char * name, bool restore_ports=false);

	FTstringList * getSettingsNames();

   protected:

	void createFilterFiles(FTspectralManip *manip, wxString &dirname, int i);
	void writeFilter (FTspectrumModifier *specmod, wxTextFile & tf);

	void loadFilterFiles(FTspectralManip *manip, wxString &dirname, int i);
	void loadFilter (FTspectrumModifier *specmod, wxTextFile & tf);

	void modifySetting (FTspectralManip *manip, int id, wxString &key, wxString &value, bool restoreports=false);
	
	wxString _basedir;

};

WX_DECLARE_LIST(wxString, FTstringList);


#endif
