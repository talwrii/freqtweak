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
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <values.h>

#include <wx/wx.h>
#include <wx/dir.h>
#include <wx/textfile.h>
#include <wx/listimpl.cpp>


#include "FTconfigManager.hpp"
#include "FTspectrumModifier.hpp"
#include "FTspectralManip.hpp"
#include "FTioSupport.hpp"
#include "FTprocessPath.hpp"


WX_DEFINE_LIST(FTstringList);


FTconfigManager::FTconfigManager(const char * basedir)
	: _basedir(basedir)
{

	if (_basedir.IsEmpty()) {
		_basedir = wxString::Format("%s/.freqtweak", getenv("HOME"));
	}

	// try to create basedir if it doesn't exist
        //wxDir bdir(_basedir);
	
	if ( ! wxDir::Exists(_basedir) ) {
		if (mkdir ( _basedir, 0755 )) {
			fprintf (stderr, "Error creating %s\n", _basedir.c_str()); 
		}
		else {
			fprintf(stderr, "Created settings directory: %s\n", _basedir.c_str());
		}
	}
	else {
		//printf ("config dir exists\n");
	}

	// make basedir/presets dir
	if ( ! wxDir::Exists(_basedir + "/presets") ) {
		if (mkdir ( _basedir + "/presets" , 0755 )) {
			fprintf (stderr, "Error creating %s/presets\n", _basedir.c_str()); 
		}
		else {
			fprintf(stderr, "Created presets directory: %s/presets\n", _basedir.c_str());
		}
	}
	else {
		//printf ("config_presets dir exists\n");
	}
	
	
}

FTconfigManager::~FTconfigManager()
{
}


bool FTconfigManager::storeSettings (const char * name)
{
	// directory to store settings
	wxString dirname(wxString::Format("%s/presets/%s", _basedir.c_str(), name));

	if ( ! wxDir::Exists(dirname) ) {
		if (mkdir ( dirname, 0755 )) {
			printf ("Error creating %s\n", dirname.c_str()); 
			return false;
		}
	}

	FTioSupport * iosup = FTioSupport::instance();

	// remove all of our files
	wxDir dir(dirname);
	if ( !dir.IsOpened() )
	{
		return false;
	}

	wxString filename;
	bool cont = dir.GetFirst(&filename);
	while ( cont )
	{
		//printf ("%s\n", filename.c_str());
		unlink (wxString::Format("%s/%s", dirname.c_str(), filename.c_str()).c_str() );
		cont = dir.GetNext(&filename);
	}


	for (int i=0; i < iosup->getActivePathCount(); i++)
	{
		FTprocessPath * procpath = iosup->getProcessPath(i);
		if (!procpath) continue; // shouldnt happen

		FTspectralManip *manip = procpath->getSpectralManip();

		
		// make config file for class settings
		wxTextFile conffile (wxString::Format("%s/config.%d", dirname.c_str(), i));

		if (! conffile.Create ()) {
			printf ("Error cannot create %s\n", conffile.GetName());
			return false;
		}

		conffile.AddLine (wxString::Format("fft_size=%d", manip->getFFTsize()));
		conffile.AddLine (wxString::Format("windowing=%d", (int) manip->getWindowing()));
		conffile.AddLine (wxString::Format("update_speed=%d", (int) manip->getUpdateSpeed()));
		conffile.AddLine (wxString::Format("oversamp=%d", manip->getOversamp()));
		conffile.AddLine (wxString::Format("input_gain=%.10g", manip->getInputGain()));
		conffile.AddLine (wxString::Format("mix_ratio=%.10g", manip->getMixRatio()));
		conffile.AddLine (wxString::Format("bypassed=%d", manip->getBypassed() ? 1: 0 ));
		conffile.AddLine (wxString::Format("muted=%d", manip->getMuted() ? 1 : 0 ));

		conffile.AddLine("");

	// now for the filter sections
	
		conffile.AddLine (wxString::Format("freq_bypassed=%d", manip->getBypassFreqFilter() ? 1: 0 ));
		conffile.AddLine (wxString::Format("scale_bypassed=%d", manip->getBypassScaleFilter() ? 1: 0 ));
		conffile.AddLine (wxString::Format("delay_bypassed=%d", manip->getBypassDelayFilter() ? 1: 0 ));
		conffile.AddLine (wxString::Format("feedback_bypassed=%d", manip->getBypassFeedbackFilter() ? 1: 0 ));	
		conffile.AddLine (wxString::Format("gate_bypassed=%d", manip->getBypassGateFilter() ? 1: 0 ));
		conffile.AddLine (wxString::Format("inverse_gate_bypassed=%d", manip->getBypassInverseGateFilter() ? 1: 0 ));

		conffile.AddLine("");
		
		conffile.AddLine (wxString::Format("freq_linked=%d",
						   (manip->getFreqFilter()->getLink()
						    ? manip->getFreqFilter()->getLink()->getId() : -1 )));
		conffile.AddLine (wxString::Format("scale_linked=%d",
						   (manip->getScaleFilter()->getLink()
						    ? manip->getScaleFilter()->getLink()->getId(): -1 )));
		conffile.AddLine (wxString::Format("delay_linked=%d",
						   (manip->getDelayFilter()->getLink()
						    ? manip->getDelayFilter()->getLink()->getId(): -1 )));
		conffile.AddLine (wxString::Format("feedback_linked=%d",
						   (manip->getFeedbackFilter()->getLink()
						    ? manip->getFeedbackFilter()->getLink()->getId(): -1 )));
		conffile.AddLine (wxString::Format("gate_linked=%d",
						   ( manip->getGateFilter()->getLink()
						     ? manip->getGateFilter()->getLink()->getId(): -1 )));
		conffile.AddLine (wxString::Format("inverse_gate_linked=%d",
						   (manip->getInverseGateFilter()->getLink()
						    ? manip->getInverseGateFilter()->getLink()->getId(): -1 )));


		conffile.AddLine("");
		// port connections
		const char ** inports = iosup->getConnectedInputPorts(i);
		wxString instr("input_ports=");
		if (inports) {
			for (int n=0; inports[n]; n++) {
				if (n == 0) {
					instr += wxString(inports[n]);
				}
				else {
					instr += wxString::Format(",%s", inports[n]);
				}
			}
			free(inports);
		}
		conffile.AddLine (instr);

		const char ** outports = iosup->getConnectedOutputPorts(i);
		wxString outstr("output_ports=");
		if (outports) {
			for (int n=0; outports[n]; n++) {
				if (n == 0) {
					outstr += wxString(outports[n]);
				}
				else {
					outstr += wxString::Format(",%s", outports[n]);
				}
			}
			free(outports);
		}
		conffile.AddLine (outstr);
		
		
		
		conffile.Write();
	
		// now create the filter files
		createFilterFiles (manip, dirname, i);
	}

	
	printf ("Stored settings into %s\n", dirname.c_str());
	
	return true;
}

bool FTconfigManager::loadSettings (const char * name)
{
	wxString dirname(wxString::Format("%s/presets/%s", _basedir.c_str(), name));

	if ( ! wxDir::Exists(dirname) ) {
		printf ("Settings %s does not exist!\n", dirname.c_str()); 
		return false;
	}

	FTioSupport * iosup = FTioSupport::instance();

	int i;	
	for (i=0; i < FT_MAXPATHS; i++)
	{
		// get config file for class settings
		wxTextFile conffile (wxString::Format("%s/config.%d", dirname.c_str(), i) );
		if (!conffile.Exists()) {
			//printf ("%d path found!\n", i);
			// enforce 0-n ordering for configs
			break;
		}

	        iosup->setProcessPathActive(i, true);
	}
	// set all remaining paths inactive
	for ( ; i < FT_MAXPATHS; i++) {
		iosup->setProcessPathActive(i, false);
	}

	for (i=0; i < iosup->getActivePathCount(); i++)
	{
		wxTextFile conffile (wxString::Format("%s/config.%d", dirname.c_str(), i) );

		FTprocessPath * procpath = iosup->getProcessPath(i);
		if (!procpath) continue; // shouldnt happen

		FTspectralManip * manip = procpath->getSpectralManip();

		if (conffile.Open())
		{
			// parse lines from it
			wxString line;
		
			line = conffile.GetFirstLine();	
			for ( unsigned int n =0;  n < conffile.GetLineCount(); n++ )
			{
				line = conffile[n];
				line.Trim(true);
				line.Trim(false);
			
				if (line.IsEmpty() || line.GetChar(0) == '#')
				{
					continue; // ignore
				}
			
				// look for = separating key and value
				wxString key;
				wxString value;
			
				int pos = line.find ('=');
				if (pos >= 0) {
					key = line.substr(0, pos);
					value = line.Mid(pos+1).Strip(wxString::both);
					//printf ("key is %s, value is %s\n", key.c_str(), value.c_str());

					modifySetting (manip, i, key, value);
				}
			
			}
		
		}
		else {
			printf ("Warning: %s could not be opened!\n", conffile.GetName());
		}


		loadFilterFiles (manip, dirname, i);
		
	}

	
	
	return true;
}


void FTconfigManager::loadFilterFiles(FTspectralManip *manip, wxString & dirname, int i)
{
	// freq filter
	wxTextFile freqfile (wxString::Format("%s/freq.%d.filter", dirname.c_str(), i));
	if (freqfile.Open()) {
		loadFilter (manip->getFreqFilter(), freqfile);
	}

	// scale filter
	wxTextFile scalefile  (wxString::Format("%s/scale.%d.filter", dirname.c_str(), i));
	if (scalefile.Open()) {
		loadFilter (manip->getScaleFilter(), scalefile);
	}
	
	// inverse filter
	wxTextFile invgatefile  (wxString::Format("%s/inverse_gate.%d.filter", dirname.c_str(), i));
	if (invgatefile.Open()) {
		loadFilter (manip->getInverseGateFilter(), invgatefile);
	}
	
	// gate filter
	wxTextFile gatefile  (wxString::Format("%s/gate.%d.filter", dirname.c_str(), i));
	if (gatefile.Open()) {
		loadFilter (manip->getGateFilter(), gatefile);		
	}

	// delay filter
	wxTextFile delayfile  (wxString::Format("%s/delay.%d.filter", dirname.c_str(), i));
	if (delayfile.Open()) {
		loadFilter (manip->getDelayFilter(), delayfile);
	}

	// feedback filter
	wxTextFile feedbackfile  (wxString::Format("%s/feedback.%d.filter", dirname.c_str(), i));
	if (feedbackfile.Open()) {
		loadFilter (manip->getFeedbackFilter(), feedbackfile);
	}

}

void FTconfigManager::loadFilter (FTspectrumModifier *specmod, wxTextFile & tf)
{
	// FORMAT FOR FILTER FILES
	// -----------------------
	//
	// One line per bin description, a bin description is:
	//   [start_bin:stop_bin] value
	// 
	// If the optional bin range is missing (one token in line) then
	// the value is assigned to the bin following the most recently filled bin.
	// The bin indexes start from 0 and the ranges are inclusive

	float *values = specmod->getValues();
	
	// parse lines from it
	wxString line;

	line = tf.GetFirstLine();	

	int lastbin = -1;
	double val;
	unsigned long sbin, ebin;
	
	for ( unsigned int i =0;  i < tf.GetLineCount(); i++ )
	{
		line = tf[i];
		line.Trim(true);
		line.Trim(false);

		if (line.IsEmpty() || line.GetChar(0) == '#')
		{
			continue; // ignore
		}

		// look for whitespace separating two possible tokens
		wxString rangestr;
		wxString value;
		
		int pos = line.find_first_of (" \t");
		if (pos >= 0) {
			rangestr = line.substr(0, pos);
			value = line.Mid(pos).Strip(wxString::both);
			// printf ("rangestr is %s, value is %s\n", rangestr.c_str(), value.c_str());

			if (rangestr.BeforeFirst(':').ToULong(&sbin)
			    && rangestr.AfterFirst(':').ToULong(&ebin))
			{
				for (unsigned int j=sbin; j <=ebin; j++) {
					if (value.ToDouble(&val)) {
						values[j] = (float) val;
					}
				}
				lastbin = ebin;
								
			}
		}
		else {
			// just value
			value = line;
			lastbin += 1;
			// printf ("bin=%d  value is %s\n", lastbin, value.c_str());

			if (value.ToDouble(&val)) {
				values[lastbin] = (float) val;
			}
			

		}
		
	}

	
}



void FTconfigManager::createFilterFiles(FTspectralManip *manip, wxString &dirname, int i)
{

	// freq filter
	wxTextFile freqfile (wxString::Format("%s/freq.%d.filter", dirname.c_str(), i));
	if (freqfile.Exists()) {
		// remove it
		unlink (freqfile.GetName());
	}
	freqfile.Create ();
	writeFilter (manip->getFreqFilter(), freqfile);
	freqfile.Write();

	// scale filter
	wxTextFile scalefile (wxString::Format("%s/scale.%d.filter", dirname.c_str(), i));
	if (scalefile.Exists()) {
		// remove it
		unlink (scalefile.GetName());
	}
	scalefile.Create ();
	writeFilter (manip->getScaleFilter(), scalefile);
	scalefile.Write();

	
	// squelch filter
	wxTextFile invgatefile (wxString::Format("%s/inverse_gate.%d.filter", dirname.c_str(), i));
	if (invgatefile.Exists()) {
		// remove it
		unlink (invgatefile.GetName());
	}
	invgatefile.Create ();
	writeFilter (manip->getInverseGateFilter(), invgatefile);
	invgatefile.Write();
	
	// gate filter
	wxTextFile gatefile  (wxString::Format("%s/gate.%d.filter", dirname.c_str(), i));
	if (gatefile.Exists()) {
		// remove it
		unlink (gatefile.GetName());
	}
	gatefile.Create ();
	writeFilter (manip->getGateFilter(), gatefile);
	gatefile.Write();

	// delay filter
	wxTextFile delayfile (wxString::Format("%s/delay.%d.filter", dirname.c_str(), i));
	if (delayfile.Exists()) {
		// remove it
		unlink (delayfile.GetName());
	}
	delayfile.Create ();
	writeFilter (manip->getDelayFilter(), delayfile);
	delayfile.Write();

	// feedback filter
	wxTextFile feedbackfile  (wxString::Format("%s/feedback.%d.filter", dirname.c_str(), i));
	if (feedbackfile.Exists()) {
		// remove it
		unlink (feedbackfile.GetName());
	}
	feedbackfile.Create ();
	writeFilter (manip->getFeedbackFilter(), feedbackfile);
	feedbackfile.Write();
	
}


void FTconfigManager::writeFilter (FTspectrumModifier *specmod, wxTextFile & tf)
{

	// FORMAT FOR FILTER FILES
	// -----------------------
	//
	// One line per bin description, a bin description is:
	//   [start_bin:stop_bin] value
	// 
	// If the optional bin range is missing (one token in line) then
	// the value is assigned to the bin following the most recently filled bin.
	// The bin indexes start from 0 and the ranges are inclusive
	
	float * values = specmod->getValues();
	
	int totbins = specmod->getLength();
	int pos = 0;
	int i;

	float lastval = values[0];
	
	for (i = 1; i < totbins; i++)
	{
//		if (wxString::Format("%.10g", values[i]) == wxString::Format("%.20g", lastval)) {
		if (values[i] == lastval) {
			continue;
		}
		else if (i == pos + 1 ) {
			// just write last number
			tf.AddLine ( wxString::Format ("%.20g", lastval));
			pos = i;
			lastval = values[i];
		}
		else {
			// write range
			tf.AddLine ( wxString::Format ("%d:%d  %.20g", pos, i-1, values[pos]));
			pos = i;
			lastval = values[i];
		}

	}

	// write last
	if (pos < totbins) {
		tf.AddLine ( wxString::Format ("%d:%d  %.20g", pos, totbins-1, values[pos]));
	}
}


void FTconfigManager::modifySetting (FTspectralManip *manip, int id, wxString &key, wxString &value)
{

	double fval;
	unsigned long ival;
	FTioSupport * iosup = FTioSupport::instance();
	
	if (key == "fft_size") {
		if (value.ToULong(&ival)) {
			manip->setFFTsize ((FTspectralManip::FFT_Size) ival);
		}
	}
	else if (key == "windowing") {
		if (value.ToULong(&ival)) {
			manip->setWindowing ((FTspectralManip::Windowing) ival);
		}
	}
	else if (key == "update_speed") {
		if (value.ToULong(&ival)) {
			manip->setUpdateSpeed ((FTspectralManip::UpdateSpeed)(int)ival);
		}
	}
	else if (key == "oversamp") {
		if (value.ToULong(&ival)) {
			manip->setOversamp ((int)ival);
		}
	}
	else if (key == "input_gain") {
		if (value.ToDouble(&fval)) {
			manip->setInputGain ((float) fval);
		}
	}
	else if (key == "mix_ratio") {
		if (value.ToDouble(&fval)) {
			manip->setMixRatio ((float) fval);
		}
	}
	else if (key == "bypassed") {
		if (value.ToULong(&ival)) {
			manip->setBypassed (ival==1 ? true: false);
		}
	}
	else if (key == "muted") {
		if (value.ToULong(&ival)) {
			manip->setMuted (ival==1 ? true: false);
		}
	}
	else if (key == "freq_bypassed") {
		if (value.ToULong(&ival)) {
			manip->setBypassFreqFilter (ival==1 ? true: false);
		}
	}
	else if (key == "scale_bypassed") {
		if (value.ToULong(&ival)) {
			manip->setBypassScaleFilter (ival==1 ? true: false);
		}
	}
	else if (key == "delay_bypassed") {
		if (value.ToULong(&ival)) {
			manip->setBypassDelayFilter (ival==1 ? true: false);
		}
	}
	else if (key == "gate_bypassed") {
		if (value.ToULong(&ival)) {
			manip->setBypassGateFilter (ival==1 ? true: false);
		}
	}
	else if (key == "inverse_gate_bypassed") {
		if (value.ToULong(&ival)) {
			manip->setBypassInverseGateFilter (ival==1 ? true: false);
		}
	}
	else if (key == "feedback_bypassed") {
		if (value.ToULong(&ival)) {
			manip->setBypassFeedbackFilter (ival==1 ? true: false);
		}
	}

	// link stuff
	else if (key == "freq_linked") {
		if (value.ToULong(&ival)) {
			if (ival >= 0 && iosup->getProcessPath(ival))
			{
				manip->getFreqFilter()->link (iosup->getProcessPath(ival)->getSpectralManip()->getFreqFilter());
			}
			else {
				manip->getFreqFilter()->unlink(false);
			}
		}
	}
	else if (key == "scale_linked") {
		if (value.ToULong(&ival)) {
			if (ival >= 0 && iosup->getProcessPath(ival))
			{
				manip->getScaleFilter()->link (iosup->getProcessPath(ival)->getSpectralManip()->getScaleFilter());
			}
			else {
				manip->getScaleFilter()->unlink(false);
			}
		}
	}
	else if (key == "delay_linked") {
		if (value.ToULong(&ival)) {
			if (ival >= 0 && iosup->getProcessPath(ival))
			{
				manip->getDelayFilter()->link (iosup->getProcessPath(ival)->getSpectralManip()->getDelayFilter());
			}
			else {
				manip->getDelayFilter()->unlink(false);
			}
		}
	}
	else if (key == "gate_linked") {
		if (value.ToULong(&ival)) {
			if (ival >= 0 && iosup->getProcessPath(ival))
			{
				manip->getGateFilter()->link (iosup->getProcessPath(ival)->getSpectralManip()->getGateFilter());
			}
			else {
				manip->getGateFilter()->unlink(false);
			}
		}
	}
	else if (key == "inverse_gate_linked") {
		if (value.ToULong(&ival)) {
			if (ival >= 0 && iosup->getProcessPath(ival))
			{
				manip->getInverseGateFilter()->link (iosup->getProcessPath(ival)->getSpectralManip()->getInverseGateFilter());
			}
			else {
				manip->getInverseGateFilter()->unlink(false);
			}
		}
	}
	else if (key == "feedback_linked") {
		if (value.ToULong(&ival)) {
			if (ival >= 0 && iosup->getProcessPath(ival))
			{
				manip->getFeedbackFilter()->link (iosup->getProcessPath(ival)->getSpectralManip()->getFeedbackFilter());
			}
			else {
				manip->getFeedbackFilter()->unlink(false);
			}
		}
	}

	// io stuff
	else if (key == "input_ports") {
		// value is comma separated list of port names
		iosup->disconnectPathInput(id, NULL); // disconnect all

		wxString port = value.BeforeFirst(',');
		wxString remain = value.AfterFirst(',');
		while (!port.IsEmpty()) {
			iosup->connectPathInput(id, port.c_str());

			port = remain.BeforeFirst(',');
			remain = remain.AfterFirst(',');
		}
		
	}
	else if (key == "output_ports") {
		// value is comma separated list of port names
		iosup->disconnectPathOutput(id, NULL); // disconnect all

		wxString port = value.BeforeFirst(',');
		wxString remain = value.AfterFirst(',');
		while (!port.IsEmpty()) {
			iosup->connectPathOutput(id, port.c_str());

			port = remain.BeforeFirst(',');
			remain = remain.AfterFirst(',');
		}
		
	}

	else {
		printf ("none of the above\n");
	}
}




FTstringList * FTconfigManager::getSettingsNames()
{
	wxString dirname(wxString::Format("%s/presets", _basedir.c_str()));

	wxDir dir(dirname);
	if ( !dir.IsOpened() ) {
		return false;
	}

	wxString filename;

	FTstringList * flist = new FTstringList();
	
	bool cont = dir.GetFirst(&filename, "", wxDIR_DIRS);

	while ( cont )
	{
		//printf ("%s\n", filename.c_str());
		flist->Append (new wxString(filename));
		cont = dir.GetNext(&filename);
	}
	
	flist->DeleteContents(TRUE);
	return flist;
}

