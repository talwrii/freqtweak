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

#include "FTmodRandomize.hpp"
#include <cstdlib>
#include <cstdio>

using namespace std;
using namespace PBD;

FTmodRandomize::FTmodRandomize (nframes_t samplerate, unsigned int fftn)
	: FTmodulatorI ("Randomize", samplerate, fftn)
{
	_confname = "Randomize";
}

FTmodRandomize::FTmodRandomize (const FTmodRandomize & other)
	: FTmodulatorI ("Randomize", other._sampleRate, other._fftN)
{
	_confname = "Randomize";
}

void FTmodRandomize::initialize()
{
	_lastframe = 0;
	
	_rate = new Control (Control::FloatType, "Rate", "Hz");
	_rate->_floatLB = 0.0;
	_rate->_floatUB = 20.0;
	_rate->setValue (5.0f);
	_controls.push_back (_rate);

	srand(0);
	
	_inited = true;
}

FTmodRandomize::~FTmodRandomize()
{
	if (!_inited) return;

	// need a going away signal
	
	_controls.clear();

	delete _rate;
}

void FTmodRandomize::modulate (nframes_t current_frame, fft_data * fftdata, unsigned int fftn, sample_t * timedata, nframes_t nframes)
{
	TentativeLockMonitor lm (_specmodLock, __LINE__, __FILE__);

	if (!lm.locked() || !_inited || _bypassed) return;

	float rate = 1.0;
	float ub,lb;
	float * filter;
	unsigned int len;
	
	_rate->getValue (rate);

	double samps = _sampleRate / rate;

	if (current_frame != _lastframe &&
	    (nframes_t) (current_frame/samps) != (nframes_t) ((current_frame + nframes)/samps))
	{
		// if this range falls  across an integral boundary divisable by the mod rate
		// fprintf (stderr, "randomize at %lu :  samps=%g  s*c=%g  s*e=%g \n", (unsigned long) current_frame, samps, (current_frame/samps), ((current_frame + nframes)/samps) );
		
		for (SpecModList::iterator iter = _specMods.begin(); iter != _specMods.end(); ++iter)
		{
			FTspectrumModifier * sm = (*iter);
			if (sm->getBypassed()) continue;

			filter = sm->getValues();
			sm->getRange(lb, ub);
			len = sm->getLength();
			
			// crap random
			for (unsigned int i=0; i < len; ++i) {
				filter[i] = lb + (float) ((ub-lb) * rand() / (RAND_MAX+1.0));
			}

			sm->setDirty(true);
		}

		_lastframe = current_frame;
	}
}
