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


#include "FTmodulatorI.hpp"
#include <algorithm>

using namespace std;
using namespace PBD;

FTmodulatorI::FTmodulatorI(string name, nframes_t samplerate, unsigned int fftn)
	: _inited(false), _name(name), _confname("UNNAMED"), _userName(""), _bypassed(false),
	  _sampleRate(samplerate), _fftN(fftn)
{
}

FTmodulatorI::~FTmodulatorI()
{
}

void FTmodulatorI::goingAway(FTspectrumModifier * ft)
{
	LockMonitor pmlock(_specmodLock, __LINE__, __FILE__);
	_specMods.remove (ft);
}

	
void FTmodulatorI::addSpecMod (FTspectrumModifier * specmod)
{
	if (!specmod) return;
	LockMonitor pmlock(_specmodLock, __LINE__, __FILE__);

	if (find(_specMods.begin(), _specMods.end(), specmod) == _specMods.end())
	{
		_specMods.push_back (specmod);
	}
}

void FTmodulatorI::removeSpecMod (FTspectrumModifier * specmod)
{
	if (!specmod) return;
	LockMonitor pmlock(_specmodLock, __LINE__, __FILE__);
	_specMods.remove (specmod);
}

void FTmodulatorI::clearSpecMods ()
{
	LockMonitor pmlock(_specmodLock, __LINE__, __FILE__);
	_specMods.clear();
}

void FTmodulatorI::getSpecMods (SpecModList & mods)
{
	LockMonitor pmlock(_specmodLock, __LINE__, __FILE__);
	mods.insert (mods.begin(), _specMods.begin(), _specMods.end());
}
