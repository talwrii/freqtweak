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

#include <stdio.h>
#include <string.h>

#include "FTspectrumModifier.hpp"
#include "FTtypes.hpp"

#include <wx/listimpl.cpp>



WX_DEFINE_LIST(FTspecModList)


FTspectrumModifier::FTspectrumModifier(FTspectrumModifier::ModifierType mtype, SpecModType smtype, int length, float initval)
	:  _modType(mtype), _specmodType(smtype), _values(0), _length(length), _linkedTo(0), _initval(initval)
	, _id(0)

{
	_values = new float[FT_MAX_FFT_SIZE/2];
	_tmpvalues = new float[FT_MAX_FFT_SIZE/2];

	for (int i=0; i < FT_MAX_FFT_SIZE/2; i++)
	{
		_values[i] = initval;
	}

	_linkedFrom = new FTspecModList();
}

FTspectrumModifier::~FTspectrumModifier()
{
	printf ("delete specmod\n");
	delete [] _values;
	delete [] _tmpvalues;
	
	delete _linkedFrom;
}


void FTspectrumModifier::setLength(int length)
{
	int origlen = _length;
	
	if (length < FT_MAX_FFT_SIZE/2) {
		_length = length;

		// now resample existing values into new length
		if (! _linkedTo)
		{
			float scale = origlen / (float) length;
			for (int i=0; i < length; i++) {
				_tmpvalues[i] = _values[(int)(i*scale)];
			}

			memcpy(_values, _tmpvalues, length * sizeof(float));
		}
	}
}


bool FTspectrumModifier::link (FTspectrumModifier *specmod)
{
	unlink(false);
	
	// do a cycle check
	FTspectrumModifier * spec = specmod;
	while (spec)
	{
		if (spec == this) {
			// cycle!!! bad!!
			return false;
		}
		spec = spec->getLink();
	}

	_linkedTo = specmod;

	specmod->addedLinkFrom(this);
	
	return true;
}

void FTspectrumModifier::unlink (bool unlinksources)
{
	if (_linkedTo) {
		_linkedTo->removedLinkFrom ( this );

		// copy their values into our internal
		copy (_linkedTo);
	}
	_linkedTo = 0;

	if (unlinksources) {
		// now unlink all who are linked to me
		wxFTspecModListNode *node = _linkedFrom->GetFirst();
		while (node)
		{
			FTspectrumModifier *specmod = node->GetData();
			specmod->unlink(false);
			// the list will be modified underneath us, so....
			if ((node = _linkedFrom->Find((unsigned) specmod))) {
				// THIS SHOULDNT HAPPEN BUT IS HERE ANYWAY
				printf ("blah link!\n");
				_linkedFrom->DeleteNode(node);
			}
			
			node = _linkedFrom->GetFirst();
			
		}
	}
}

void FTspectrumModifier::addedLinkFrom (FTspectrumModifier * specmod)
{
	// called from link()

	if (! _linkedFrom->Find ((unsigned) specmod)) {
		_linkedFrom->Append ( (unsigned) specmod, specmod);
	}
	
}

void FTspectrumModifier::removedLinkFrom (FTspectrumModifier * specmod)
{
	// called from unlink()
	wxFTspecModListNode * node =  _linkedFrom->Find ((unsigned) specmod);

	if (node) {
		_linkedFrom->DeleteNode (node);
	}
	
}


float * FTspectrumModifier::getValues()
{
	if (_linkedTo) {
		return _linkedTo->getValues();
	}

	return _values;
}

void FTspectrumModifier::reset()
{
	float *data;

	if (getModifierType() == FREQ_MODIFIER)
	{
		float incr = (_max - _min) / _length;
		float val = _min;
		
		if (_linkedTo) {
			data = _linkedTo->getValues();
			for (int i=0; i < _length; i++) {
				_values[i] = data[i] = val;
				val += incr;
			}
		}
		else {
			for (int i=0; i < _length; i++) {
				_values[i] = val;
				val += incr;
			}
		}
	}
	else {
		if (_linkedTo) {
			data = _linkedTo->getValues();
			for (int i=0; i < _length; i++) {
				_values[i] = data[i] = _initval;
			}
		}
		else {
			for (int i=0; i < _length; i++) {
				_values[i] = _initval;
			}
		}
	}
}


void FTspectrumModifier::copy (FTspectrumModifier *specmod)
{
	// this always copies into our internal values
	if (!specmod) return;
	
	float * othervals = specmod->getValues();

	memcpy (_values, othervals, _length * sizeof(float));
}
