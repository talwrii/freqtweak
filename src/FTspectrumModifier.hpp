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

#ifndef __FTSPECTRUMMODIFIER_HPP__
#define __FTSPECTRUMMODIFIER_HPP__

#include "FTtypes.hpp"

#include <wx/wx.h>

class FTspecModList;


class FTspectrumModifier
{
  public:

	enum ModifierType
	{
		GAIN_MODIFIER = 0,
		TIME_MODIFIER,
		UNIFORM_MODIFIER,
		RATIO_MODIFIER,
		FREQ_MODIFIER,
		DB_MODIFIER,
	};

	FTspectrumModifier(ModifierType mtype, SpecModType smtype, int length=512, float initval=0.0);
	virtual ~FTspectrumModifier();

	void setId (int id) { _id = id; }
	int getId () { return _id; }
	
	void setLength(int length);
	int getLength() { return _length; }

	
	float * getValues();

	ModifierType getModifierType() { return _modType; }
	SpecModType getSpecModifierType() { return _specmodType; }

	FTspectrumModifier * getLink() { return _linkedTo; }
	bool link (FTspectrumModifier *specmod);
	void unlink (bool unlinksources=true);

	void setRange(float min, float max) { _min = min; _max = max; }
	void getRange(float &min, float &max) { min = _min; max = _max; }
	float getMin() const { return _min;}
	float getMax() const { return _max;}

	// resets all bins to constructed value
	void reset();

	void copy (FTspectrumModifier *specmod);
	
	FTspecModList & getLinkedFrom() { return *_linkedFrom; }
	
  protected:

	void addedLinkFrom (FTspectrumModifier * specmod);
	void removedLinkFrom (FTspectrumModifier * specmod);
	
	ModifierType _modType;
	SpecModType _specmodType;
	
	// might point to a linked value array
	float * _values;

	float * _tmpvalues; // used for copying
	
	int _length;

	FTspectrumModifier *_linkedTo;

	float _min, _max;
	float _initval;

	FTspecModList * _linkedFrom;

	int _id;
};

WX_DECLARE_LIST(FTspectrumModifier, FTspecModList);


#endif
