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
#include <wx/filename.h>

#include "FTconfigManager.hpp"
#include "FTspectrumModifier.hpp"
#include "FTspectralEngine.hpp"
#include "FTioSupport.hpp"
#include "FTprocessPath.hpp"
#include "FTdspManager.hpp"
#include "version.h"

#include "xml++.hpp"

FTconfigManager::FTconfigManager(const std::string & basedir)
        : _basedir (basedir)
{
	if (_basedir.empty()) {
		char * homestr = getenv("HOME");

		if (homestr) {
		        _basedir = static_cast<const char *> (wxString::Format(wxT("%s%c.freqtweak"), homestr, wxFileName::GetPathSeparator()).fn_str());
		}
		else {
			_basedir = ".freqtweak";
		}
	}

	// try to create basedir if it doesn't exist
        //wxDir bdir(_basedir);
	
	if ( ! wxDir::Exists(wxString::FromAscii (_basedir.c_str())) ) {
		if (mkdir ( _basedir.c_str(), 0755 )) {
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
	if ( ! wxDir::Exists(wxString::FromAscii (_basedir.c_str()) + wxFileName::GetPathSeparator() + wxT("presets")) ) {
		std::string predir (wxString::Format(wxT("%s%cpresets"), _basedir.c_str(), wxFileName::GetPathSeparator()).fn_str());
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


bool FTconfigManager::storeSettings (const std::string &name, bool uselast)
{
	if (!uselast && (name != "")) {
		return false;
	}

	wxString dirname;

	// directory to store settings
	if (uselast)
	{
		dirname = wxString::Format(wxT("%s%clast_setting"), _basedir.c_str(), wxFileName::GetPathSeparator());
	}
	else
	{
		dirname = wxString::Format(wxT("%s%cpresets%c%s"), _basedir.c_str(), wxFileName::GetPathSeparator(),
					   wxFileName::GetPathSeparator(), name.c_str());
	}
	
	if ( ! wxDir::Exists(dirname) ) {
		if (mkdir ( dirname.fn_str(), 0755 )) {
			printf ("Error creating %s\n", static_cast<const char *> (dirname.mb_str())); 
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
		unlink (wxString::Format(wxT("%s/%s"), static_cast<const char *> (dirname.fn_str()),
					 static_cast<const char *> (filename.fn_str())).fn_str() );
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
			paramsNode->add_property ("fft_size", static_cast<const char *> (wxString::Format(wxT("%d"), engine->getFFTsize()).mb_str()));
			paramsNode->add_property ("windowing", static_cast<const char *> (wxString::Format(wxT("%d"), engine->getWindowing()).mb_str()));			
			paramsNode->add_property ("update_speed", static_cast<const char *> (wxString::Format(wxT("%d"), engine->getUpdateSpeed()).mb_str()));			
			paramsNode->add_property ("oversamp", static_cast<const char *> (wxString::Format(wxT("%d"), engine->getOversamp()).mb_str()));			
			paramsNode->add_property ("tempo", static_cast<const char *> (wxString::Format(wxT("%d"), engine->getTempo()).mb_str()));			
			paramsNode->add_property ("max_delay", static_cast<const char *> (wxString::Format(wxT("%.10g"), engine->getMaxDelay()).mb_str()));			
		}


		vector<FTprocI *> procmods;
		engine->getProcessorModules (procmods);

		XMLNode * chanNode = channelsNode->add_child ("Channel");

		chanNode->add_property ("pos", static_cast<const char *> (wxString::Format(wxT("%d"), i).mb_str()));
		chanNode->add_property ("input_gain", static_cast<const char *> (wxString::Format(wxT("%.10g"), engine->getInputGain()).mb_str()));
		chanNode->add_property ("mix_ratio", static_cast<const char *> (wxString::Format(wxT("%.10g"), engine->getMixRatio()).mb_str()));
		chanNode->add_property ("bypassed", static_cast<const char *> (wxString::Format(wxT("%d"), engine->getBypassed() ? 1: 0).mb_str()));
		chanNode->add_property ("muted", static_cast<const char *> (wxString::Format(wxT("%d"), engine->getMuted() ? 1 : 0).mb_str()));
		
		
		// now for the filter sections

		XMLNode * procmodsNode = chanNode->add_child ("ProcMods");
		
		for (unsigned int n=0; n < procmods.size(); ++n)
		{
			FTprocI *pm = procmods[n];
			vector<FTspectrumModifier *> filts;
			pm->getFilters (filts);

			XMLNode * pmNode = procmodsNode->add_child ("ProcMod");
			pmNode->add_property ("pos", static_cast<const char *> (wxString::Format(wxT("%d"), n).mb_str()));
			pmNode->add_property ("name", pm->getConfName());
			
			for (unsigned int m=0; m < filts.size(); ++m)
			{
				XMLNode * filtNode = pmNode->add_child ("Filter");

				filtNode->add_property ("pos", static_cast<const char *> (wxString::Format(wxT("%d"), m).mb_str()));
				filtNode->add_property ("name", filts[m]->getConfigName().c_str());
				filtNode->add_property ("linked", static_cast<const char *> (
				        wxString::Format(wxT("%d"),
							 filts[m]->getLink() ?
							 filts[m]->getLink()->getId() : -1).mb_str()));
					
				filtNode->add_property ("bypassed", static_cast<const char *> (
                                        wxString::Format(wxT("%d"),
							 filts[m]->getBypassed() ? 1 : 0).mb_str()));

				std::string filtfname (wxString::Format(wxT("%d_%d_%s.filter"), i, n, filts[m]->getConfigName().c_str()).fn_str());
				filtNode->add_property ("file", filtfname);

				// write out filter file
				wxTextFile filtfile (wxString::Format(wxT("%s%c%s"),
								      static_cast<const char *> (dirname.fn_str()),
								      wxFileName::GetPathSeparator(),
								      filtfname.c_str()));
				if (filtfile.Exists()) {
					// remove it
					unlink (wxString (filtfile.GetName()).fn_str ());
				}
				filtfile.Create ();
				writeFilter (filts[m], filtfile);
				filtfile.Write();
				filtfile.Close();

				// write Extra node
				XMLNode * extran = filts[m]->getExtraNode();
				filtNode->add_child_copy (*extran);

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
	
	if (configdoc.write (static_cast<const char *> (
                wxString::Format(wxT("%s%c%s"), static_cast<const char *> (dirname.fn_str()),
				 wxFileName::GetPathSeparator(), "config.xml").fn_str())))
	{	    
		fprintf (stderr, "Stored settings into %s\n", static_cast<const char *> (dirname.fn_str()));
		return true;
	}
	else {
		fprintf (stderr, "Failed to store settings into %s\n", static_cast<const char *> (dirname.fn_str()));
		return false;
	}
}

bool FTconfigManager::loadSettings (const std::string &name, bool restore_ports, bool uselast)
{
	vector<vector <FTprocI *> > tmpvec;
	return loadSettings(name, restore_ports, false, tmpvec, uselast);
}

bool FTconfigManager::loadSettings (const std::string &name, bool restore_ports, bool ignore_iosup, vector< vector<FTprocI *> > & procvec, bool uselast)
{
	if (!uselast && (name != "")) {
		return false;
	}

	wxString dirname;
	if (uselast) {
		dirname = wxString::Format(wxT("%s%clast_setting"), _basedir.c_str(), wxFileName::GetPathSeparator());
	}
	else {
		dirname = wxString::Format(wxT("%s%cpresets/%s"), _basedir.c_str(), wxFileName::GetPathSeparator(), name.c_str());
	}
	
	if ( ! wxDir::Exists(dirname) ) {
		printf ("Settings %s does not exist!\n", static_cast<const char *> (dirname.fn_str())); 
		return false;
	}

	FTioSupport * iosup = 0;

	if (!ignore_iosup) {
		iosup = FTioSupport::instance();
	}
	
	// open file
	string configfname(static_cast<const char *> (wxString::Format(wxT("%s%c%s"),
								       static_cast<const char *> (dirname.fn_str()),
								       wxFileName::GetPathSeparator(),
								       "config.xml").fn_str()));
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

	if (!ignore_iosup)
	{
		unsigned int i;
		for (i=0; i < chanlist.size() && i < FT_MAXPATHS; i++)
		{
			iosup->setProcessPathActive(i, true);
		}
		// set all remaining paths inactive
		for ( ; i < FT_MAXPATHS; i++) {
			iosup->setProcessPathActive(i, false);
		}
	}
	else {
		// set up procvec with its channels
		for (unsigned int i=0; i < chanlist.size() && i < FT_MAXPATHS; i++)
		{
			procvec.push_back(vector<FTprocI *>());
		}
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
			wxString value (wxString::FromAscii ((*propiter)->value().c_str()));

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
		wxString tmpstr (wxString::FromAscii (prop->value().c_str()));
		if (!tmpstr.ToULong (&chan_pos) || chan_pos >= FT_MAXPATHS) {
			fprintf (stderr, "invalid pos in channel!\n"); 
			continue;
		}

		FTspectralEngine * engine = 0;

		if (!ignore_iosup) {
			FTprocessPath * procpath = iosup->getProcessPath((int) chan_pos);
			if (!procpath) continue; // shouldnt happen
			
			engine = procpath->getSpectralEngine();

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
				wxString value (wxString::FromAscii ((*propiter)->value().c_str()));

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
		}
		
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

			if (pmNode->name() != "ProcMod") continue;
			
			if (!(prop = pmNode->property ("pos"))) {
				fprintf (stderr, "pos missing in procmod!\n"); 
				continue;
			}
			unsigned long ppos;
			tmpstr = wxString::FromAscii (prop->value().c_str());
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
			FTprocI * procmod = FTdspManager::instance()->getModuleByConfigName(pmname);
			if (!procmod) {
				fprintf (stderr, "no proc module '%s' supported\n", pmname.c_str()); 
				continue;
			}
			procmod = procmod->clone();

			// must call this before initialization
			procmod->setMaxDelay ((float)max_delay);

			procmod->initialize();

			if (!ignore_iosup) {
				procmod->setSampleRate (iosup->getSampleRate());
			}
			
			procmod->setOversamp ((int)oversamp);

			// load up the filters in the procmod
			
			XMLNodeList filtlist = pmNode->children();
			XMLNodeConstIterator filtiter;
			XMLNode * filtNode;

			for (filtiter=filtlist.begin(); filtiter != filtlist.end(); ++filtiter)
			{
				filtNode = *filtiter;

			        if (filtNode->name() != "Filter")
				{
					continue;
				}
				
				if (!(prop =filtNode->property ("pos"))) {
					fprintf (stderr, "pos missing in filter!\n"); 
					continue;
				}
				unsigned long fpos;
				tmpstr = wxString::FromAscii (prop->value().c_str());
				if (!tmpstr.ToULong (&fpos)) {
					fprintf (stderr, "invalid filter pos in channel!\n"); 
					continue;
				}

				if (!(prop = filtNode->property ("file"))) {
					fprintf (stderr, "filter filename missing in procmod!\n"); 
					continue;
				}
				std::string filtfname = prop->value();

				FTspectrumModifier * specmod = procmod->getFilter (fpos);
				if (!specmod) {
					fprintf (stderr, "no filter at index %lu in procmod!\n", fpos); 
					continue;
				}

				// load filter
				wxTextFile filtfile (wxString::Format(wxT("%s%c%d_%d_%s.filter"),
								      static_cast<const char *> (dirname.fn_str()),
								      wxFileName::GetPathSeparator(),
								      (int) chan_pos, (int) ppos,
								      specmod->getConfigName().c_str()));
				if (filtfile.Open()) {

					loadFilter (specmod, filtfile);
					filtfile.Close();
				}

				// set bypassed
				if ((prop = filtNode->property ("bypassed"))) {
					wxString value (wxString::FromAscii (prop->value().c_str()));
					if (value.ToULong(&uval)) {
						specmod->setBypassed (uval==1 ? true: false);
					}
				}

				// actual linkage must wait for later
				long linked = -1;
				if ((prop = filtNode->property ("linked"))) {
					wxString value (wxString::FromAscii (prop->value().c_str()));
					if (value.ToLong(&linked) && linked >= 0) {
						
						_linkCache.push_back (LinkCache(chan_pos, linked, ppos, fpos));
					}
					else {
						specmod->unlink(false);
					}
				}

				// extra node
				XMLNode * extraNode = find_named_node (filtNode, "Extra");
				
				if (extraNode) {
					specmod->setExtraNode (extraNode);
				}
			}

			// insert procmod

			if (!ignore_iosup)
			{
				engine->insertProcessorModule (procmod, ppos);
			}
			else {
				// add to vector in the right spot
				vector<FTprocI*>::iterator iter = procvec[chan_pos].begin();
				
				for (unsigned int n=0; n < ppos && iter!=procvec[chan_pos].end(); ++n) {
					++iter;
				}
				
				procvec[chan_pos].insert (iter, procmod);
			}
		}


		if (ignore_iosup) {
			// can skip to the next one
			continue;
		}
		
		// apply global settings
		engine->setFFTsize ((FTspectralEngine::FFT_Size) fft_size);
		engine->setWindowing ((FTspectralEngine::Windowing) windowing);
		engine->setUpdateSpeed ((FTspectralEngine::UpdateSpeed)(int) update_speed);
		

		// input ports

		if (restore_ports)
		{
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
		
	}

	if (!ignore_iosup)
	{
		// now we can apply linkages
		list<LinkCache>::iterator liter;
		for (liter = _linkCache.begin(); liter != _linkCache.end(); ++liter)
		{
			LinkCache & lc = *liter;

			FTspectrumModifier *source = iosup->getProcessPath(lc.source_chan)->getSpectralEngine()
				->getProcessorModule(lc.mod_n)->getFilter(lc.filt_n);

			FTspectrumModifier *dest = iosup->getProcessPath(lc.dest_chan)->getSpectralEngine()
				->getProcessorModule(lc.mod_n)->getFilter(lc.filt_n);

			if (dest && source) {
				source->link (dest);
			}
			else {
				fprintf(stderr, "could not link! source or dest does not exist!\n");
			}
		
		}
	
	}
	else
	{
		// just use the stored ones
		list<LinkCache>::iterator liter;
		for (liter = _linkCache.begin(); liter != _linkCache.end(); ++liter)
		{
			LinkCache & lc = *liter;

			FTspectrumModifier *source = procvec[lc.source_chan][lc.mod_n]->getFilter(lc.filt_n);
			FTspectrumModifier *dest = procvec[lc.dest_chan][lc.mod_n]->getFilter(lc.filt_n);

			source->link (dest);
		}
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
		
		int pos = line.find_first_of (wxT(" \t"));
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
			tf.AddLine ( wxString::Format (wxT("%.20g"), lastval));
			pos = i;
			lastval = values[i];
		}
		else {
			// write range
			tf.AddLine ( wxString::Format (wxT("%d:%d  %.20g"), pos, i-1, values[pos]));
			pos = i;
			lastval = values[i];
		}

	}

	// write last
	if (pos < totbins) {
		tf.AddLine ( wxString::Format (wxT("%d:%d  %.20g"), pos, totbins-1, values[pos]));
	}
}



list<string> FTconfigManager::getSettingsNames()
{
	wxString dirname(wxString::Format(wxT("%s%cpresets"), _basedir.c_str(), wxFileName::GetPathSeparator()));

	list<string> flist;
	
	wxDir dir(dirname);
	if ( !dir.IsOpened() ) {
		return flist;
	}

	wxString filename;

	
	bool cont = dir.GetFirst(&filename, wxT(""), wxDIR_DIRS);

	while ( cont )
	{
		//printf ("%s\n", filename.c_str());
		flist.push_back (static_cast<const char *> (filename.fn_str()));
		cont = dir.GetNext(&filename);
	}

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
