
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


// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <stdio.h>
#include <math.h>
#include "FTactiveBarGraph.hpp"
#include "FTspectrumModifier.hpp"
#include "FTutils.hpp"
#include "FTmainwin.hpp"
#include "FTjackSupport.hpp"


enum
{
	FT_1Xscale=1000,
	FT_2Xscale,
	FT_LogaXscale,
	FT_LogbXscale,	
};



// the event tables connect the wxWindows events with the functions (event
// handlers) which process them. It can be also done at run-time, but for the
// simple menu events like this the static method is much simpler.
BEGIN_EVENT_TABLE(FTactiveBarGraph, wxPanel)
	EVT_PAINT(FTactiveBarGraph::OnPaint)
	EVT_SIZE(FTactiveBarGraph::OnSize)
	EVT_LEFT_DOWN(FTactiveBarGraph::OnMouseActivity)
	EVT_LEFT_UP(FTactiveBarGraph::OnMouseActivity)
	EVT_RIGHT_DOWN(FTactiveBarGraph::OnMouseActivity)
	EVT_RIGHT_UP(FTactiveBarGraph::OnMouseActivity)
	EVT_MIDDLE_DOWN(FTactiveBarGraph::OnMouseActivity)
	EVT_MIDDLE_UP(FTactiveBarGraph::OnMouseActivity)
	EVT_MOTION(FTactiveBarGraph::OnMouseActivity)
	EVT_ENTER_WINDOW(FTactiveBarGraph::OnMouseActivity)
	EVT_LEAVE_WINDOW(FTactiveBarGraph::OnMouseActivity)
	EVT_MOTION(FTactiveBarGraph::OnMouseActivity)

	EVT_MENU (FT_1Xscale,FTactiveBarGraph::OnXscaleMenu)
	EVT_MENU (FT_2Xscale,FTactiveBarGraph::OnXscaleMenu)
	EVT_MENU (FT_LogaXscale,FTactiveBarGraph::OnXscaleMenu)
	EVT_MENU (FT_LogbXscale,FTactiveBarGraph::OnXscaleMenu)
	
END_EVENT_TABLE()


FTactiveBarGraph::FTactiveBarGraph(FTmainwin *win, wxWindow *parent, wxWindowID id,
			     const wxPoint& pos,
			     const wxSize& size,
			     long style ,
			     const wxString& name)

	: wxPanel(parent, id, pos, size, style, name)
	//, _topHeight(4), _bottomHeight(4), _leftWidth(4, _rightWidth(4)
	, _specMod(0), _topSpecMod(0)
	,_mindb(-50.0), _maxdb(0.0), _absmindb(-50), _absmaxdb(0.0)
	, _tmpfilt(0), _toptmpfilt(0)
	, _barColor0("skyblue"), _barColor1("steelblue")
	,  _barColor2("seagreen"), _barColor3("darkseagreen")
	,_barColorDead("gray30")
	,_tipColor(200,200,0)
	, _penColor("blue"), _backingMap(0)
	, _xScaleType(XSCALE_1X), _lastX(0)
	, _dragging(false), _zooming(false)
	, _mainwin(win)
{
	SetBackgroundColour(*wxBLACK);

	_barBrush0.SetColour(_barColor0);
	_barBrush0.SetStyle(wxSOLID);
	_barBrush1.SetColour(_barColor1);
	_barBrush1.SetStyle(wxSOLID);

	_barBrush2.SetColour(_barColor2);
	_barBrush2.SetStyle(wxSOLID);
	_barBrush3.SetColour(_barColor3);
	_barBrush3.SetStyle(wxSOLID);
	_barBrushDead.SetColour(_barColorDead);
	_barBrushDead.SetStyle(wxSOLID);
	
	_tipBrush.SetColour(_tipColor);
	_tipBrush.SetStyle(wxSOLID);
	
	_bgBrush.SetColour(wxColour(30,50,30));
	//_bgBrush.SetStyle(wxCROSS_HATCH);
	
	_barPen.SetColour(_penColor);
	_barPen.SetStyle(wxTRANSPARENT);

	_xscaleMenu = new wxMenu("");
	_xscaleMenu->Append ( new wxMenuItem(_xscaleMenu, FT_1Xscale, "1x Scale"));
	_xscaleMenu->Append ( new wxMenuItem(_xscaleMenu, FT_2Xscale, "2x Scale"));
	_xscaleMenu->Append ( new wxMenuItem(_xscaleMenu, FT_LogaXscale, "logA Scale"));
	_xscaleMenu->Append ( new wxMenuItem(_xscaleMenu, FT_LogbXscale, "logB Scale"));

}

FTactiveBarGraph::~FTactiveBarGraph()
{
	if (_tmpfilt) delete [] _tmpfilt;
	if (_toptmpfilt) delete [] _toptmpfilt;

	if (_backingMap) delete _backingMap;
	delete _xscaleMenu;
}

void FTactiveBarGraph::setSpectrumModifier (FTspectrumModifier *sm)
{
	_specMod = sm;
	_xscale = _width/(float)_specMod->getLength();

	_min = _absmin = _specMod->getMin();
	_max = _absmax = _specMod->getMax();

 	if (_specMod->getModifierType() == FTspectrumModifier::GAIN_MODIFIER)
	{
		_mindb = _absmindb;
		_maxdb = _absmaxdb;
	}

	
	if (_tmpfilt) delete [] _tmpfilt;
	_tmpfilt = new float[_specMod->getLength()];
	
}

void FTactiveBarGraph::setTopSpectrumModifier (FTspectrumModifier *sm)
{
	_topSpecMod = sm;

	// should be same as other one
	_xscale = _width/(float)_topSpecMod->getLength();
	_min = _absmin = _topSpecMod->getMin();
	_max =  _absmax = _topSpecMod->getMax();

 	if (_topSpecMod->getModifierType() == FTspectrumModifier::GAIN_MODIFIER)
	{
		_mindb = _absmindb;
		_maxdb = _absmaxdb;
	}

	
	if (_toptmpfilt) delete [] _toptmpfilt;
	_toptmpfilt = new float[_topSpecMod->getLength()];
	
}


void FTactiveBarGraph::setXscale(XScaleType sc)
{
	_xScaleType = sc; 
	Refresh(FALSE);
}


bool FTactiveBarGraph::setMinMax(float min, float max)
{
 	if (_specMod->getModifierType() == FTspectrumModifier::GAIN_MODIFIER)
	{
		if (min >= _absmin && max <= _absmax) {
			_min = min;
			_max = max;
			_mindb = valToDb(min);
			_maxdb = valToDb(max);
			
			Refresh(FALSE);
			return true;
		}
		Refresh(FALSE);
		return false;
	}
	else if (min >= _absmin && max <= _absmax) {
		_min = min;
		_max = max;
		Refresh(FALSE);
		return true;
	}

	Refresh(FALSE);
	return false;
}


void FTactiveBarGraph::xToFreqRange(int x, float &fromfreq, float &tofreq, int &frombin, int &tobin)
{
	float freqPerBin =  FTjackSupport::instance()->getSampleRate()/(2.0 * (double)_specMod->getLength());

	//printf ("specmod length = %d  freqperbin=%g\n", _specMod->getLength(), freqPerBin);
	xToBinRange(x, frombin, tobin);

	fromfreq = freqPerBin * frombin;
	tofreq = freqPerBin * tobin + freqPerBin;
}

void FTactiveBarGraph::xToBinRange(int x, int &frombin, int &tobin)
{
	if (!_specMod) {
		frombin = tobin = 0;
		return;
	}
	
	// converts x coord into filter bin
	// according to scaling

	int bin, lbin, rbin;
	int totbins = _specMod->getLength();
	//float xscale = _width / (float)totbins;


	
	if (x < 0) x = 0;
	else if (x >= _width) x = _width-1;
	
	if (_xScaleType == XSCALE_1X) {
		bin = rbin = lbin = (int)(x / _xscale);
		//printf (" %d  %g  %d\n", x, bin*xscale, (int)(bin * xscale));
		
		// find lowest with same x
		while ( ((int)((lbin-1)*_xscale) == x) && (lbin > 0)) {
			lbin--;
		}
		// find highest with same x
		while ( ((int)((rbin+1)*_xscale) == x) && (rbin < totbins-1)) {
			rbin++;
		}

		frombin = lbin;
		tobin = rbin;
	}
	else if (_xScaleType == XSCALE_2X) {
		float hxscale = _xscale * 2;

		if (x >= _width-2) {
			lbin = totbins/2;
			rbin = totbins-1;
		}
		else {
		
			bin = rbin = lbin = (int)(x / hxscale);
			//printf (" %d  %g  %d\n", x, bin*xscale, (int)(bin * xscale));
			
			// find lowest with same x
			while ( ((int)((lbin-1)*hxscale) == x) && (lbin > 0)) {
				lbin--;
			}
			// find highest with same x
			while ( ((int)((rbin+1)*hxscale) == x) && (rbin < totbins>>1)) {
				rbin++;
			}
		}
		
		frombin = lbin;
		tobin = rbin;
	}
	else if (_xScaleType == XSCALE_LOGA)
	{
		if (x >= _width-1) {
			lbin = totbins/2;
			rbin = totbins-1;
		}
		else {
			float xscale = x / (float)_width;
			
			// use log scale for freq
			lbin = rbin = (int) pow ( totbins>>1, xscale) - 1;
			
			// find lowest with same x
			while ( (lbin > 0) && ((int)(((FTutils::fast_log10(lbin) / FTutils::fast_log10(totbins/2)) * _width)) == x)) {
				lbin--;
			}
			// find highest with same x
			while ( (rbin < totbins>>1) && ((int)(((FTutils::fast_log10(rbin) / FTutils::fast_log10(totbins/2)) )) == x)) {
				rbin++;
			}
			
		}

		frombin = lbin;
		tobin = rbin;

		// printf ("bin %d  fromx=%d  tox=%d\n", bin, fromx, tox);
	}
	else if (_xScaleType == XSCALE_LOGB)
	{
		if (x >= _width-1) {
			lbin = (int) (totbins/3.0);
			rbin = totbins-1;
		}
		else {
			float xscale = x / (float)_width;
			
			// use log scale for freq
			lbin = rbin = (int) pow ( totbins/3.0, xscale) - 1;
			
			// find lowest with same x
			while ( (lbin > 0) && ((int)(((FTutils::fast_log10(lbin) / FTutils::fast_log10(totbins/3.0)) * _width)) == x)) {
				lbin--;
			}
			// find highest with same x
			while ( (rbin < (int)(totbins/3.0)) && ((int)(((FTutils::fast_log10(rbin) / FTutils::fast_log10(totbins/3.0)) )) == x)) {
				rbin++;
			}
			
		}

		frombin = lbin;
		tobin = rbin;

		// printf ("bin %d  fromx=%d  tox=%d\n", bin, fromx, tox);
	}
	else {
		frombin = 0;
		tobin = 0;
	}

	//printf ("x=%d  frombin=%d tobin=%d\n", x, frombin, tobin);
}

int FTactiveBarGraph::xDeltaToBinDelta(int xdelt)
{

	return (int) (xdelt / _xscale);

}

float FTactiveBarGraph::valDiffY(float val, int lasty, int newy)
{
	if (_specMod->getModifierType() == FTspectrumModifier::GAIN_MODIFIER) {
		// db scaled
		float vdb = valToDb (val);
		float ddb = ((lasty-newy)/(float)_height) * (_maxdb - _mindb) ;

		return dbToVal (vdb + ddb);
	}
	else {
		float shiftval = yToVal(newy) - yToVal(lasty);
		return ( val + shiftval);
	}
}

void FTactiveBarGraph::binToXRange(int bin, int &fromx, int &tox)
{
	if (!_specMod) {
		fromx = tox = 0;
		return;
	}

	// converts bin into x coord range
	// according to scaling

	int x, lx, rx;
	int totbins = _specMod->getLength();
	//float xscale = _width / (float)totbins;

	
	if (bin < 0) bin = 0;
	else if (bin >= totbins) bin = totbins-1;
	
	if (_xScaleType == XSCALE_1X) {
		//bin = rbin = lbin = (int)(x / xscale);
		x = lx = rx = (int) (bin * _xscale);
		//printf (" %d  %g  %d\n", x, bin*xscale, (int)(bin * xscale));
		
		// find lowest x with same bin
		while ( ((int)((lx-1)/_xscale) == bin) && (lx > 0)) {
			lx--;
		}
		// find highest with same x
		while ( ((int)((rx+1)/_xscale) == bin) && (rx < _width-1)) {
			rx++;
		}

		fromx = lx;
		tox = rx;
	}
	else if (_xScaleType == XSCALE_2X) {
		float hxscale = _xscale * 2;

		if (bin >= totbins>>1) {
			rx = (int) (totbins * _xscale);
			lx = rx - 2;
		}
		else {
			//bin = rbin = lbin = (int)(x / xscale);
			x = lx = rx = (int) (bin * hxscale );
			//printf (" %d  %g  %d\n", x, bin*xscale, (int)(bin * xscale));

			// find lowest x with same bin
			while ( ((int)((lx-1)/(hxscale)) == bin) && (lx > 0)) {
				lx--;
			}
			// find highest with same x
			while ( ((int)((rx+1)/hxscale) == bin) && (rx < _width-1)) {
				rx++;
			}
		}

		

		fromx = lx;
		tox = rx;
	}
	else if (_xScaleType == XSCALE_LOGA)
	{
		// use log scale for freq
		if (bin > totbins/2) {
			lx = rx = (int) (totbins * _xscale);
			
		} else if (bin == 0) {
			lx = 0;
			rx = (int) ((FTutils::fast_log10(bin+2) / FTutils::fast_log10(totbins*0.5)) * _width);
		} else {
			lx = (int) ((FTutils::fast_log10(bin+1) / FTutils::fast_log10(totbins*0.5)) * _width);
			rx = (int) ((FTutils::fast_log10(bin+2) / FTutils::fast_log10(totbins*0.5)) * _width);
		}


		fromx = lx;
		tox = rx;

		// printf ("bin %d  fromx=%d  tox=%d\n", bin, fromx, tox);
	}
	else if (_xScaleType == XSCALE_LOGB)
	{
		// use log scale for freq
		if (bin > (int)(totbins*0.3333)) {
			lx = rx = (int) (totbins * _xscale);
			
		} else if (bin == 0) {
			lx = 0;
			rx = (int) ((FTutils::fast_log10(bin+2) / FTutils::fast_log10(totbins*0.3333)) * _width);
		} else {
			lx = (int) ((FTutils::fast_log10(bin+1) / FTutils::fast_log10(totbins*0.3333)) * _width);
			rx = (int) ((FTutils::fast_log10(bin+2) / FTutils::fast_log10(totbins*0.3333)) * _width);
		}


		fromx = lx;
		tox = rx;

		// printf ("bin %d  fromx=%d  tox=%d\n", bin, fromx, tox);
	}
	else {
		fromx = 0;
		tox = 0;
	}

	//printf ("bin=%d  fromx=%d tox=%d\n", bin, fromx, tox);
}


float FTactiveBarGraph::valToDb(float val)
{
	// assumes 0 <= yval <= 1
	
	if (val <= 0.0) {
		// negative infinity really
		return -200.0;
	}
	
	//float nval = (20.0 * FTutils::fast_log10(yval / refmax));
	float nval = (20.0 * FTutils::fast_log10(val));
	
	// printf ("scaled value is %g   mincut=%g\n", nval, _minCutoff);
	return nval;
	
}

float FTactiveBarGraph::dbToVal(float db)
{
	
	//float nval = (20.0 * FTutils::fast_log10(yval / refmax));
	float nval = pow ( 10.0, db/20);
	
	// printf ("scaled value is %g   mincut=%g\n", nval, _minCutoff);
	return nval;
	
}



int FTactiveBarGraph::valToY(float val)
{
	int y = 0;
	
	if (_specMod->getModifierType() == FTspectrumModifier::GAIN_MODIFIER) {
		// db scale it
		float db = valToDb(val);
		y = (int) (( (db - _mindb) / (_maxdb - _mindb)) * _height);
		//printf ("val=%g  db=%g  y=%d\n", val, db, y);

	}
	else {
		y = (int) ( (val - _min)/(_max-_min) * _height);
	}	

	//printf ("val to y: %g to %d\n", val, y);
	return y;
}


float FTactiveBarGraph::yToDb(int y)
{
	return (((_height - y)/(float)_height) * (_maxdb-_mindb)) + _mindb ;
}

float FTactiveBarGraph::yToVal(int y)
{
	float val = 0.0;
	
	if (_specMod->getModifierType() == FTspectrumModifier::GAIN_MODIFIER) {
		// go from db to linear
		
		float db = yToDb (y);

		val = dbToVal(db);
		//printf ("y=%d  val=%g  db=%g\n",  y, val, db);

	}
	else {
		val = (((_height - y) / (float)_height)) * (_max-_min) + _min;
	}	

	return val;
}


void FTactiveBarGraph::OnPaint(wxPaintEvent & event)
{
	//printf ("ActiveBarGraph::OnPaint  xscale=%g\n", _xscale);
	if (!_backingMap || !_specMod) {
		event.Skip();
		return;
	}
	       
	//Clear();
	wxPaintDC dc(this);
	
	wxMemoryDC backdc;
	if (_backingMap)
		backdc.SelectObject(*_backingMap);

	
	int bincnt = _specMod->getLength();
	//float barwidthF = _width / (float) barcnt;
	//int currx = 0;

	float *bvalues = 0, *tvalues=0;

	if (_specMod) {
		bvalues = _specMod->getValues();
	}

	if (_topSpecMod) {
		tvalues = _topSpecMod->getValues();
	}

	bool both = false;
	if (bvalues && tvalues) {
		both = true;
	}
	
	backdc.BeginDrawing();

	backdc.SetBackground(_bgBrush);
	backdc.Clear();
	
	// draw bars
	//backdc.SetBrush(_barBrush);
	backdc.SetPen(_barPen);

	/*
	if (_width == barcnt)
	{
		for (int i=0; i < barcnt; i++ )
		{
			int val = (int) ( (values[i] - _min)/(_max-_min) * _height);
			backdc.DrawRectangle( i, _height - val, 1,  val);
		}
	}
	*/

	for (int i=0; i < bincnt; i++ )
	{
		int yu=0 , yl = _height;
		if (bvalues) {
			yu =  _height - valToY (bvalues[i]);
		}
		if (tvalues) {
			yl =  _height - valToY (tvalues[i]);
		}
		
		int leftx, rightx;
		binToXRange(i, leftx, rightx);

		if (both)
		{
			if (i%2==0) {
				backdc.SetBrush(_barBrush2);
			}
			else  {
				backdc.SetBrush(_barBrush3);
			}
		}
		else {
			if (i%2==0) {
				backdc.SetBrush(_barBrush1);
			}
			else  {
				backdc.SetBrush(_barBrush0);
			}
		}
		// printf ("%08x:  %d  %d\n", (unsigned) this, leftx, y);

		// main bar
		if (yu < yl) {
			backdc.DrawRectangle( leftx,  yu , rightx - leftx + 1,  yl - yu);

			// top line
			backdc.SetBrush(_tipBrush);
			backdc.DrawRectangle( leftx, yu, rightx - leftx + 1,  2);
			backdc.DrawRectangle( leftx, yl, rightx - leftx + 1,  2);
		}
		else {
			backdc.SetBrush(_barBrushDead);
			backdc.DrawRectangle( leftx,  yu , rightx - leftx + 1,  yl - yu);
		}
	}


	if (_zooming) {
		// draw xor'd box
		backdc.SetLogicalFunction (wxINVERT);

		backdc.DrawRectangle ( 0, _topzoomY, _width, _bottomzoomY - _topzoomY);

	}
	

	
	backdc.EndDrawing();

	// blit to screen
	dc.Blit(0,0, _width, _height, &backdc, 0,0);
	
	// event.Skip();
}


void FTactiveBarGraph::recalculate()
{
	int totbins = _specMod->getLength();
	_xscale = _width / (float)totbins;

	_min = _specMod->getMin();
	_max = _specMod->getMax();
	
	Refresh(FALSE);
}


void FTactiveBarGraph::OnSize(wxSizeEvent & event)
{

	GetClientSize(&_width, &_height);
	//printf ("ActiveBarGraph::OnSize:  width=%d\n", _width);

	if (_backingMap) delete _backingMap;

	_backingMap = new wxBitmap(_width, _height);
	
	SetBackgroundColour(*wxBLACK);
	Clear();

	recalculate();
	
	event.Skip();
}

void FTactiveBarGraph::OnMouseActivity( wxMouseEvent &event)
{
	if (!_specMod) {
		event.Skip();
		return;
	}
	
	int pX = event.GetX();
	int pY = event.GetY();
	int i,j;
	
	//int length = _specMod->getLength();
	//float xscale = (float) _width / (float)length;

	FTspectrumModifier *specm;
	
	if (_specMod) {
		specm = _specMod;
	}
	else if (_topSpecMod) {
		specm = _topSpecMod;
	}
	else {
		// nothing to do
		return;
	}
	
	
	if (event.LeftDown()) {
		if (event.RightIsDown()) {
			SetCursor(wxCURSOR_HAND);
		}
		CaptureMouse();

	}
	else if (event.RightDown()) {
		SetCursor(wxCURSOR_HAND);
		CaptureMouse();
	}
			


	if (event.Entering()) {
		SetCursor(wxCURSOR_PENCIL);
		updatePositionLabels(pX, pY, true);
	}
	else if (event.Leaving()) {
		SetCursor(*wxSTANDARD_CURSOR);
		_mainwin->updatePosition("", "");
	}
	else if (event.MiddleUp()) {
		// popup scale menu
		this->PopupMenu ( _xscaleMenu, pX, pY);

	}
	else if (!_dragging && event.LeftIsDown() && (_zooming || (event.ControlDown() && event.ShiftDown() && event.AltDown())))
	{
		// zooming related
		if (event.LeftDown()) {
			_zooming = true;
			_firstY = _topzoomY = _bottomzoomY = pY;
			Refresh(FALSE);
		}
		else if (event.LeftIsDown()) {
			if (pY < _firstY) {
				_bottomzoomY = _firstY;
				_topzoomY = pY;
			}
			else {
				_bottomzoomY = pY;
				_topzoomY = _firstY;
			}

			if (_topzoomY < 0) _topzoomY = 0;
			if (_bottomzoomY > _height) _bottomzoomY = _height;

			Refresh(FALSE);
			updatePositionLabels(pX, pY, true);
		}
		
	}
	else if ((event.LeftDown() || (_dragging && event.Dragging() && event.LeftIsDown())) && !event.RightIsDown() && !_zooming)
	{
		// Pencil drawing
	   
		if (event.LeftDown()) {
			_firstX = _lastX = pX;
			_firstY = _lastY = pY;
			_dragging = true;
		}
		
		// modify spectrumModifier for bins
		float *values ;
		FTspectrumModifier *specmod=0;
		
		if (_topSpecMod) {
			if (event.ShiftDown() && _specMod) {
				// shift does regular specmod if there is a topspecmod
				values = _specMod->getValues();
				specmod =_specMod;
			}
			else {
				values = _topSpecMod->getValues();
				specmod = _topSpecMod;
			}
		}
		else if (_specMod) {
			values = _specMod->getValues();
			specmod = _specMod;
		}
		else {
			event.Skip();
			return;
		}
		
		int leftx, rightx;
		int sign;
		
		if (_lastX <= pX) {
			leftx = _lastX;
			rightx = pX;
			sign = 1;
		}
		else {
			leftx = pX;
			rightx = _lastX;
			sign = -1;
		}
		
		// compute values to modify
		int frombin, tobin, fromi, toi;
		xToBinRange(leftx, fromi, toi);
		frombin = fromi;
		xToBinRange(rightx, fromi, toi);
		tobin = toi;
		
		//int fromi = (int) (_lastX / xscale);
		//int toi = (int) (pX / xscale);

		int useY = pY;
		
		if (event.ControlDown()) {
			if (_firstCtrl) {
				_firstY = pY;
				_firstCtrl = false;
			}
			useY = _firstY;

			if (useY < 0) useY = 0;
			else if (useY > _height) useY = _height;

			float val = yToVal (useY);
			for ( i=frombin; i<=tobin; i++)
			{
				values[i] = val;
				//values[i] = (((_height - useY) / (float)_height)) * (_max-_min) + _min;
				//printf ("i=%d  values %g\n", i, values[i]);
			}

		}
		else {
			_firstCtrl = true;

			if (useY < 0) useY = 0;
			else if (useY > _height) useY = _height;

			// do linear interpolation between lasty and usey
			float adj = (_lastY - useY) / (float)(1 + tobin-frombin);
			//printf ("adjust is %g   useY=%d  lasty=%d\n", adj, useY, _lastY);
			float curradj = 0;
			float leftY;
			
			if (sign > 0) {
				leftY = _lastY;
			}
			else {
				leftY = useY;
			}
			
			for ( i=frombin; i<=tobin; i++)
			{
				values[i] = yToVal((int) (leftY - sign*curradj));
				//values[i] = (((_height - useY) / (float)_height)) * (_max-_min) + _min;
				//printf ("i=%d  values %g\n", i, values[i]);
				curradj += adj;
			}
		}
		
		
		Refresh(FALSE);
		_mainwin->updateGraphs(this, specm->getSpecModifierType());
		
		_lastX = pX;
		_lastY = useY;

		updatePositionLabels(pX, useY, true, specmod);

	}
	else if ((event.RightDown() || (_dragging && event.Dragging() && event.RightIsDown())) && !_zooming)
	{
		// shift entire contour around
		if (event.RightDown()) {
			_firstX = _lastX = pX;
			_firstY = _lastY = pY;
			SetCursor(wxCURSOR_HAND);
			_dragging = true;
		}

		float * valueslist[2];
		FTspectrumModifier *specmod = _specMod;
		bool edgehold = event.ControlDown();

		float *values;
		int totbins;

		if (_topSpecMod) {
			if (event.ShiftDown() && _specMod) {
				valueslist[0] = _specMod->getValues();
				specmod = _specMod;
				totbins = _specMod->getLength();
				valueslist[1] = 0;

				// do both
				if (event.LeftIsDown()) {
					valueslist[1] = _topSpecMod->getValues();
				}
			}
			else {
				valueslist[0] = _topSpecMod->getValues();
				specmod = _topSpecMod;
				totbins = _topSpecMod->getLength();
				valueslist[1] = 0;

				// do both
				if (event.LeftIsDown()) {
					valueslist[1] = _specMod->getValues();
				}
			}
		}
		else if (_specMod) {
			valueslist[0] = _specMod->getValues();
			specmod = _specMod;
			totbins = _specMod->getLength();
			valueslist[1] = 0;

			if (event.LeftIsDown() && _topSpecMod) {
				valueslist[1] = _topSpecMod->getValues();
			}
		}
		else {
			event.Skip();
			return;
		}

		
		// compute difference in x and y from last
		int diffX, diffY;
		diffX = pX - _lastX;
		diffY = pY - _lastY;
		
		int shiftbins = xDeltaToBinDelta (diffX);
		//float shiftval = yToVal(pY) - yToVal(_lastY);

		//printf ("shiftbins %d   diffx %d   diffy %d\n", shiftbins, diffX, diffY);

		for (int n = 0; n < 2 && valueslist[n]; n++)
		{
			values = valueslist[n];

			if (shiftbins < 0) {
				// shiftbins is NEGATIVE shift left

			// store first shiftbins
				for (i=0; i < -shiftbins; i++) {
					_tmpfilt[i] = valDiffY (values[i],  _lastY, pY);
				}
			
				for (i=0; i < totbins + shiftbins; i++) {
					values[i] = valDiffY (values[i-shiftbins], _lastY, pY);
				}

				// finish off with end
				if (edgehold) {
					for (i=totbins+shiftbins; i < totbins; i++) {
						values[i] = values[i-1];
					}
				}
				else {
					for (j=0, i=totbins+shiftbins; i < totbins; i++, j++) {
						values[i] = _tmpfilt[j];
					}
				}

			
			}
			else if (shiftbins > 0) {

				// shiftbins is POSITIVE, shift right
				// store last shiftbins
				for (i=totbins-shiftbins; i < totbins; i++) {
					_tmpfilt[i] = valDiffY (values[i], _lastY, pY);
				}
			
			
				for ( i=totbins-1; i >= shiftbins; i--) {
					values[i] = valDiffY (values[i-shiftbins],  _lastY, pY);
				}

				// start off with first values (either wrapped or edge held)
				if (edgehold) {
					for ( i=shiftbins-1; i >= 0; i--) {
						values[i] = values[i+1];
					}
				}
				else {
					for (j=totbins-shiftbins, i=0; i < shiftbins; i++, j++) {
						values[i] = _tmpfilt[j];
					}
				}


			}
			else {
				// no bin shift just values
				for ( i=0; i < totbins; i++) {
					values[i] = valDiffY (values[i], _lastY, pY);
				}
			}

		}
		
		Refresh(FALSE);
		_mainwin->updateGraphs(this, specm->getSpecModifierType());
			
		if (shiftbins != 0) {
			_lastX = pX;
		}
		
		_lastY = pY;

		updatePositionLabels(pX, pY, true, specmod);

	}
	else if (event.ButtonUp()  && !event.LeftIsDown() && !event.RightIsDown()) {
		ReleaseMouse();
		SetCursor(wxCURSOR_PENCIL);
		_dragging = false;

		if (event.RightUp() && event.ControlDown() && event.AltDown())
		{
			if (event.ShiftDown())
			{
				// reset zoom
				_zooming = false;
				if (_specMod->getModifierType() == FTspectrumModifier::GAIN_MODIFIER)
				{
					_mindb = _absmindb;
					_maxdb = _absmaxdb;
					Refresh(FALSE);
				}
				else {
					setMinMax (_absmin, _absmax);
				}
			}
			else {
				// reset filter
				if (_specMod) {
					_specMod->reset();
				}
				if (_topSpecMod) {
					_topSpecMod->reset();
				}
				
				Refresh(FALSE);
				
				_mainwin->updateGraphs(this, specm->getSpecModifierType());
				updatePositionLabels(pX, pY, true);
			}
		}
		else if (_zooming)
		{
			// commit zoom
			_zooming = false;
			setMinMax ( yToVal (_bottomzoomY), yToVal (_topzoomY) );
		}

		event.Skip();
	}
	else if (event.Moving())
	{
		if (_topSpecMod && !event.ShiftDown()) {
			updatePositionLabels(pX, pY, true, _topSpecMod);
		}
		else if (_specMod) {
			updatePositionLabels(pX, pY, true, _specMod);
		}
	}
	else {
		event.Skip();
	}

}

void FTactiveBarGraph::updatePositionLabels(int pX, int pY, bool showreal, FTspectrumModifier *specmod)
{
	// calculate freq range and val for status bar
	float sfreq, efreq;
	int frombin, tobin;
	xToFreqRange(pX, sfreq, efreq, frombin, tobin);
	_freqstr.Printf ("%5d - %5d Hz", (int) sfreq, (int) efreq);

	float val, realval;	

	if (!specmod) {
		if (_specMod) specmod = _specMod;
		else if (_topSpecMod) specmod = _topSpecMod;
		else return;
	}

	float *data = specmod->getValues();

	if (specmod->getModifierType() == FTspectrumModifier::GAIN_MODIFIER) {
		val = yToDb (pY);
		if (showreal) {
			realval = valToDb (data[frombin]);
			_valstr.Printf ("C: %8.1f dB   @: %8.1f dB", val, realval);
		}
		else {
			_valstr.Printf ("C: %8.1f dB", val);
		}
	}
	else if (specmod->getModifierType() == FTspectrumModifier::TIME_MODIFIER) {
		
		val = yToVal (pY);
		if (showreal) {
			realval = data[frombin];
			_valstr.Printf ("C: %8.0f ms    @: %8.0f ms", val * 1000.0, realval * 1000.0);
		}
		else {
			_valstr.Printf ("C: %8.0f ms", val * 1000.0);
		}
	}
	else if (specmod->getModifierType() == FTspectrumModifier::UNIFORM_MODIFIER) {
		
		val = yToVal (pY);
		if (showreal) {
			realval = data[frombin];
			_valstr.Printf ("C: %8.1f %%    @: %8.1f %%", val * 100.0, realval * 100.0);
		}
		else {
			_valstr.Printf ("C: %8.1f %%", val * 100.0);
		}
	}
	else if (specmod->getModifierType() == FTspectrumModifier::DB_MODIFIER) {
		val = yToVal (pY);
		if (showreal) {
			realval = data[frombin];
			_valstr.Printf ("C: %8.2f dB   @: %8.2f dB", val, realval);
		}
		else {
			_valstr.Printf ("C: %8.2f dB", val);
		}
		
	}
	else {
		val = yToVal (pY);
		if (showreal) {
			realval = data[frombin];
			_valstr.Printf ("C: %8.3f    @: %8.3f", val, realval);
		}
		else {
			_valstr.Printf ("C: %8.3f", val);
		}
		
	}
	
	_mainwin->updatePosition ( _freqstr, _valstr );
}


void FTactiveBarGraph::OnXscaleMenu (wxMenuEvent &event)
{
	//wxMenuItem *item = (wxMenuItem *) event.GetEventObject();

	
	if (event.GetId() == FT_1Xscale) {
		_xScaleType = XSCALE_1X;
	}
	else if (event.GetId() == FT_2Xscale) {
		_xScaleType = XSCALE_2X;
	}
	else if (event.GetId() == FT_LogaXscale) {
		_xScaleType = XSCALE_LOGA;
	}
	else if (event.GetId() == FT_LogbXscale) {
		_xScaleType = XSCALE_LOGB;
	}
	else {
		event.Skip();
	}

	Refresh(FALSE);
}
