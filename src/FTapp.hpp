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

#ifndef __FTPAPPHPP__
#define __FTPAPPHPP__


#include <wx/wx.h>

#include "FTtypes.hpp"

class FTmainwin;
class FTprocessPath;

class FTapp : public wxApp
{
	
  public: 
	// override base class virtuals
	// ----------------------------
	FTapp();
	
	// this one is called on application startup and is a good place for the app
	// initialization (doing it here and not in the ctor allows to have an error
	// return: if OnInit() returns false, the application terminates)
	virtual bool OnInit();
	
	FTmainwin * getMainwin() { return _mainwin; }

	//FTprocessPath * getProcessPath(int index);
	
  protected:
	FTmainwin * _mainwin;



};


DECLARE_APP(FTapp);


#endif
