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

#ifndef __FTACTIVEBARGRAPH_HPP__
#define __FTACTIVEBARGRAPH_HPP__

#include <wx/wx.h>

#include "FTtypes.hpp"

class FTspectrumModifier;
class FTmainwin;


class FTactiveBarGraph
	: public wxPanel
{
  public:
	FTactiveBarGraph(FTmainwin *win, wxWindow *parent, wxWindowID id, 
		      const wxPoint& pos = wxDefaultPosition,
		      const wxSize& size = wxDefaultSize,
		      long style = wxRAISED_BORDER,
		      const wxString& name = "ActiveBarGraph");

	virtual ~FTactiveBarGraph();

	
	void setSpectrumModifier (FTspectrumModifier *sm);
	FTspectrumModifier * getSpectrumModifier() { return _specMod; }

	void setTopSpectrumModifier (FTspectrumModifier *sm);
	FTspectrumModifier * getTopSpectrumModifier() { return _topSpecMod; }
	
	
	void setXscale(XScaleType sc);
	XScaleType getXscale() { return _xScaleType; }
	
	void OnPaint ( wxPaintEvent &event);
	void OnSize ( wxSizeEvent &event);
	void OnMouseActivity ( wxMouseEvent &event );
	void OnXscaleMenu (wxMenuEvent &event);

	void recalculate();
	
  protected:

	void xToBinRange(int x, int &frombin, int &tobin);
	void binToXRange(int bin, int &fromx, int &tox);
	int xDeltaToBinDelta(int xdelt);
	void xToFreqRange(int x, float &fromfreq, float &tofreq, int &frombin, int &tobin);
	
	int valToY(float val);
	float yToVal(int y);

	float valToDb(float val);
	float dbToVal(float db);
	float yToDb(int y);

	float valDiffY(float val, int lasty, int newy);

	void updatePositionLabels(int pX, int pY, bool showreal=false, FTspectrumModifier *specmod=0);
	
	
	int _width, _height;

	int _plotWidth, _plotHeight;
	int _leftWidth;
	int _topHeight;
	int _rightWidth;
	int _bottomHeight;
	
	FTspectrumModifier * _specMod;
	FTspectrumModifier * _topSpecMod;

	float _min, _max;
	
	float _xscale;

	float _mindb;
	
	float *_tmpfilt, *_toptmpfilt;
	
	wxColour _barColor0,_barColor1;
	wxColour _barColor2,_barColor3, _barColorDead, _tipColor;
	wxBrush  _barBrush0, _barBrush1, _barBrush2, _barBrush3, _barBrushDead ,_tipBrush;
	wxBrush _bgBrush;
	wxColour _penColor;
	wxPen  _barPen;
	
	wxBitmap * _backingMap;

	XScaleType _xScaleType;
	
	// mouse stuff
	int _lastX, _lastY;
	int _firstX, _firstY;
	bool _dragging;
	bool _firstCtrl;
	
	wxString _freqstr, _valstr;

	wxMenu * _xscaleMenu;
	
	FTmainwin * _mainwin;
	
  private:
	// any class wishing to process wxWindows events must use this macro
	DECLARE_EVENT_TABLE()	

};


#endif
