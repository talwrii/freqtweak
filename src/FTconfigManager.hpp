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
#include <string>
#include <list>
using namespace std;

class FTspectralEngine;
class FTspectrumModifier;
class XMLNode;

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

	void writeFilter (FTspectrumModifier *specmod, wxTextFile & tf);

	void loadFilter (FTspectrumModifier *specmod, wxTextFile & tf);

	XMLNode* find_named_node (const XMLNode * node, string name);
	
	wxString _basedir;

	class LinkCache {
	public:
		LinkCache (unsigned int src, unsigned int dest, unsigned int modn, unsigned int filtn)
			: source_chan(src), dest_chan(dest), mod_n(modn), filt_n(filtn) {}
		
		unsigned int source_chan;
		unsigned int dest_chan;
		unsigned int mod_n;
		unsigned int filt_n;

	};

	list<LinkCache> _linkCache;
};

WX_DECLARE_LIST(wxString, FTstringList);


#endif
