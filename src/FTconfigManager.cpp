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
#include <wx/filename.h>

#include "FTconfigManager.hpp"
#include "FTspectrumModifier.hpp"
#include "FTspectralEngine.hpp"
#include "FTioSupport.hpp"
#include "FTprocessPath.hpp"
#include "FTdspManager.hpp"
#include "version.h"

#include "xml++.hpp"

WX_DEFINE_LIST(FTstringList);


FTconfigManager::FTconfigManager(const char * basedir)
	: _basedir(basedir)
{

	if (_basedir.IsEmpty()) {
		char * homestr = getenv("HOME");

		if (homestr) {
			_basedir = wxString::Format("%s%c.freqtweak", homestr, wxFileName::GetPathSeparator());
		}
		else {
			_basedir = ".freqtweak";
		}
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
	if ( ! wxDir::Exists(_basedir + wxFileName::GetPathSeparator() + "presets") ) {
		wxString predir =  wxString::Format("%s%cpresets", _basedir.c_str(), wxFileName::GetPathSeparator());
		if (mkdir (predir.c_str(), 0755 )) {
			fprintf (stderr, "Error creating %s\n", predir.c_str()); 
		}
		else {
			fprintf(stderr, "Created presets directory: %s\n", predir.c_str());
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
	if (strcmp (name, "") == 0) {
		return false;
	}
		
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


	// make xmltree
	XMLTree configdoc;
	XMLNode * rootNode = new XMLNode("Preset");
	rootNode->add_property("version", freqtweak_version);
	configdoc.set_root (rootNode);
	
	// Params node has global dsp settings
	XMLNode * paramsNode = rootNode->add_child ("Params");

	XMLNode * channelsNode = rootNode->add_child ("Channels");
	
	for (int i=0; i < iosup->getActivePathCount(); i++)
	{
		FTprocessPath * procpath = iosup->getProcessPath(i);
		if (!procpath) continue; // shouldnt happen

		FTspectralEngine *engine = procpath->getSpectralEngine();

		if (i==0)
		{
			// pull global params from first procpath
			paramsNode->add_property ("fft_size", wxString::Format("%d", engine->getFFTsize()).c_str());
			paramsNode->add_property ("windowing", wxString::Format("%d", engine->getWindowing()).c_str());			
			paramsNode->add_property ("update_speed", wxString::Format("%d", engine->getUpdateSpeed()).c_str());			
			paramsNode->add_property ("oversamp", wxString::Format("%d", engine->getOversamp()).c_str());			
			paramsNode->add_property ("tempo", wxString::Format("%d", engine->getTempo()).c_str());			
			paramsNode->add_property ("max_delay", wxString::Format("%.10g", engine->getMaxDelay()).c_str());			
		}


		vector<FTprocI *> procmods;
		engine->getProcessorModules (procmods);

		XMLNode * chanNode = channelsNode->add_child ("Channel");

		chanNode->add_property ("pos", wxString::Format("%d", i).c_str());
		chanNode->add_property ("input_gain", wxString::Format("%.10g", engine->getInputGain()).c_str());
		chanNode->add_property ("mix_ratio", wxString::Format("%.10g", engine->getMixRatio()).c_str());
		chanNode->add_property ("bypassed", wxString::Format("%d", engine->getBypassed() ? 1: 0).c_str());
		chanNode->add_property ("muted", wxString::Format("%d", engine->getMuted() ? 1 : 0).c_str());
		
		
		// now for the filter sections

		XMLNode * procmodsNode = chanNode->add_child ("ProcMods");
		
		for (unsigned int n=0; n < procmods.size(); ++n)
		{
			FTprocI *pm = procmods[n];
			vector<FTspectrumModifier *> filts;
			pm->getFilters (filts);

			XMLNode * pmNode = procmodsNode->add_child ("ProcMod");
			pmNode->add_property ("pos", wxString::Format("%d", n).c_str());
			pmNode->add_property ("name", pm->getName());
			
			for (unsigned int m=0; m < filts.size(); ++m)
			{
				XMLNode * filtNode = pmNode->add_child ("Filter");

				filtNode->add_property ("pos", wxString::Format("%d", m).c_str());
				filtNode->add_property ("name", filts[m]->getConfigName().c_str());
				filtNode->add_property ("linked", wxString::Format("%d",
										   filts[m]->getLink() ?
										   filts[m]->getLink()->getId() : -1).c_str());
					
				filtNode->add_property ("bypassed", wxString::Format("%d",
										     filts[m]->getBypassed() ? 1 : 0).c_str());

				string filtfname = wxString::Format("%d_%d_%s.filter", i, n, filts[m]->getConfigName().c_str()).c_str();
				filtNode->add_property ("file", filtfname);

				// write out filter file
				wxTextFile filtfile (wxString::Format("%s%c%s", dirname.c_str(), wxFileName::GetPathSeparator(),
								      filtfname.c_str()));
				if (filtfile.Exists()) {
					// remove it
					unlink (filtfile.GetName());
				}
				filtfile.Create ();
				writeFilter (filts[m], filtfile);
				filtfile.Write();
				filtfile.Close();
			}

		}


		// port connections		
		XMLNode * inputsNode = chanNode->add_child ("Inputs");

		const char ** inports = iosup->getConnectedInputPorts(i);
		if (inports) {
			for (int n=0; inports[n]; n++) {

				XMLNode * portNode = inputsNode->add_child ("Port");
				portNode->add_property ("name", inports[n]);
			}
			free(inports);
		}

		XMLNode * outputsNode = chanNode->add_child ("Outputs");
		
		const char ** outports = iosup->getConnectedOutputPorts(i);
		if (outports) {
			for (int n=0; outports[n]; n++) {

				XMLNode * portNode = outputsNode->add_child ("Port");
				portNode->add_property ("name", outports[n]);
			}
			free(outports);
		}

	}

	// write doc to file
	
	if (configdoc.write (wxString::Format("%s%c%s", dirname.c_str(), wxFileName::GetPathSeparator(), "config.xml").c_str()))
	{	    
		fprintf (stderr, "Stored settings into %s\n", dirname.c_str());
		return true;
	}
	else {
		fprintf (stderr, "Failed to store settings into %s\n", dirname.c_str());
		return false;
	}
}

bool FTconfigManager::loadSettings (const char * name, bool restore_ports)
{
	if (strcmp (name, "") == 0) {
		return false;
	}
	
	wxString dirname(wxString::Format("%s%cpresets/%s", _basedir.c_str(), wxFileName::GetPathSeparator(), name));

	if ( ! wxDir::Exists(dirname) ) {
		printf ("Settings %s does not exist!\n", dirname.c_str()); 
		return false;
	}

	FTioSupport * iosup = FTioSupport::instance();

	
	// open file
	string configfname(wxString::Format("%s%c%s", dirname.c_str(), wxFileName::GetPathSeparator(), "config.xml").c_str());
	XMLTree configdoc (configfname);

	if (!configdoc.initialized()) {
		fprintf (stderr, "Error loading config at %s!\n", configfname.c_str()); 
		return false;
	}

	XMLNode * rootNode = configdoc.root();
	if (!rootNode || rootNode->name() != "Preset") {
		fprintf (stderr, "Preset root node not found in %s!\n", configfname.c_str()); 
		return false;
	}

	// get channels

	XMLNode * channelsNode = find_named_node (rootNode, "Channels");
	if (!channelsNode ) {
		fprintf (stderr, "Preset Channels node not found in %s!\n", configfname.c_str()); 
		return false;
	}

	XMLNodeList chanlist = channelsNode->children();
	if (chanlist.size() < 1) {
		fprintf (stderr, "No channels found in %s!\n", configfname.c_str()); 
		return false;
	}

	unsigned int i;
	for (i=0; i < chanlist.size() && i < FT_MAXPATHS; i++)
	{
	        iosup->setProcessPathActive(i, true);
	}
	// set all remaining paths inactive
	for ( ; i < FT_MAXPATHS; i++) {
		iosup->setProcessPathActive(i, false);
	}


	// get global params
	unsigned long fft_size = 1024;
	unsigned long windowing = 0;
	unsigned long update_speed = 2;
	unsigned long oversamp = 4;
	unsigned long tempo = 120;
	double        max_delay = 2.5;

	XMLNode * paramsNode = find_named_node (rootNode, "Params");
	if (paramsNode)
	{
		XMLPropertyConstIterator propiter;
		XMLPropertyList proplist = paramsNode->properties();
		
		for (propiter=proplist.begin(); propiter != proplist.end(); ++propiter)
		{
			string key = (*propiter)->name();
			wxString value = (*propiter)->value().c_str();

			if (key == "fft_size") {
				value.ToULong(&fft_size);
			}
			else if (key == "windowing") {
				value.ToULong(&windowing);
			}
			else if (key == "update_speed") {
				value.ToULong(&update_speed);
			}
			else if (key == "oversamp") {
				value.ToULong(&oversamp);
			}
			else if (key == "tempo") {
				value.ToULong(&tempo);
			}
			else if (key == "max_delay") {
				value.ToDouble(&max_delay);
			}
		}
	}


	_linkCache.clear();
	
	// clear all procpaths
	XMLNodeConstIterator chaniter;
	XMLNode * chanNode;
	double fval;
	unsigned long uval;
	
	for (chaniter=chanlist.begin(); chaniter != chanlist.end(); ++chaniter)
	{
		chanNode = *chaniter;

		XMLProperty * prop;

		if (!(prop = chanNode->property ("pos"))) {
			fprintf (stderr, "pos missing in channel!\n"); 
			continue;
		}
		
		unsigned long chan_pos;
		wxString tmpstr (prop->value().c_str());
		if (!tmpstr.ToULong (&chan_pos) || chan_pos >= FT_MAXPATHS) {
			fprintf (stderr, "invalid pos in channel!\n"); 
			continue;
		}

		FTprocessPath * procpath = iosup->getProcessPath((int) chan_pos);
		if (!procpath) continue; // shouldnt happen

		FTspectralEngine * engine = procpath->getSpectralEngine();

		// apply some of the global settings now
		engine->setOversamp ((int) oversamp);
		engine->setTempo ((int) tempo);
		engine->setMaxDelay ((float) max_delay);

		
		// get channel settings
		XMLPropertyConstIterator propiter;
		XMLPropertyList proplist = chanNode->properties();

		for (propiter=proplist.begin(); propiter != proplist.end(); ++propiter)
		{
			string key = (*propiter)->name();
			wxString value = (*propiter)->value().c_str();

			if (key == "input_gain") {
				if (value.ToDouble(&fval)) {
					engine->setInputGain ((float) fval);
				}
			}
			else if (key == "mix_ratio") {
				if (value.ToDouble(&fval)) {
					engine->setMixRatio ((float) fval);
				}
			}
			else if (key == "bypassed") {
				if (value.ToULong(&uval)) {
					engine->setBypassed (uval==1 ? true: false);
				}
			}
			else if (key == "muted") {
				if (value.ToULong(&uval)) {
					engine->setMuted (uval==1 ? true: false);
				}
			}	
		}


		// clear existing procmods
		engine->clearProcessorModules();
		
		// get procmods node
		XMLNode * procmodsNode = find_named_node (chanNode, "ProcMods");
		
		if ( !procmodsNode ) {
			fprintf (stderr, "Preset ProcMods node not found in %s!\n", configfname.c_str()); 
			return false;
		}

		XMLNodeList pmlist = procmodsNode->children();
		XMLNodeConstIterator pmiter;
		XMLNode * pmNode;

		for (pmiter=pmlist.begin(); pmiter != pmlist.end(); ++pmiter)
		{
			pmNode = *pmiter;

			if (!(prop = pmNode->property ("pos"))) {
				fprintf (stderr, "pos missing in procmod!\n"); 
				continue;
			}
			unsigned long ppos;
			tmpstr = prop->value().c_str();
			if (!tmpstr.ToULong (&ppos)) {
				fprintf (stderr, "invalid pos in procmod!\n"); 
				continue;
			}

			if (!(prop = pmNode->property ("name"))) {
				fprintf (stderr, "name missing in procmod!\n"); 
				continue;
			}
			string pmname = prop->value();

			// construct new procmod
			FTprocI * procmod = FTdspManager::instance()->getModuleByName(pmname);
			if (!procmod) {
				fprintf (stderr, "no proc module '%s' supported\n", pmname.c_str()); 
				continue;
			}
			procmod = procmod->clone();

			// must call this before initialization
			procmod->setMaxDelay ((float)max_delay);

			procmod->initialize();

			procmod->setSampleRate (iosup->getSampleRate());
			procmod->setOversamp ((int)oversamp);

			// load up the filters in the procmod
			
			XMLNodeList filtlist = pmNode->children();
			XMLNodeConstIterator filtiter;
			XMLNode * filtNode;

			for (filtiter=filtlist.begin(); filtiter != filtlist.end(); ++filtiter)
			{
				filtNode = *filtiter;
				
				if (!(prop =filtNode->property ("pos"))) {
					fprintf (stderr, "pos missing in filter!\n"); 
					continue;
				}
				unsigned long fpos;
				tmpstr = prop->value().c_str();
				if (!tmpstr.ToULong (&fpos)) {
					fprintf (stderr, "invalid filter pos in channel!\n"); 
					continue;
				}

				if (!(prop = filtNode->property ("file"))) {
					fprintf (stderr, "filter filename missing in procmod!\n"); 
					continue;
				}
				string filtfname = prop->value();

				FTspectrumModifier * specmod = procmod->getFilter (fpos);
				if (!specmod) {
					fprintf (stderr, "no filter at index %lu in procmod!\n", fpos); 
					continue;
				}

				// load filter
				wxTextFile filtfile (wxString::Format("%s%c%d_%d_%s.filter", dirname.c_str(), wxFileName::GetPathSeparator(),
								      (int) chan_pos, (int) ppos,
								      specmod->getConfigName().c_str()));
				if (filtfile.Open()) {

					loadFilter (specmod, filtfile);
					filtfile.Close();
				}

				// set bypassed
				if ((prop = filtNode->property ("bypassed"))) {
					wxString value (prop->value().c_str());
					if (value.ToULong(&uval)) {
						specmod->setBypassed (uval==1 ? true: false);
					}
				}

				// actual linkage must wait for later
				long linked = -1;
				if ((prop = filtNode->property ("linked"))) {
					wxString value (prop->value().c_str());
					if (value.ToLong(&linked) && linked >= 0) {
						
						_linkCache.push_back (LinkCache(chan_pos, linked, ppos, fpos));
					}
				}
				
				
			}

			// insert procmod

			engine->insertProcessorModule (procmod, ppos);
			
		}


		// apply global settings
		engine->setFFTsize ((FTspectralEngine::FFT_Size) fft_size);
		engine->setWindowing ((FTspectralEngine::Windowing) windowing);
		engine->setUpdateSpeed ((FTspectralEngine::UpdateSpeed)(int) update_speed);
		

		// input ports
		
		XMLNode * inputsNode = find_named_node (chanNode, "Inputs");
		if (inputsNode )
		{
			XMLNodeList portlist = inputsNode->children();
			XMLNodeConstIterator portiter;

			iosup->disconnectPathInput(chan_pos, NULL); // disconnect all
			
			for (portiter = portlist.begin(); portiter != portlist.end(); ++portiter)
			{
				XMLNode * port = *portiter;
				if (port->name() == "Port") {
					XMLProperty * prop = port->property("name");
					if (prop) {
						iosup->connectPathInput(chan_pos, prop->value().c_str());
					}
				}
			}
			
		}
		else {
			fprintf (stderr, "channel inputs node not found in %s!\n", configfname.c_str()); 
		}

		// output ports
		XMLNode * outputsNode = find_named_node (chanNode, "Outputs");
		if (inputsNode )
		{
			XMLNodeList portlist = outputsNode->children();
			XMLNodeConstIterator portiter;

			iosup->disconnectPathOutput(chan_pos, NULL); // disconnect all
			
			for (portiter = portlist.begin(); portiter != portlist.end(); ++portiter)
			{
				XMLNode * port = *portiter;
				if (port->name() == "Port") {
					XMLProperty * prop = port->property("name");
					if (prop) {
						iosup->connectPathOutput(chan_pos, prop->value().c_str());
					}
				}
			}
			
		}
		else {
			fprintf (stderr, "channel outputs node not found in %s!\n", configfname.c_str()); 
		}
		
		
	}

	// now we can apply linkages
	list<LinkCache>::iterator liter;
	for (liter = _linkCache.begin(); liter != _linkCache.end(); ++liter)
	{
		LinkCache & lc = *liter;

		FTspectrumModifier *source = iosup->getProcessPath(lc.source_chan)->getSpectralEngine()
			->getProcessorModule(lc.mod_n)->getFilter(lc.filt_n);

		FTspectrumModifier *dest = iosup->getProcessPath(lc.dest_chan)->getSpectralEngine()
			->getProcessorModule(lc.mod_n)->getFilter(lc.filt_n);

		source->link (dest);
		
	}
	
	
	return true;
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


// void FTconfigManager::modifySetting (FTspectralEngine *engine, int id, string &key, string &val, bool restoreports)
// {

// 	double fval;
// 	unsigned long ival;
// 	FTioSupport * iosup = FTioSupport::instance();
// 	wxString value(val);
	
// 	if (key == "fft_size") {
// 		if (value.ToULong(&ival)) {
// 			engine->setFFTsize ((FTspectralEngine::FFT_Size) ival);
// 		}
// 	}
// 	else if (key == "windowing") {
// 		if (value.ToULong(&ival)) {
// 			engine->setWindowing ((FTspectralEngine::Windowing) ival);
// 		}
// 	}
// 	else if (key == "update_speed") {
// 		if (value.ToULong(&ival)) {
// 			engine->setUpdateSpeed ((FTspectralEngine::UpdateSpeed)(int)ival);
// 		}
// 	}
// 	else if (key == "oversamp") {
// 		if (value.ToULong(&ival)) {
// 			engine->setOversamp ((int)ival);
// 		}
// 	}
// 	else if (key == "input_gain") {
// 		if (value.ToDouble(&fval)) {
// 			engine->setInputGain ((float) fval);
// 		}
// 	}
// 	else if (key == "mix_ratio") {
// 		if (value.ToDouble(&fval)) {
// 			engine->setMixRatio ((float) fval);
// 		}
// 	}
// 	else if (key == "bypassed") {
// 		if (value.ToULong(&ival)) {
// 			engine->setBypassed (ival==1 ? true: false);
// 		}
// 	}
// 	else if (key == "muted") {
// 		if (value.ToULong(&ival)) {
// 			engine->setMuted (ival==1 ? true: false);
// 		}
// 	}
// 	// io stuff
// 	else if (key == "input_ports" && restoreports) {
// 		// value is comma separated list of port names
// 		iosup->disconnectPathInput(id, NULL); // disconnect all

// 		wxString port = value.BeforeFirst(',');
// 		wxString remain = value.AfterFirst(',');
// 		while (!port.IsEmpty()) {
// 			iosup->connectPathInput(id, port.c_str());

// 			port = remain.BeforeFirst(',');
// 			remain = remain.AfterFirst(',');
// 		}
		
// 	}
// 	else if (key == "output_ports"  && restoreports) {
// 		// value is comma separated list of port names
// 		iosup->disconnectPathOutput(id, NULL); // disconnect all

// 		wxString port = value.BeforeFirst(',');
// 		wxString remain = value.AfterFirst(',');
// 		while (!port.IsEmpty()) {
// 			iosup->connectPathOutput(id, port.c_str());

// 			port = remain.BeforeFirst(',');
// 			remain = remain.AfterFirst(',');
// 		}
		
// 	}
// 	else if (key == "tempo") {
// 		if (value.ToULong(&ival)) {
// 			engine->setTempo ((int) ival);
// 		}
// 	}
// 	else if (key == "max_delay") {
// 		if (value.ToDouble(&fval)) {
// 			engine->setMaxDelay ((float) fval);
// 		}
// 	}
// 	else {
// 		vector<FTprocI *> procmods;
// 		engine->getProcessorModules (procmods);

// 		bool done = false;
// 		for (unsigned int n=0; n < procmods.size(); ++n)
// 		{
// 			FTprocI *pm = procmods[n];
// 			vector<FTspectrumModifier *> filts;
// 			pm->getFilters (filts);
			
// 			for (unsigned int m=0; m < filts.size(); ++m)
// 			{
// 				if (key == wxString::Format("%s_bypassed", filts[m]->getConfigName().c_str())) {
// 					if (value.ToULong(&ival)) {
// 						filts[m]->setBypassed (ival==1 ? true: false);
// 					}
// 					done = true;
// 					break;
// 				}
// 				else if ( key == wxString::Format("%s_linked", filts[m]->getConfigName().c_str())) {
// 					if (value.ToULong(&ival)) {
// 						if (ival >= 0 && iosup->getProcessPath(ival))
// 						{
// 							filts[m]->link (iosup->getProcessPath(ival)->getSpectralEngine()->
// 									getProcessorModule(n)->getFilter(m));
// 						}
// 						else {
// 							filts[m]->unlink(false);
// 						}
// 					}
// 					done = true;
// 					break;
// 				}
// 			}
// 			if (done) break;
// 		}

// 	}
	
// // 	else if (key == "freq_bypassed") {
// // 		if (value.ToULong(&ival)) {
// // 			engine->setBypassFreqFilter (ival==1 ? true: false);
// // 		}
// // 	}
// // 	else if (key == "scale_bypassed") {
// // 		if (value.ToULong(&ival)) {
// // 			engine->setBypassScaleFilter (ival==1 ? true: false);
// // 		}
// // 	}
// // 	else if (key == "delay_bypassed") {
// // 		if (value.ToULong(&ival)) {
// // 			engine->setBypassDelayFilter (ival==1 ? true: false);
// // 		}
// // 	}
// // 	else if (key == "gate_bypassed") {
// // 		if (value.ToULong(&ival)) {
// // 			engine->setBypassGateFilter (ival==1 ? true: false);
// // 		}
// // 	}
// // 	else if (key == "inverse_gate_bypassed") {
// // 		if (value.ToULong(&ival)) {
// // 			engine->setBypassInverseGateFilter (ival==1 ? true: false);
// // 		}
// // 	}
// // 	else if (key == "feedback_bypassed") {
// // 		if (value.ToULong(&ival)) {
// // 			engine->setBypassFeedbackFilter (ival==1 ? true: false);
// // 		}
// // 	}

// // 	// link stuff
// // 	else if (key == "freq_linked") {
// // 		if (value.ToULong(&ival)) {
// // 			if (ival >= 0 && iosup->getProcessPath(ival))
// // 			{
// // 				engine->getFreqFilter()->link (iosup->getProcessPath(ival)->getSpectralEngine()->getFreqFilter());
// // 			}
// // 			else {
// // 				engine->getFreqFilter()->unlink(false);
// // 			}
// // 		}
// // 	}
// // 	else if (key == "scale_linked") {
// // 		if (value.ToULong(&ival)) {
// // 			if (ival >= 0 && iosup->getProcessPath(ival))
// // 			{
// // 				engine->getScaleFilter()->link (iosup->getProcessPath(ival)->getSpectralEngine()->getScaleFilter());
// // 			}
// // 			else {
// // 				engine->getScaleFilter()->unlink(false);
// // 			}
// // 		}
// // 	}
// // 	else if (key == "delay_linked") {
// // 		if (value.ToULong(&ival)) {
// // 			if (ival >= 0 && iosup->getProcessPath(ival))
// // 			{
// // 				engine->getDelayFilter()->link (iosup->getProcessPath(ival)->getSpectralEngine()->getDelayFilter());
// // 			}
// // 			else {
// // 				engine->getDelayFilter()->unlink(false);
// // 			}
// // 		}
// // 	}
// // 	else if (key == "gate_linked") {
// // 		if (value.ToULong(&ival)) {
// // 			if (ival >= 0 && iosup->getProcessPath(ival))
// // 			{
// // 				engine->getGateFilter()->link (iosup->getProcessPath(ival)->getSpectralEngine()->getGateFilter());
// // 			}
// // 			else {
// // 				engine->getGateFilter()->unlink(false);
// // 			}
// // 		}
// // 	}
// // 	else if (key == "inverse_gate_linked") {
// // 		if (value.ToULong(&ival)) {
// // 			if (ival >= 0 && iosup->getProcessPath(ival))
// // 			{
// // 				engine->getInverseGateFilter()->link (iosup->getProcessPath(ival)->getSpectralEngine()->getInverseGateFilter());
// // 			}
// // 			else {
// // 				engine->getInverseGateFilter()->unlink(false);
// // 			}
// // 		}
// // 	}
// // 	else if (key == "feedback_linked") {
// // 		if (value.ToULong(&ival)) {
// // 			if (ival >= 0 && iosup->getProcessPath(ival))
// // 			{
// // 				engine->getFeedbackFilter()->link (iosup->getProcessPath(ival)->getSpectralEngine()->getFeedbackFilter());
// // 			}
// // 			else {
// // 				engine->getFeedbackFilter()->unlink(false);
// // 			}
// // 		}
// // 	}

// // 	else {
// // 		printf ("none of the above\n");
// // 	}
// }




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

XMLNode *
FTconfigManager::find_named_node (const XMLNode * node, string name)
{
        XMLNodeList nlist;
        XMLNodeConstIterator niter;
        XMLNode* child;
                                                                                                      
        nlist = node->children();
                                                                                                      
        for (niter = nlist.begin(); niter != nlist.end(); ++niter) {
                                                                                                      
                child = *niter;
                                                                                                      
                if (child->name() == name) {
                        return child;
                }
        }
                                                                                                      
        return 0;
}
