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

#ifndef __FTSPECTRALMANIP_HPP__
#define __FTSPECTRALMANIP_HPP__

#if HAVE_CONFIG_H
#include <config.h>
#endif


#ifdef HAVE_SRFFTW_H
#include <srfftw.h>
#else
#include <rfftw.h>
#endif

#include "FTutils.hpp"
#include "FTtypes.hpp"

class FTprocessPath;
class RingBuffer;
class FTspectrumModifier;
class FTupdateToken;

class FTspectralManip

{
  public: 
	FTspectralManip();
	virtual ~FTspectralManip();
	
	void processNow (FTprocessPath *procpath);

	enum FFT_Size
	{
		FFT_32 = 32,
		FFT_64 = 64,
		FFT_128 = 128,
		FFT_256 = 256,
		FFT_512 = 512,
		FFT_1024 = 1024,
		FFT_2048 = 2048,
		FFT_4096 = 4096,
		FFT_8192 = 8192

	};

	enum Windowing
	{
		WINDOW_HANNING = 0,
		WINDOW_HAMMING,
		WINDOW_BLACKMAN,
		WINDOW_RECTANGLE
	};

	enum UpdateSpeed
	{
		SPEED_FAST = 1,
		SPEED_MED = 2,
		SPEED_SLOW = 4,
		SPEED_TURTLE = 8,
	};
	
	static const int  NUM_WINDOWS  = 5;
	

	void setId (int id);
	int getId () { return _id; }

	void setUpdateToken (FTupdateToken *tok) { _updateToken = tok; }
	FTupdateToken * getUpdateToken() { return _updateToken; }
	
	void setFFTsize (FFT_Size sz); // ONLY call when not processing
	FFT_Size getFFTsize() { return (FFT_Size) _fftN; }

	void setSampleRate (nframes_t rate) { _sampleRate = rate; }
	nframes_t getSampleRate() { return _sampleRate; }
	
	void setWindowing (Windowing w) { _windowing = w; }
	Windowing getWindowing() { return _windowing; }

	void setAverages (int avg) { if (avg > _maxAverages) _averages = _maxAverages; _averages = avg; }
	int getAverages () { return _averages; }

	void setOversamp (int osamp);
	int getOversamp () { return _oversamp; }

	void setUpdateSpeed (UpdateSpeed speed);
	UpdateSpeed getUpdateSpeed () { return _updateSpeed; }

	void setInputGain (float gain) { _inputGain = gain; }
	float getInputGain () { return _inputGain; }

	// 1.0 is fully wet
	void setMixRatio (float ratio) { _mixRatio = ratio; }
	float getMixRatio () { return _mixRatio;}

	void setBypassed (bool flag) { _bypassFlag = flag; }
	bool getBypassed () { return _bypassFlag; }

	void setMuted (bool flag) { _mutedFlag = flag; }
	bool getMuted () { return _mutedFlag; }
	
	
	float getMaxDelay () { return _maxDelay; }
	void setMaxDelay (float secs); // ONLY call when not processing
	
	void setTempo (int tempo) { _tempo = tempo; }
        int getTempo() { return _tempo; }
	
	const float * getRunningInputPower() { return _runningInputPower; }
	const float * getRunningOutputPower() { return _runningOutputPower; }

	nframes_t getLatency();


	void setBypassFreqFilter (bool flag) { _bypassFreq = flag; }
	bool getBypassFreqFilter () { return _bypassFreq; }

	void setBypassDelayFilter (bool flag) { _bypassDelay = flag; }
	bool getBypassDelayFilter () { return _bypassDelay; }

	void setBypassFeedbackFilter (bool flag) { _bypassFeedb = flag; }
	bool getBypassFeedbackFilter () { return _bypassFeedb; }

	void setBypassScaleFilter (bool flag) { _bypassScale = flag; }
	bool getBypassScaleFilter () { return _bypassScale; }

        void setBypassGateFilter (bool flag) { _bypassGate = flag; }
	bool getBypassGateFilter () { return _bypassGate; }

	void setBypassInverseGateFilter (bool flag) { _bypassInverseGate = flag; }
	bool getBypassInverseGateFilter () { return _bypassInverseGate; }

	void setBypassMashLimitFilter (bool flag) { _bypassMashLimit = flag; }
	bool getBypassMashLimitFilter () { return _bypassMashLimit; }
	void setBypassMashPushFilter (bool flag) { _bypassMashPush = flag; }
	bool getBypassMashPushFilter () { return _bypassMashPush; }
	

	FTspectrumModifier * getFreqFilter() { return _freqFilter; }
	FTspectrumModifier * getDelayFilter() { return _delayFilter; }
	FTspectrumModifier * getFeedbackFilter() { return _feedbackFilter; }
	FTspectrumModifier * getScaleFilter() { return _scaleFilter; }
	FTspectrumModifier * getGateFilter() { return _gateFilter; }
	FTspectrumModifier * getInverseGateFilter() { return _inverseGateFilter; }
	FTspectrumModifier * getMashLimitFilter() { return _mashLimitFilter; }
	FTspectrumModifier * getMashPushFilter() { return _mashPushFilter; }
	
	static const char ** getWindowStrings() { return (const char **) _windowStrings; }
	static const int getWindowStringsCount() { return _windowStringCount; }
	static const int * getFFTSizes() { return (const int *) _fftSizes; }
	static const int getFFTSizeCount() { return _fftSizeCount; }
	
protected:

	void processFilter (FTprocessPath *procpath, fftw_real *data);
	void processDelay (FTprocessPath *procpath, fftw_real *data);

	void processPitchScale (FTprocessPath *procpath, fftw_real *data);
	void processGate (FTprocessPath *procpath, fftw_real *data);
	void processInverseGate (FTprocessPath *procpath, fftw_real *data);

	void processMashLimit (FTprocessPath *procpath, fftw_real *data);
	void processMashPush (FTprocessPath *procpath, fftw_real *data);

	
	void computeAverageInputPower (fftw_real *fftbuf);
	void computeAverageOutputPower (fftw_real *fftbuf);
	
	void createWindowVectors(bool noalloc=false);   
	void createRaisedCosineWindow();
	void createRectangleWindow();    
	void createHanningWindow();
	void createHammingWindow();
	void createBlackmanWindow();

	void reinitPlan(FTprocessPath *procpath);

	inline float powerLogScale(float yval, float min);
	

	static const int _windowStringCount;
	static const char * _windowStrings[];
	static const int _fftSizeCount;
	static const int  _fftSizes[];
	
	// fft size (thus frame length)
        int _fftN;
	Windowing _windowing;
	int _oversamp;
	unsigned long _maxDelaySamples;
	float _maxDelay;
	int _maxAverages;
	int _averages;
	
	rfftw_plan _fftPlan; // forward fft
	rfftw_plan _ifftPlan; // inverse fft


	int _newfftN;
	bool _fftnChanged;
	
	// this is a very large ringbuffer
	// used store the fft results over time
	// each frame is stored sequentially.
	// the length is determined by the maximum delay time
	RingBuffer *_frameFifo;

	// space for average input power buffer
	// elements = _fftN/2 * MAX_AVERAGES * MAX_OVERSAMP 
	fftw_real * _inputPowerSpectra;
	fftw_real * _outputPowerSpectra;

	// the current running avg power
	// elements = _fftN/2
	fftw_real * _runningInputPower;
	fftw_real * _runningOutputPower;

	// stuff for pitchscaling
	float *gLastPhase, *gSumPhase, *gAnaFreq, *gSynFreq, *gAnaMagn, *gSynMagn;
	
	nframes_t _sampleRate;
	
	// filters
	FTspectrumModifier * _freqFilter;
	FTspectrumModifier * _delayFilter;
	FTspectrumModifier * _feedbackFilter;
	FTspectrumModifier * _scaleFilter;
	FTspectrumModifier * _gateFilter;
	FTspectrumModifier * _inverseGateFilter;
	FTspectrumModifier * _mashLimitFilter;
	FTspectrumModifier * _mashPushFilter;

	
	bool _bypassFreq;
	bool _bypassDelay;
	bool _bypassFeedb;
	bool _bypassGate;
	bool _bypassInverseGate;
	bool _bypassScale;
	bool _bypassMashLimit;
	bool _bypassMashPush;
	
	float _inputGain;
	float _mixRatio;
	bool _bypassFlag;
	bool _mutedFlag;

	UpdateSpeed _updateSpeed;
	int _id;
	FTupdateToken * _updateToken;

	int _tempo;
private:
	
	fftw_real *_inwork, *_outwork;
	fftw_real *_winwork;
	fftw_real *_accum;
	fftw_real *_scaletemp;
	
	// for windowing
	float ** _mWindows;
	

	// for averaging

	int _currInAvgIndex;
	int _currOutAvgIndex;

	bool _avgReady;
	float _dbAdjust;
	
};



inline float FTspectralManip::powerLogScale(float yval, float min)
{
	
	if (yval <= min) {
		return -200.0;
	}

//   	if (yval > _maxval) {
//   		_maxval = yval;
//   	}
	
	//float nval = (10.0 * FTutils::fast_log10(yval / max));
	float nval = (10.0 * FTutils::fast_log10 (yval));
	// printf ("scaled value is %g   mincut=%g\n", nval, _minCutoff);
	return nval;
	
}


#endif
