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
#include "FTspectralManip.hpp"
#include "FTprocessPath.hpp"
#include "RingBuffer.hpp"
#include "FTspectrumModifier.hpp"
#include "FTioSupport.hpp"
#include "FTupdateToken.hpp"

const int FTspectralManip::_windowStringCount = 4;
const char * FTspectralManip::_windowStrings[] = {
	"Hanning", "Hamming", "Blackman", "Rectangle"
	};

const int FTspectralManip::_fftSizeCount = 9;
const int FTspectralManip::_fftSizes[] = {
	32, 64, 128, 256, 512, 1024, 2048, 4096, 8192
};


// in samples  (about 3 seconds at 44100)

#define FT_MAX_DELAYSAMPLES (1 << 19)


FTspectralManip::FTspectralManip()
	: _fftN (512), _windowing(FTspectralManip::WINDOW_HANNING)
	, _oversamp(4), _averages(8), _fftnChanged(false)
	, _bypassFreq(false), _bypassDelay(false), _bypassFeedb(false)
	  , _bypassGate(true), _bypassScale(false), _bypassMashLimit(true), _bypassMashPush(true)
	, _inputGain(1.0), _mixRatio(1.0), _bypassFlag(false), _mutedFlag(false), _updateSpeed(SPEED_MED)
	, _id(0), _updateToken(0)
	, _currInAvgIndex(0), _currOutAvgIndex(0), _avgReady(false)
	, _dbAdjust(-37.0)
{
	_inwork = new fftw_real [FT_MAX_FFT_SIZE];
	_outwork = new fftw_real [FT_MAX_FFT_SIZE];
	_winwork = new fftw_real [FT_MAX_FFT_SIZE];
	_scaletemp = new fftw_real [FT_MAX_FFT_SIZE];

	gLastPhase = new float [FT_MAX_FFT_SIZE];
	gSumPhase = new float [FT_MAX_FFT_SIZE];
	gAnaFreq = new float [FT_MAX_FFT_SIZE];
	gSynFreq = new float [FT_MAX_FFT_SIZE];
	gAnaMagn = new float [FT_MAX_FFT_SIZE];
	gSynMagn = new float [FT_MAX_FFT_SIZE];
	
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

	// in seconds
	_maxDelay = ((FT_MAX_DELAYSAMPLES) / ((float)_sampleRate*sizeof(sample_t))) ; 
	
	// this is a big boy containing the frequency data frames over time
	_frameFifo = new RingBuffer( (FT_MAX_DELAYSAMPLES) * sizeof(fftw_real) );
	
	// window init
	createWindowVectors();


	
	// create filters
	_freqFilter = new FTspectrumModifier(FTspectrumModifier::GAIN_MODIFIER, FREQ_SPECMOD, _fftN/2, 1.0);
	_freqFilter->setRange(0.0, 1.0);
	
	_delayFilter = new FTspectrumModifier(FTspectrumModifier::TIME_MODIFIER, DELAY_SPECMOD, _fftN/2, 0.0);
	_delayFilter->setRange(0.0, _maxDelay);
	
	_feedbackFilter = new FTspectrumModifier(FTspectrumModifier::UNIFORM_MODIFIER, FEEDB_SPECMOD, _fftN/2, 0.0);
	_feedbackFilter->setRange(0.0, 1.0);
	
// 	_scaleFilter = new FTspectrumModifier(FTspectrumModifier::RATIO_MODIFIER, SCALE_SPECMOD, _fftN/2, 1.0);
// 	_scaleFilter->setRange(0.5, 2.0);
 	_scaleFilter = new FTspectrumModifier(FTspectrumModifier::SEMITONE_MODIFIER, SCALE_SPECMOD, _fftN/2, 1.0);
 	_scaleFilter->setRange(0.5, 2.0);
	
	_gateFilter = new FTspectrumModifier(FTspectrumModifier::DB_MODIFIER, GATE_SPECMOD, _fftN/2, -90.0);
	_gateFilter->setRange(-90.0, 0.0);
	//_gateFilter->reset();
	
	_inverseGateFilter = new FTspectrumModifier(FTspectrumModifier::DB_MODIFIER, GATE_SPECMOD, _fftN/2, 0.0);
	_inverseGateFilter->setRange(-90.0, 0.0);

// 	_mashPushFilter = new FTspectrumModifier(FTspectrumModifier::DB_MODIFIER, MASH_SPECMOD, _fftN/2, -70.0);
// 	_mashPushFilter->setRange(-70.0, 0.0);
// 	_mashLimitFilter = new FTspectrumModifier(FTspectrumModifier::DB_MODIFIER, MASH_SPECMOD, _fftN/2, 0.0);
// 	_mashLimitFilter->setRange(-70.0, 0.0);
	
	_averages = (int) (_oversamp * _updateSpeed * 512/(float)_fftN); // magic?
	
}

FTspectralManip::~FTspectralManip()
{
	delete [] _inwork;
	delete [] _outwork;
	delete [] _winwork;
	delete [] _scaletemp;

	delete [] gLastPhase;
	delete [] gSumPhase;
	delete [] gAnaFreq;
	delete [] gSynFreq;
	delete [] gAnaMagn;
	delete [] gSynMagn;
	
	delete [] _accum;

	
	delete [] _inputPowerSpectra;
        delete [] _outputPowerSpectra;

	delete [] _runningInputPower;
	delete [] _runningOutputPower;
	
	
	_fftPlan = rfftw_create_plan(_fftN, FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE);		
	_ifftPlan = rfftw_create_plan(_fftN, FFTW_COMPLEX_TO_REAL, FFTW_ESTIMATE);		

	delete _frameFifo;

	// destroy window vectors
	for(int i = 0; i < NUM_WINDOWS; i++)
	{
		delete [] _mWindows[i];
	}
	

	delete _freqFilter;
	delete _delayFilter;
	delete _feedbackFilter;
	delete _scaleFilter;
	delete _gateFilter;
	delete _inverseGateFilter;

	rfftw_destroy_plan (_fftPlan);
	rfftw_destroy_plan (_ifftPlan);
	
	
}


void FTspectralManip::setId (int id)
{
	_id = id;
	// set the id of all our filters
	_freqFilter->setId (id);
	_delayFilter->setId (id);
	_feedbackFilter->setId (id);
	_scaleFilter->setId (id);
	_gateFilter->setId (id);
	_inverseGateFilter->setId (id);
}

void FTspectralManip::setFFTsize (FTspectralManip::FFT_Size sz)
{
	// THIS MUST NOT BE CALLED WHILE WE ARE ACTIVATED!
	
	if ((int) sz != _fftN) {
		_newfftN = sz;
		_fftnChanged = false;
		// the processing thread will check this
		// and do the real work

		_averages = (int) (_oversamp * _updateSpeed * 512/(float)_newfftN); // magic?

		if (_averages == 0) _averages = 1;
		
		// change these now for the GUIs sake
		_freqFilter->setLength(_newfftN/2);
		_delayFilter->setLength(_newfftN/2);
		_feedbackFilter->setLength(_newfftN/2);
		_scaleFilter->setLength(_newfftN/2);
		_gateFilter->setLength(_newfftN/2);
		_inverseGateFilter->setLength(_newfftN/2);

		reinitPlan(0);
	}

}

void FTspectralManip::setOversamp (int osamp)
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
}

void FTspectralManip::setUpdateSpeed (UpdateSpeed speed)
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


void FTspectralManip::reinitPlan(FTprocessPath *procpath)
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
	
	_freqFilter->setLength(_fftN/2);
	_delayFilter->setLength(_fftN/2);
	_feedbackFilter->setLength(_fftN/2);
	_scaleFilter->setLength(_fftN/2);
	_gateFilter->setLength(_fftN/2);
	_inverseGateFilter->setLength(_fftN/2);

	// flush FIFOs
	_frameFifo->reset();
	_frameFifo->mem_set(0);
	
	//procpath->getInputFifo()->reset();
	//procpath->getOutputFifo()->reset();

	memset((char *) _accum, 0, 2* _fftN * sizeof(fftw_real));
	memset((char *) _inwork, 0, _fftN * sizeof(fftw_real));

	
	//_mashPushFilter->setLength (_fftN/2);
	//_mashLimitFilter->setLength (_fftN/2);
}

nframes_t FTspectralManip::getLatency()
{
    	int step_size = _fftN / _oversamp;
        int latency = _fftN - step_size;

	return latency;
}

/**
 * Main FFT processing done here
 *  this is called from the i/o thread
 */
void FTspectralManip::processNow (FTprocessPath *procpath)
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


		
		// do spectral filter (uses _outwork in place)
		processFilter (procpath, _outwork);

		// do pitch scaling
		processPitchScale (procpath, _outwork);

		// do power mashing (sort of compressing)
		//processMashPush (procpath, _outwork);
		//processMashLimit (procpath, _outwork);
		
		// do power gating
		processGate (procpath, _outwork);
		processInverseGate (procpath, _outwork);
		
		
		// do spectral delay
		// uses _outwork as input and the frameFifo as output
		processDelay (procpath, _outwork);

		
		// pull out next frames worth of data for reverse fft
		_frameFifo->read( (char *) _winwork, sizeof(fftw_real) * _fftN);

		// compute running mag^2 buffer for output
		computeAverageOutputPower (_winwork);

		
		// do reverse FFT
		rfftw_one(_ifftPlan, _winwork, _outwork);

		// the output is scaled by fftN, we need to normalize it and window it
		for ( i=0; i < _fftN; i++)
		{
			_accum[i] += _mixRatio * 4.0f * win[i] * _outwork[i] / ((float)_fftN * osamp);
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


void FTspectralManip::processFilter (FTprocessPath *procpath, fftw_real *data)
{
	if (_bypassFreq) {
		return;
	}
	
	float *filter = _freqFilter->getValues();
	float min = _freqFilter->getMin();
	float max = _freqFilter->getMax();
	float filt;

	int fftN2 = (_fftN+1) >> 1;
	
	for (int i = 0; i < fftN2; i++)
	{
		if (filter[i] > max) filt = max;
		else if (filter[i] < min) filt = min;
		else filt = filter[i];
		
		data[i] *= filt;
		data[_fftN-i] *= filt;
	}
}

void FTspectralManip::processInverseGate (FTprocessPath *procpath, fftw_real *data)
{
	if (_bypassInverseGate) {
		return;
	}
	
	float *filter = _inverseGateFilter->getValues();

	float power;
	float db;
	int fftn2 = (_fftN+1) >> 1;
	
	// only allow data through if power is below threshold
	
 	for (int i = 0; i < fftn2; i++)
 	{
		power = (data[i] * data[i]) + (data[_fftN-i] * data[_fftN-i]);
		db = powerLogScale (power, 0.0000000) + _dbAdjust; // total fudge factors
		
  		//if (db < filter[i])
		if (db > filter[i])
		{
			//printf ("db %g\n", db);

			data[i] = data[_fftN-i] = 0.0;
 		}
 	}

}

void FTspectralManip::processGate (FTprocessPath *procpath, fftw_real *data)
{
	if (_bypassGate) {
		return;
	}
	
	float *filter = _gateFilter->getValues();

	float power;
	float db;
	int fftn2 = (_fftN+1) >> 1;
	
	// only allow data through if power is above threshold
	
 	for (int i = 0; i < fftn2; i++)
 	{
		power = (data[i] * data[i]) + (data[_fftN-i] * data[_fftN-i]);
		db = powerLogScale (power, 0.0000000) + _dbAdjust; // total fudge factors

		if (db < filter[i])
		{
			//printf ("db %g\n", db);

			data[i] = data[_fftN-i] = 0.0;
 		}
 	}

}

/**
 *  scales power levels lower than filter up to the filter level
 */
void FTspectralManip::processMashPush (FTprocessPath *procpath, fftw_real *data)
{
	if (_bypassMashPush) {
		return;
	}

	float *filter = _mashPushFilter->getValues();
	//float min = _mashFilter->getMin();
	//float max = _mashFilter->getMax();
	float power, db, filt;
	int fftn2 = (_fftN+1) >> 1;

	
 	for (int i = 0; i < fftn2; i++)
 	{
		power = (data[i] * data[i]) + (data[_fftN-i] * data[_fftN-i]);
		db = powerLogScale (power, 0.0000000) ; // total fudge factors

		//filt = (  filter[i] - 10*FTutils::fast_log10(15000));
		filt = filter[i];
		
  		if (db < filt)
		{
			//printf ("db %g\n", db);
			//filt =  pow ( 10.0, ((db - filt) )/10.0);
			filt = 0.5;
			//printf ("db %g  filter[i]=%g  filt %g\n", db, filter[i], filt);

			data[i] *= filt;
			data[_fftN-i] *= filt;
 		}

 	}
	
}

/**
 *  limit power levels greater than filter down to the filter level
 */
void FTspectralManip::processMashLimit (FTprocessPath *procpath, fftw_real *data)
{
	if (_bypassMashLimit) {
		return;
	}
	
	float *filter = _mashLimitFilter->getValues();
	//float min = _mashFilter->getMin();
	//float max = _mashFilter->getMax();
	float power, db, filt;
	int fftn2 = (_fftN+1) >> 1;

	
 	for (int i = 0; i < fftn2; i++)
 	{
		power = (data[i] * data[i]) + (data[_fftN-i] * data[_fftN-i]);
		db = powerLogScale (power, 0.0000000) - 50; // total fudge factors
		//filt = (  filter[i] - 10*FTutils::fast_log10(15000));
		//db = powerLogScale (power, 0.00, 1) - 100; // total fudge factors
		filt = filter[i];
		
  		if (db > filt)
		{
			//printf ("db %g\n", db);
			filt = pow ( 10.0, (filt - db)/10.0);

			//printf ("db %g  filter[i]=%g  filt %g\n", db, filter[i], filt);

			data[i] *= filt;
			data[_fftN-i] *= filt;
 		}

 	}
	
}


void FTspectralManip::processPitchScale (FTprocessPath *procpath, fftw_real *data)
{
 	if (_bypassScale) {
 		return;
 	}

	float *filter = _scaleFilter->getValues();

	double magn, phase, tmp, real, imag;
	double freqPerBin, expct;
	long k, qpd, index, stepSize;
	int fftFrameSize2 = _fftN / 2;
	int fftFrameLength = _fftN;

	float min = _scaleFilter->getMin();
	float max = _scaleFilter->getMax();
	float filt;

	int osamp = _oversamp;
	
	stepSize = fftFrameLength/osamp;
	freqPerBin = _sampleRate*2.0/(double)fftFrameLength;
	expct = 2.0*M_PI*(double)stepSize/(double)fftFrameLength;

	/* this is the analysis step */
	for (k = 0; k <= fftFrameSize2; k++) {
		
		real = data[k];
		imag = data[fftFrameLength - k];
		
		/* compute magnitude and phase */
		magn = sqrt(real*real + imag*imag);
		phase = atan2(imag,real);
		
		/* compute phase difference */
		tmp = phase - gLastPhase[k];
		gLastPhase[k] = phase;
		
		/* subtract expected phase difference */
		tmp -= (double)k*expct;
		
		/* map delta phase into +/- Pi interval */
		qpd = (long) (tmp/M_PI);
		if (qpd >= 0) qpd += qpd&1;
		else qpd -= qpd&1;
		tmp -= M_PI*(double)qpd;
		
		/* get deviation from bin frequency from the +/- Pi interval */
		tmp = osamp*tmp/(2.0f*M_PI);
		
		/* compute the k-th partials' true frequency */
		tmp = (double)k*freqPerBin + tmp*freqPerBin;
		
		/* store magnitude and true frequency in analysis arrays */
		gAnaMagn[k] = magn;
		gAnaFreq[k] = tmp;
		
	}
	
	/* ***************** PROCESSING ******************* */
	/* this does the actual pitch scaling */
	memset(gSynMagn, 0, fftFrameLength*sizeof(float));
	memset(gSynFreq, 0, fftFrameLength*sizeof(float));
	for (k = 0; k <= fftFrameSize2; k++)
	{
		if (filter[k] > max) filt = max;
		else if (filter[k] < min) filt = min;
		else filt = filter[k];

		index = (long) (k/filt);
		if (index <= fftFrameSize2) {
			/* new bin overrides existing if magnitude is higher */ 

			if (gAnaMagn[index] > gSynMagn[k]) {
				gSynMagn[k] = gAnaMagn[index];
				gSynFreq[k] = gAnaFreq[index] * filt;
			}
			
			/* fill empty bins with nearest neighbour */
			
			if ((gSynFreq[k] == 0.) && (k > 0)) {
				gSynFreq[k] = gSynFreq[k-1];
				gSynMagn[k] = gSynMagn[k-1];
			}
		}
	}
	
	
	/* ***************** SYNTHESIS ******************* */
	/* this is the synthesis step */
	for (k = 0; k <= fftFrameSize2; k++) {
		
		/* get magnitude and true frequency from synthesis arrays */
		magn = gSynMagn[k];
		tmp = gSynFreq[k];
		
		/* subtract bin mid frequency */
		tmp -= (double)k*freqPerBin;

		/* get bin deviation from freq deviation */
		tmp /= freqPerBin;
		
		/* take osamp into account */
		tmp = 2.*M_PI*tmp/osamp;
		
		/* add the overlap phase advance back in */
		tmp += (double)k*expct;
		
		/* accumulate delta phase to get bin phase */
		gSumPhase[k] += tmp;
		phase = gSumPhase[k];
		
		data[k] = magn*cos(phase);
		data[fftFrameLength - k] = magn*sin(phase);
	} 
	
}



void FTspectralManip::processDelay (FTprocessPath *procpath, fftw_real *data)
{
	if (_bypassDelay)
	{
		_frameFifo->write ( (char *) data, sizeof(fftw_real) * _fftN);
		// RETURNS HERE
		return;
	}

	float *delay = _delayFilter->getValues();
	float *feedb = _feedbackFilter->getValues();
	float feedback = 0.0;

	float mindelay = _delayFilter->getMin();
	float maxdelay = _delayFilter->getMax();
	float thisdelay;
	
	float *rdest = 0, *idest = 0;
	float *rcurr = 0, *icurr = 0;
	nframes_t bshift, fshift;
	int fftn2 = (_fftN+1) >> 1;

	
	//RingBuffer::rw_vector readvec[2];
	RingBuffer::rw_vector wrvec[2];

	_frameFifo->get_write_vector(wrvec);

	
	for (int i = 0; i < fftn2; i++)
	{
		if (_bypassFeedb) {
			feedback = 0.0;
		}
		else {
			feedback = feedb[i] < 0.0 ? 0.0 : feedb[i];
		}
		
		if (delay[i] > maxdelay) {
			thisdelay = maxdelay;
		}
		else if (delay[i] <= mindelay) {
			// force feedback to 0 if no delay
			feedback = 0.0;
			thisdelay = mindelay;
		}
		else {
			thisdelay = delay[i];
		}
		
		// frames to shift
		fshift = ((nframes_t)(_sampleRate * thisdelay * sizeof(sample_t))) / _fftN;
		//nframes_t bshift = fshift * _fftN * sizeof(sample_t);
		//printf ("bshift %Zd  wrvec[0]=%d\n", bshift, wrvec[0].len);
		// byte offset to start of frame
		//bshift = fshift * _fftN * sizeof(sample_t);
		bshift = fshift * _fftN * sizeof(sample_t);


		// we know the next frame is in the first segment of the FIFO
		// because our FIFO size is always shifted one frame at a time
		rcurr = (float * ) (wrvec[0].buf + i*sizeof(sample_t));
		icurr = (float * ) (wrvec[0].buf + (_fftN-i)*sizeof(sample_t));
		
		if (wrvec[0].len > bshift) {
			rdest = (float *) (wrvec[0].buf + bshift + i*sizeof(sample_t));
			idest = (float *) (wrvec[0].buf + bshift + (_fftN-i)*sizeof(sample_t));
		}
		else if (wrvec[1].len) {
			bshift -= wrvec[0].len;
			rdest = (float *) (wrvec[1].buf + bshift + i*sizeof(sample_t));
			idest = (float *) (wrvec[1].buf + bshift + (_fftN-i)*sizeof(sample_t));
		}
		else {
			printf ("BLAHHALALAHLA\n");
			continue;
		}
		
		
		*rdest = data[i] + (*rcurr)*feedback;
		*idest = data[_fftN-i] + (*icurr)*feedback;
	}
	
	// advance it
	_frameFifo->write_advance(_fftN * sizeof(fftw_real));
	
	//_frameFifo->write ( (char *) data, sizeof(fftw_real) * _fftN);
}






void FTspectralManip::computeAverageInputPower (fftw_real *fftbuf)
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

void FTspectralManip::computeAverageOutputPower (fftw_real *fftbuf)
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




void FTspectralManip::createWindowVectors (bool noalloc)
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

void FTspectralManip::createRectangleWindow ()
{
    ///////////////////////////////////////////////////////////////////////////
    int i;
    ///////////////////////////////////////////////////////////////////////////
    
    for(i = 0; i < FT_MAX_FFT_SIZE; i++)
    {
	_mWindows[WINDOW_RECTANGLE][i] = 0.5;
    }
}


void FTspectralManip::createHanningWindow ()
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

void FTspectralManip::createHammingWindow ()
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


void FTspectralManip::createBlackmanWindow ()
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





