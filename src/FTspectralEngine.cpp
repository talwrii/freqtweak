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



#ifdef HAVE_SFFTW_H
#include <sfftw.h>
#else
#include <fftw.h>
#endif

#ifdef HAVE_SRFFTW_H
#include <srfftw.h>
#else
#include <rfftw.h>
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "FTtypes.hpp"
#include "FTspectralEngine.hpp"
#include "FTprocessPath.hpp"
#include "RingBuffer.hpp"
#include "FTspectrumModifier.hpp"
#include "FTioSupport.hpp"
#include "FTupdateToken.hpp"
#include "FTprocI.hpp"


const int FTspectralEngine::_windowStringCount = 4;
const char * FTspectralEngine::_windowStrings[] = {
	"Hanning", "Hamming", "Blackman", "Rectangle"
	};

const int FTspectralEngine::_fftSizeCount = 9;
const int FTspectralEngine::_fftSizes[] = {
	32, 64, 128, 256, 512, 1024, 2048, 4096, 8192
};


// in samples  (about 3 seconds at 44100)

#define FT_MAX_DELAYSAMPLES (1 << 19)


FTspectralEngine::FTspectralEngine()
	: _fftN (512), _windowing(FTspectralEngine::WINDOW_HANNING)
	, _oversamp(4), _averages(8), _fftnChanged(false)
	, _inputGain(1.0), _mixRatio(1.0), _bypassFlag(false), _mutedFlag(false), _updateSpeed(SPEED_MED)
	  , _id(0), _updateToken(0), _maxDelay(2.5)
	, _currInAvgIndex(0), _currOutAvgIndex(0), _avgReady(false)
{
	_inwork = new fftw_real [FT_MAX_FFT_SIZE];
	_outwork = new fftw_real [FT_MAX_FFT_SIZE];
	_winwork = new fftw_real [FT_MAX_FFT_SIZE];

	
	_accum = new fftw_real [2 * FT_MAX_FFT_SIZE];

	memset((char *) _accum, 0, 2*FT_MAX_FFT_SIZE*sizeof(fftw_real));
	memset((char *) _inwork, 0, FT_MAX_FFT_SIZE*sizeof(fftw_real));

	
	_inputPowerSpectra = new fftw_real [FT_MAX_FFT_SIZE/2];
        _outputPowerSpectra = new fftw_real [FT_MAX_FFT_SIZE/2];

	_runningInputPower = new fftw_real [FT_MAX_FFT_SIZE/2];
	_runningOutputPower = new fftw_real [FT_MAX_FFT_SIZE/2];
	
	
	_fftPlan = rfftw_create_plan(_fftN, FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE);		
	_ifftPlan = rfftw_create_plan(_fftN, FFTW_COMPLEX_TO_REAL, FFTW_ESTIMATE);		

	_sampleRate = FTioSupport::instance()->getSampleRate();

	//setMaxelay(_maxDelay);
	
	// window init
	createWindowVectors();


	_averages = (int) (_oversamp * _updateSpeed * 512/(float)_fftN); // magic?


	// create processing modules
	// TODO: do this dynamically


	
}

FTspectralEngine::~FTspectralEngine()
{
	delete [] _inwork;
	delete [] _outwork;
	delete [] _winwork;

	
	delete [] _accum;

	
	delete [] _inputPowerSpectra;
        delete [] _outputPowerSpectra;

	delete [] _runningInputPower;
	delete [] _runningOutputPower;
	
	

	// destroy window vectors
	for(int i = 0; i < NUM_WINDOWS; i++)
	{
		delete [] _mWindows[i];
	}
	

	for (vector<FTprocI*>::iterator iter = _procModules.begin();
	     iter != _procModules.end(); ++iter)
	{
		delete (*iter);
	}
	
	rfftw_destroy_plan (_fftPlan);
	rfftw_destroy_plan (_ifftPlan);
	
	
}


void FTspectralEngine::getProcessorModules (vector<FTprocI *> & modules)
{
	modules.clear();
	
	for (vector<FTprocI*>::iterator iter = _procModules.begin();
	     iter != _procModules.end(); ++iter)
	{
		modules.push_back (*iter);
	}
}

FTprocI * FTspectralEngine::getProcessorModule ( unsigned int num)
{
	if (num < _procModules.size()) {
		return _procModules[num];
	}

	return 0;
}

void FTspectralEngine::insertProcessorModule (FTprocI * procmod, unsigned int index)
{
	if (!procmod) return;
	
	vector<FTprocI*>::iterator iter = _procModules.begin();

	for (unsigned int n=0; n < index && iter!=_procModules.end(); ++n) {
		++iter;
	}

	procmod->setOversamp (_oversamp);
	procmod->setFFTsize (_fftN);
	procmod->setSampleRate (_sampleRate);
	
	_procModules.insert (iter, procmod);
}

void FTspectralEngine::appendProcessorModule (FTprocI * procmod)
{
	if (!procmod) return;

	procmod->setOversamp (_oversamp);
	procmod->setFFTsize (_fftN);
	procmod->setSampleRate (_sampleRate);
	
	_procModules.push_back (procmod);
}

void FTspectralEngine::moveProcessorModule (unsigned int from, unsigned int to)
{
	// both indexes refer to current positions within the list
	
	vector<FTprocI*>::iterator iter = _procModules.begin();

	for (unsigned int n=0; n < from && iter!=_procModules.end(); ++n) {
		++iter;
	}

	if (iter == _procModules.end())
		return;

	// remove from
	FTprocI * fproc = (*iter);
	_procModules.erase (iter);

	iter = _procModules.begin();
	
	if (to >= from) {
		// need to go one less
		for (unsigned int n=0; n < to && iter!=_procModules.end(); ++n) {
			++iter;
		}
	}
	else {
		for (unsigned int n=0; n < to && iter!=_procModules.end(); ++n) {
			++iter;
		}
	}

	_procModules.insert (iter, fproc);
	
}

void FTspectralEngine::removeProcessorModule (unsigned int index, bool destroy)
{
	if (index >= _procModules.size()) return;

	vector<FTprocI*>::iterator iter = _procModules.begin();

	for (unsigned int n=0; n < index && iter!=_procModules.end(); ++n) {
		++iter;
	}
	if (iter == _procModules.end())
		return;

	if (destroy) {
		FTprocI * proc = (*iter);
		delete proc;
	}
	
	_procModules.erase(iter);
}

void FTspectralEngine::clearProcessorModules (bool destroy)
{
	vector<FTprocI*>::iterator iter = _procModules.begin();

	if (destroy) {
		for (; iter != _procModules.end(); ++iter) {
			delete (*iter);
		}
	}

	_procModules.clear();
}

void FTspectralEngine::setId (int id)
{
	_id = id;
	// set the id of all our filters

	for (vector<FTprocI*>::iterator iter = _procModules.begin();
	     iter != _procModules.end(); ++iter)
	{
		(*iter)->setId (id);
	}
}

void FTspectralEngine::setFFTsize (FTspectralEngine::FFT_Size sz)
{
	// THIS MUST NOT BE CALLED WHILE WE ARE ACTIVATED!
	
	if ((int) sz != _fftN) {
		_newfftN = sz;
		_fftnChanged = false;

		_averages = (int) (_oversamp * _updateSpeed * 512/(float)_newfftN); // magic?

		if (_averages == 0) _averages = 1;
		
		// change these now for the GUIs sake
		for (vector<FTprocI*>::iterator iter = _procModules.begin();
		     iter != _procModules.end(); ++iter)
		{
			(*iter)->setFFTsize (_newfftN);
		}

		reinitPlan(0);
	}

}

void FTspectralEngine::setOversamp (int osamp)
{
	_oversamp = osamp;

	_averages = (int) (_oversamp * (float) _updateSpeed * 512/(float)_fftN); // magic?
	if (_averages == 0) _averages = 1;

	// reset averages
	memset(_runningOutputPower, 0, _fftN * sizeof(float));
	memset(_runningInputPower, 0, _fftN * sizeof(float));
	_currInAvgIndex = 0;
	_currOutAvgIndex = 0;
	_avgReady = false;

	// set it in all the modules
	for (vector<FTprocI*>::iterator iter = _procModules.begin();
	     iter != _procModules.end(); ++iter)
	{
		(*iter)->setOversamp (_oversamp);
	}
	
}

void FTspectralEngine::setUpdateSpeed (UpdateSpeed speed)
{
	_updateSpeed = speed;
	_averages = (int) (_oversamp * (float) _updateSpeed * 512/(float)_fftN); // magic?
	if (_averages == 0) _averages = 1;

	// reset averages
	memset(_runningOutputPower, 0, _fftN * sizeof(float));
	memset(_runningInputPower, 0, _fftN * sizeof(float));
	_currInAvgIndex = 0;
	_currOutAvgIndex = 0;
	_avgReady = false;

}

void FTspectralEngine::setMaxDelay(float secs)
{
	// THIS MUST NOT BE CALLED WHILE WE ARE ACTIVATED!
	if (secs <= 0.0) return;

	_maxDelay = secs;
	
	for (vector<FTprocI*>::iterator iter = _procModules.begin();
	     iter != _procModules.end(); ++iter)
	{
		(*iter)->setMaxDelay (secs);
	}
	
}

void FTspectralEngine::reinitPlan(FTprocessPath *procpath)
{

	// destroy current plan
	_fftN = _newfftN;
	rfftw_destroy_plan (_fftPlan);
	rfftw_destroy_plan (_ifftPlan);
	_fftPlan = rfftw_create_plan(_fftN, FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE);		
	_ifftPlan = rfftw_create_plan(_fftN, FFTW_COMPLEX_TO_REAL, FFTW_ESTIMATE);		
	
	createWindowVectors(true);

	// reset averages
	memset(_runningOutputPower, 0, _fftN * sizeof(float));
	memset(_runningInputPower, 0, _fftN * sizeof(float));
	_currInAvgIndex = 0;
	_currOutAvgIndex = 0;
	_avgReady = false;
	
	//procpath->getInputFifo()->reset();
	//procpath->getOutputFifo()->reset();

	for (vector<FTprocI*>::iterator iter = _procModules.begin();
	     iter != _procModules.end(); ++iter)
	{
		(*iter)->reset();
	}
	
	memset((char *) _accum, 0, 2* _fftN * sizeof(fftw_real));
	memset((char *) _inwork, 0, _fftN * sizeof(fftw_real));

}

nframes_t FTspectralEngine::getLatency()
{
    	int step_size = _fftN / _oversamp;
        int latency = _fftN - step_size;

	return latency;
}

/**
 * Main FFT processing done here
 *  this is called from the i/o thread
 */
void FTspectralEngine::processNow (FTprocessPath *procpath)
{
	int i;
	int osamp = _oversamp;
	int step_size = _fftN / osamp;
        int latency = _fftN - step_size;
	float * win = _mWindows[_windowing];

	

	// do we have enough data for next frame (oversampled)?
	while (procpath->getInputFifo()->read_space() >= (step_size * sizeof(sample_t)))
	{
		//printf ("processing spectral sizeof sample = %d  fftw_real = %d\n", sizeof(sample_t), sizeof(fftw_real));
		
		// copy data into fft work buf
		procpath->getInputFifo()->read ( (char *) (&_inwork[latency]), step_size * sizeof(sample_t) );

		//printf ("real data 1 = %g\n", _inwork[1]);
		
		// window data into winwork
		for(i = 0; i < _fftN; i++)
		{
			_winwork[i] = _inwork[i] * win[i] * _inputGain; 
		}

		
		// do forward real FFT
		rfftw_one(_fftPlan, _winwork, _outwork);

		// compute running mag^2 buffer for input
		computeAverageInputPower (_outwork);


		// do processing in order with each processing module
		fftw_real * tempin = _outwork;
		fftw_real * tempout = _winwork;
		
		for (vector<FTprocI*>::iterator iter = _procModules.begin();
		     iter != _procModules.end(); ++iter)
		{
			// do it in place
			(*iter)->process (tempin,  _fftN);

		}
		// at the end the good data is in tempin
		
		// compute running mag^2 buffer for output
		computeAverageOutputPower (tempin);

		
		// do reverse FFT
		rfftw_one(_ifftPlan, tempin, tempout);

		// the output is scaled by fftN, we need to normalize it and window it
		for ( i=0; i < _fftN; i++)
		{
			_accum[i] += _mixRatio * 4.0f * win[i] * tempout[i] / ((float)_fftN * osamp);
		}

		// mix in dry only if necessary
		if (_mixRatio < 1.0) {
			float dry = 1.0 - _mixRatio;
			for (i=0; i < step_size; i++) {
				_accum[i] += dry * _inwork[i] ;
			}
		}
		
		// put step_size worth of the real data into the processPath out buffer
		procpath->getOutputFifo()->write( (char *)_accum, sizeof(sample_t) * step_size);		

		
		// shift output accumulator data
		memmove(_accum, _accum + step_size, _fftN*sizeof(sample_t));

		// shift input fifo (inwork)
		memmove(_inwork, _inwork + step_size, latency*sizeof(sample_t));
		
		
		

		// update events for those who listen
 		if (_avgReady && _updateToken) {
			_updateToken->setUpdated(true);
 			_avgReady = false;
 		}
	}
}




void FTspectralEngine::computeAverageInputPower (fftw_real *fftbuf)
{
	float power;
	int fftn2 = (_fftN+1) / 2;

	if (_averages > 1) {
	
		for (int i=0; i < fftn2 ; i++)
		{
			power = (fftbuf[i] * fftbuf[i]) + (fftbuf[_fftN-i] * fftbuf[_fftN-i]);
			if (_currInAvgIndex > 0) {
				_inputPowerSpectra[i] += power;
			}
			else {
				_inputPowerSpectra[i] = power;
				
			}	
		}
		
		_currInAvgIndex = (_currInAvgIndex+1) % _averages;
		
		if (_currInAvgIndex == 0) {
			for (int i=0; i < fftn2 ; i++)
			{
				_runningInputPower[i] = _inputPowerSpectra[i] / _averages;
			}
			_avgReady = true;
		}
	}
	else {
		// 1 average, minimize looping
		for (int i=0; i < fftn2 ; i++)
		{
			_runningInputPower[i] = (fftbuf[i] * fftbuf[i]) + (fftbuf[_fftN-i] * fftbuf[_fftN-i]);
		}		
		_avgReady = true;

	}
	
}

void FTspectralEngine::computeAverageOutputPower (fftw_real *fftbuf)
{
	float power;
	int fftn2 = (_fftN+1) / 2;

	if (_averages > 1) {
	
		for (int i=0; i < fftn2 ; i++)
		{
			power = (fftbuf[i] * fftbuf[i]) + (fftbuf[_fftN-i] * fftbuf[_fftN-i]);
			if (_currOutAvgIndex > 0) {
				_outputPowerSpectra[i] += power;
			}
			else {
				_outputPowerSpectra[i] = power;
				
			}	
		}
		
		_currOutAvgIndex = (_currOutAvgIndex+1) % _averages;
		
		if (_currOutAvgIndex == 0) {
			for (int i=0; i < fftn2 ; i++)
			{
				_runningOutputPower[i] = _outputPowerSpectra[i] / _averages;
			}
			_avgReady = true;
		}
	}
	else {
		// 1 average, minimize looping
		for (int i=0; i < fftn2 ; i++)
		{
			_runningOutputPower[i] = (fftbuf[i] * fftbuf[i]) + (fftbuf[_fftN-i] * fftbuf[_fftN-i]);
		}		
		_avgReady = true;
	}

	
}




void FTspectralEngine::createWindowVectors (bool noalloc)
{
    ///////////////////////////////////////////////////////////////////////////
    int i;
    ///////////////////////////////////////////////////////////////////////////

    if (!noalloc)
    {
	    ///////////////////////////////////////////////////////////////////////////
	    // create window array
	    _mWindows = new float*[NUM_WINDOWS];
	    
	    ///////////////////////////////////////////////////////////////////////////
	    // allocate vectors
	    for(i = 0; i < NUM_WINDOWS; i++)
	    {
		    _mWindows[i] = new float[FT_MAX_FFT_SIZE];
	    }
    }
    
    ///////////////////////////////////////////////////////////////////////////
    // create windows
    createRectangleWindow ();
    createHanningWindow ();
    createHammingWindow ();
    createBlackmanWindow ();
}

void FTspectralEngine::createRectangleWindow ()
{
    ///////////////////////////////////////////////////////////////////////////
    int i;
    ///////////////////////////////////////////////////////////////////////////
    
    for(i = 0; i < FT_MAX_FFT_SIZE; i++)
    {
	_mWindows[WINDOW_RECTANGLE][i] = 0.5;
    }
}


void FTspectralEngine::createHanningWindow ()
{
   ///////////////////////////////////////////////////////////////////////////
   int i;
   ///////////////////////////////////////////////////////////////////////////
   
   for(i = 0; i < _fftN; i++)
   {
	   _mWindows[WINDOW_HANNING][i] = 0.81 * ( // fudge factor
		   0.5 - 
		   (0.5 * 
		    //(float) cos(2.0 * M_PI * i / (_fftN - 1.0)));
		    (float) cos(2.0 * M_PI * i / (_fftN))));
    }    
}

void FTspectralEngine::createHammingWindow ()
{
   ///////////////////////////////////////////////////////////////////////////
   int i;
   ///////////////////////////////////////////////////////////////////////////
   
   for(i = 0; i < _fftN; i++)
    {
	    _mWindows[WINDOW_HAMMING][i] = 0.82 * ( // fudge factor
		    0.54 - 
		    (0.46 * 
		     (float) cos(2.0 * M_PI * i / (_fftN - 1.0))));
    }   
}


void FTspectralEngine::createBlackmanWindow ()
{
    ///////////////////////////////////////////////////////////////////////////
    int i;
    ///////////////////////////////////////////////////////////////////////////
    
    for(i = 0; i < _fftN; i++)
    {
	    _mWindows[WINDOW_BLACKMAN][i] = 0.9 * ( // fudge factor
		    0.42 - 
		    (0.50 * (float) cos(
			    2.0 * M_PI * i /(_fftN - 1.0))) + 
		    (0.08 * (float) cos(
			    4.0 * M_PI * i /(_fftN - 1.0))));
    }
}




