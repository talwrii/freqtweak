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
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include "FTjackSupport.hpp"
#include "FTprocessPath.hpp"
#include "FTspectralManip.hpp"
#include "FTtypes.hpp"

#include <jack/jack.h>


FTjackSupport::FTjackSupport(const char *name)
	: _inited(false), _activePathCount(0)
{
	// init process path info
	for (int i=0; i < FT_MAXPATHS; i++) {
		_pathInfos[i] = 0;
	}

	if (name) {
		snprintf(_name, sizeof(_name)-1, "%s", name);
	}
	else {
		strncat(_name, "", sizeof(_name)-1);
	}
}

FTjackSupport::~FTjackSupport()
{
	// init process path info
	for (int i=0; i < FT_MAXPATHS; i++) {
		if (_pathInfos[i]) {
			delete _pathInfos[i];
		}
	}

	if (_inited) {
		jack_client_close ( _jackClient );
	}
}

/**
 *  Initialize and connect to jack server.
 *  Returns false if failed
 */
bool FTjackSupport::init()
{
	/* try to become a client of the JACK server */
	int pid = getpid();

	if (strcmp(_name, "") == 0) {
		snprintf(_name, sizeof(_name)-1, "freqtweak-%d", pid);
	}
	
	if ((_jackClient = jack_client_new (_name)) == 0) {
		fprintf (stderr, "JACK Error: Either invalid name or JACK server not running?\n");
		return false;
	}

	/* tell the JACK server to call `process()' whenever
	   there is work to be done.
	*/

	jack_set_process_callback (_jackClient, FTjackSupport::processCallback, 0);

	/* tell the JACK server to call `bufsize()' whenever
	   the maximum number of frames that will be passed
	   to `process()' changes
	*/

	jack_set_buffer_size_callback (_jackClient, FTjackSupport::bufsizeCallback, 0);

	/* tell the JACK server to call `srate()' whenever
	   the sample rate of the system changes.
	*/


	jack_set_sample_rate_callback (_jackClient, FTjackSupport::srateCallback, 0);

	/* tell the JACK server to call `jack_shutdown()' if
	   it ever shuts down, either entirely, or if it
	   just decides to stop calling us.
	*/

	jack_on_shutdown (_jackClient, FTjackSupport::jackShutdown, 0);

	/* display the current sample rate. once the client is activated 
	   (see below), you should rely on your own sample rate
	   callback (see above) for this value.
	*/

	_sampleRate = jack_get_sample_rate (_jackClient);
	//printf ("engine sample rate: %lu\n", _sampleRate);

	_inited = true;
	return true;
}

bool FTjackSupport::startProcessing()
{
	if (jack_activate (_jackClient)) {
		fprintf (stderr, "Error: cannot activate jack client!\n");
		return false;
	}

	return true;
}

bool FTjackSupport::stopProcessing()
{
	if (jack_deactivate (_jackClient)) {
		fprintf (stderr, "Error: cannot deactivate jack client!\n");
		return false;
	}

	return true;
}

bool FTjackSupport::close()
{
	if (_inited) {
		stopProcessing();
		jack_client_close ( _jackClient );
		return true;
	}
	return false;
}


FTprocessPath * FTjackSupport::setProcessPathActive (int index, bool active)
{
	if (!_inited) return 0;

	char nbuf[30];

	PathInfo *tmppath;
	FTprocessPath * ppath;
	
	if (index >=0 && index < FT_MAXPATHS) {
		if (_pathInfos[index]) {
			tmppath = _pathInfos[index];
			ppath = tmppath->procpath;

			if (active) {
				if (tmppath->active) {
					//already active, do nothing
					return ppath;
				}
			}
			else {
				if (tmppath->active)  {
					//  detach ports
					jack_port_unregister (_jackClient, tmppath->inputport);
					jack_port_unregister (_jackClient, tmppath->outputport);
				
					// just mark it inactive but not destroy it
					tmppath->active = false;
					_activePathCount--;
					//we are not deleting old one, we reuse it later if necessary
				}
				return 0;
			}
		}
		else {
			// it is a new one, construct new processPath
			if (active) {
				tmppath = new PathInfo();
				ppath = new FTprocessPath();
			}
			else {
				return 0;
			}
		}

		// it only gets here if it is brand new, or going from inactive->active
		
		sprintf(nbuf,"in_%d", index + 1);
		tmppath->inputport = jack_port_register (_jackClient, nbuf, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);

		
		sprintf(nbuf,"out_%d", index + 1);
		tmppath->outputport = jack_port_register (_jackClient, nbuf, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
		jack_port_set_latency (tmppath->outputport, ppath->getSpectralManip()->getLatency());

		tmppath->procpath = ppath;
		tmppath->active = true;
		
		_pathInfos[index] = tmppath;

		ppath->setId (index);
		_activePathCount++;
		
		return ppath;
	}
	
	return false;
}


const char * FTjackSupport::getInputPortName(int index)
{
	if (index >=0 && index < FT_MAXPATHS) {
		if (_pathInfos[index]) {
			return jack_port_name(_pathInfos[index]->inputport);
		}
	}

	return NULL;
}

const char * FTjackSupport::getOutputPortName(int index)
{
	if (index >=0 && index < FT_MAXPATHS) {
		if (_pathInfos[index]) {
			return jack_port_name(_pathInfos[index]->outputport);
		}
	}

	return NULL;
}


bool FTjackSupport::connectPathInput (int index, const char *inname)
{
	if (index >=0 && index < FT_MAXPATHS && _pathInfos[index])
	{
		if (jack_connect (_jackClient, inname, jack_port_name(_pathInfos[index]->inputport))) {
			fprintf (stderr, "JACK error: cannot connect input port: %s -> %s\n", inname,
				 jack_port_name(_pathInfos[index]->inputport));
			return false;
		}

		return true;
	}

	return false;
}

bool FTjackSupport::connectPathOutput (int index, const char *outname)
{
	if (index >=0 && index < FT_MAXPATHS && _pathInfos[index])
	{
		if (jack_connect (_jackClient, jack_port_name(_pathInfos[index]->outputport), outname)) {
			fprintf (stderr, "JACK error: cannot connect output port: %s -> %s\n",
				 jack_port_name(_pathInfos[index]->outputport), outname);
			return false;
		}
	}

	return false;
}


bool FTjackSupport::disconnectPathInput (int index, const char *inname)
{
	if (index >=0 && index < FT_MAXPATHS && _pathInfos[index])
	{
		if (inname)
		{
			if (jack_disconnect (_jackClient, inname, jack_port_name(_pathInfos[index]->inputport))) {
				fprintf (stderr, "cannot disconnect input port\n");
				return false;
			}
			return true;
		}
		else {
			// disconnect all from our input port
			const char ** portnames = jack_port_get_connections (_pathInfos[index]->inputport);
			if (portnames) {
				for (int i=0; portnames[i]; i++) {
					jack_disconnect (_jackClient, portnames[i], jack_port_name(_pathInfos[index]->inputport));
				}
				free(portnames);
			}
			return true;
		}
	}

	return false;
}

bool FTjackSupport::disconnectPathOutput (int index, const char *outname)
{
	
	if (index >=0 && index < FT_MAXPATHS && _pathInfos[index])
	{
		if (outname)
		{
			if (jack_disconnect (_jackClient, jack_port_name(_pathInfos[index]->outputport), outname)) {
				fprintf (stderr, "cannot disconnect output ports\n");
				return false;
			}
			return true;
		}
		else {
			// disconnect all from our output port
			const char ** portnames = jack_port_get_connections (_pathInfos[index]->outputport);
			if (portnames) {
				for (int i=0; portnames[i]; i++) {
					jack_disconnect (_jackClient, jack_port_name(_pathInfos[index]->outputport), portnames[i]);
				}
				free(portnames);
			}
			return true;
		}

	}

	return false;
}


const char ** FTjackSupport::getInputConnectablePorts (int index)
{
	const char ** portnames = NULL;

	if (index >=0 && index < FT_MAXPATHS && _pathInfos[index])
	{
		//char regexstr[100];
		// anything but our own output port
		//snprintf(regexstr, 99, "^(%s)", jack_port_name (_pathInfos[index]->outputport) );
		
		portnames = jack_get_ports( _jackClient, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
	}

	return portnames;
}

const char ** FTjackSupport::getOutputConnectablePorts (int index)
{
	const char ** portnames = NULL;

	if (index >=0 && index < FT_MAXPATHS && _pathInfos[index])
	{
		//char regexstr[100];
		// anything but our own input port
		//snprintf(regexstr, 99, "^(%s)", jack_port_name (_pathInfos[index]->inputport) );
		
		portnames = jack_get_ports( _jackClient, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);
	}

	return portnames;
}


const char ** FTjackSupport::getConnectedInputPorts(int index)
{
	const char ** portnames = NULL;

	if (index >=0 && index < FT_MAXPATHS && _pathInfos[index])
	{
		//char regexstr[100];
		// anything but our own input port
		//snprintf(regexstr, 99, "^(%s)", jack_port_name (_pathInfos[index]->inputport) );
		
		portnames = jack_port_get_connections( _pathInfos[index]->inputport);
	}

	return portnames;
}

const char ** FTjackSupport::getConnectedOutputPorts(int index)
{
	const char ** portnames = NULL;

	if (index >=0 && index < FT_MAXPATHS && _pathInfos[index])
	{
		//char regexstr[100];
		// anything but our own input port
		//snprintf(regexstr, 99, "^(%s)", jack_port_name (_pathInfos[index]->inputport) );
		
		portnames = jack_port_get_connections( _pathInfos[index]->outputport);
	}

	return portnames;
}




/** static callbacks **/

int FTjackSupport::processCallback (jack_nframes_t nframes, void *arg)
{
	FTjackSupport * jsup = (FTjackSupport *) FTioSupport::instance();
	PathInfo * tmppath;
	
	// do processing for each path
	for (int i=0; i < FT_MAXPATHS; i++)
	{
		if (jsup->_pathInfos[i] && jsup->_pathInfos[i]->active) {
			tmppath = jsup->_pathInfos[i];
			
			sample_t *in = (sample_t *) jack_port_get_buffer (tmppath->inputport, nframes);
			sample_t *out = (sample_t *) jack_port_get_buffer (tmppath->outputport, nframes);
			tmppath->procpath->processData(in, out, nframes);
			
		}
	}
	
	return 0;	
}

int FTjackSupport::bufsizeCallback (jack_nframes_t nframes, void *arg)
{
	FTjackSupport * jsup = (FTjackSupport *) FTioSupport::instance();

	for (int i=0; i < FT_MAXPATHS; i++)
	{
		if (jsup->_pathInfos[i]) {
			jsup->_pathInfos[i]->procpath->setMaxBufsize(nframes);
		}
	}
	
	return 0;
}

int FTjackSupport::srateCallback (jack_nframes_t nframes, void *arg)
{
	FTjackSupport * jsup = (FTjackSupport *) FTioSupport::instance();

	for (int i=0; i < FT_MAXPATHS; i++)
	{
		if (jsup->_pathInfos[i]) {
			jsup->_pathInfos[i]->procpath->setSampleRate(nframes);
		}
	}
	
	return 0;
}

void FTjackSupport::jackShutdown (void *arg)
{
	FTjackSupport * jsup = (FTjackSupport *) FTioSupport::instance();

	fprintf (stderr, "Jack shut us down!\n");

	for (int i=0; i < FT_MAXPATHS; i++)
	{
		if (jsup->_pathInfos[i]) {
		   jsup->_pathInfos[i]->active = false;
		}
	}
	
	jsup->_inited = false;
	jsup->_activePathCount = 0;

	// reconnect?
}

// This isn't in use yet.
int FTjackSupport::portsChanged (jack_port_id_t port, int blah, void *arg)
{
	FTjackSupport * jsup = (FTjackSupport *) FTioSupport::instance();

	fprintf (stderr, "Ports changed on us!\n");
 
	jsup->_portsChanged = true;

	return 0;
}
