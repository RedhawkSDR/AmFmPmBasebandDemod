/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file distributed with this
 * source distribution.
 *
 * This file is part of REDHAWK Basic Components AmFmPmBasebandDemod.
 *
 * REDHAWK Basic Components AmFmPmBasebandDemod is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Lesser General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * REDHAWK Basic Components AmFmPmBasebandDemod is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with this
 * program.  If not, see http://www.gnu.org/licenses/.
 */

/**************************************************************************

    This is the component code. This file contains the child class where
    custom functionality can be added to the component. Custom
    functionality to the base class can be extended here. Access to
    the ports can also be done from this class

 **************************************************************************/

#include "AmFmPmBasebandDemod.h"

PREPARE_LOGGING(AmFmPmBasebandDemod_i)

AmFmPmProcessor::AmFmPmProcessor() :
	inputIndex(0),
	demod(NULL),
	demodInput(Complex(0.0,0.0),BUFFER_LENGTH)
{
}

AmFmPmProcessor::~AmFmPmProcessor()
{
	if (demod!=NULL)
		delete demod;
}

void AmFmPmProcessor::newDemod() {
	float freqGain;
	if (freqDev <= 0)
		freqGain = 1.0 / sampleRate;
	else
		freqGain = freqDev;
	float inialPhase = 0;
	if (demod != NULL) {
		inialPhase = demod->getPhase();
		delete demod;
	}
	demod = new AmFmPmBasebandDemod(demodInput, amBuf, pmBuf, fmBuf, freqGain,
			phaseDev, inialPhase);
}

void AmFmPmProcessor::setup(AmFmPmBasebandDemod_i* component, double freqDevaition, double phaseDeviation, RealArray* fmOutput, RealArray* pmOutput, RealArray* amOutput)
{
	parent = component;
	freqDev=freqDevaition;
	phaseDev = phaseDeviation;
	amBuf = amOutput;
	pmBuf = pmOutput;
	fmBuf = fmOutput;
	newDemod();
}

void AmFmPmProcessor::process (std::vector<float>& input)
{
	//process some data
	for(size_t i=0; i< input.size(); i+=2) {
		//convert to the tunerInput complex data type
		demodInput[inputIndex++] = Complex(input[i],input[i+1]);
		if (inputIndex==BUFFER_LENGTH) {
			inputIndex=0;
			if (demod->process())
				parent->doOutput();
		}
	}
}
void AmFmPmProcessor::updateSampleRate(double fs)
{
	sampleRate = fs;
	if (freqDev<=0)
		newDemod();
}

AmFmPmBasebandDemod_i::AmFmPmBasebandDemod_i(const char *uuid, const char *label) :
    		AmFmPmBasebandDemod_base(uuid, label),
    		fmOutput(0.0,BUFFER_LENGTH),
    		pmOutput(0.0,BUFFER_LENGTH),
    		amOutput(0.0,BUFFER_LENGTH),
    		amBuf(NULL),
    		pmBuf(NULL),
    		fmBuf(NULL),
    		listener(*this, &AmFmPmBasebandDemod_i::callBackFunc)
{
	LOG_DEBUG(AmFmPmBasebandDemod_i, "AmFmPmBasebandDemod_i::AmFmPmBasebandDemod_i() constructor entry");
	outputBuffer.reserve(BUFFER_LENGTH);
	//initialize processing classes and private variables
	squelchThreshold = 0;
	dataFloat_In->setMaxQueueDepth(1000);

	am_dataFloat_out->setNewConnectListener(&listener);
	am_dataFloat_out->setNewDisconnectListener(&listener);
	pm_dataFloat_out->setNewConnectListener(&listener);
	pm_dataFloat_out->setNewDisconnectListener(&listener);
	fm_dataFloat_out->setNewConnectListener(&listener);
	fm_dataFloat_out->setNewDisconnectListener(&listener);


}

void AmFmPmBasebandDemod_i::configure(const CF::Properties & props) throw (CORBA::SystemException, CF::PropertySet::InvalidConfiguration, CF::PropertySet::PartialConfiguration)
		{
	LOG_DEBUG(AmFmPmBasebandDemod_i, "configure() entry");
	AmFmPmBasebandDemod_base::configure(props);
	for (CORBA::ULong i=0; i < props.length(); ++i) {
		const std::string id = (const char*) props[i].id;
		PropertyInterface* property = getPropertyFromId(id);
		if (property->id == "squelch") {
			squelchThreshold = std::pow(10.0,squelch / 10);
		}
		else if (property->id == "freqDeviation" || property->id == "phaseDeviation") {
			remakeDemods();
		}
	}

	LOG_DEBUG(AmFmPmBasebandDemod_i, "AmFmPmBasebandDemod_i::configure() - freqDeviation    = " << freqDeviation);
	LOG_DEBUG(AmFmPmBasebandDemod_i, "AmFmPmBasebandDemod_i::configure() - phaseDeviation   = " << phaseDeviation);
	LOG_DEBUG(AmFmPmBasebandDemod_i, "AmFmPmBasebandDemod_i::configure() - squelch          = " << squelch);
	LOG_DEBUG(AmFmPmBasebandDemod_i, "AmFmPmBasebandDemod_i::configure() - squelchThreshold = " << squelchThreshold);
}


int AmFmPmBasebandDemod_i::serviceFunction()
{
	LOG_TRACE(AmFmPmBasebandDemod_i, "serviceFunction()");
	pkt = dataFloat_In->getPacket(0.0);
	if (pkt==NULL)
		return NOOP;

	if(pkt->inputQueueFlushed)
	{
		LOG_WARN(AmFmPmBasebandDemod_i, "Input queue is flushing");
		demods.clear();
	}
	{
		bool updateSRI = pkt->sriChanged;
		boost::mutex::scoped_lock lock(demodLock);
		map_type::iterator i = demods.find(pkt->streamID);
		bool updatedSampleRated = false;
		if (i == demods.end())
		{
			LOG_DEBUG(AmFmPmBasebandDemod_i, "AmFmPmBasebandDemod_i::AmFmPmBasebandDemod_i() got a new stream:  " << pkt->streamID);
			updateSRI = true;
			map_type::value_type processor(pkt->streamID, AmFmPmProcessor());
			i = demods.insert(demods.end(),processor);
			updatedSampleRated = true;
			i->second.updateSampleRate(1.0/pkt->SRI.xdelta);
			remakeDemod(&i->second);
		}
		//Check if SRI has been changed
		if (updateSRI) {
			LOG_DEBUG(AmFmPmBasebandDemod_i, "@ serviceFunction() - sriChanges ");
			if (!updatedSampleRated) {
				i->second.updateSampleRate(1.0/pkt->SRI.xdelta);
			}
			if (pkt->SRI.mode != 1) {
				LOG_DEBUG(AmFmPmBasebandDemod_i, "WARNING --  mode is not 1 -- treating real data as if complex");
			}
			//data real even though input was complex
			pkt->SRI.mode=0;
			am_dataFloat_out->pushSRI(pkt->SRI);
			fm_dataFloat_out->pushSRI(pkt->SRI);
			pm_dataFloat_out->pushSRI(pkt->SRI);
		}
		i->second.process(pkt->dataBuffer);

		if (pkt->EOS)
		{
			LOG_DEBUG(AmFmPmBasebandDemod_i, "AmFmPmBasebandDemod_i::AmFmPmBasebandDemod_i() got an EOS");
			demods.erase(i);
		}
	}

	delete pkt; //must delete the dataTransfer object when no longer needed
	LOG_TRACE(AmFmPmBasebandDemod_i, "AmFmPmBasebandDemod_i::AmFmPmBasebandDemod_i() service function done");
	return NORMAL;
}

void AmFmPmBasebandDemod_i::doOutput()
{
	if(amBuf!=NULL)
	{
		//Check Squelch and output
		for (size_t j=0; j<amOutput.size(); j++) {
			if(amOutput[j]*amOutput[j] <= squelchThreshold)
				amOutput[j]=0.0;
			outputBuffer.push_back(amOutput[j]);
		}
		LOG_TRACE(AmFmPmBasebandDemod_i, "AmFmPmBasebandDemod_i::AmFmPmBasebandDemod_i() pushing AM"  << outputBuffer.size());
		am_dataFloat_out->pushPacket(outputBuffer, pkt->T, pkt->EOS, pkt->streamID);
		outputBuffer.clear();
	}
	if (fmBuf!=NULL)
	{
		for (size_t j=0; j<fmOutput.size(); j++) {
			if(fmOutput[j]*fmOutput[j] <= squelchThreshold)
				fmOutput[j]=0.0;
			outputBuffer.push_back(fmOutput[j]);
		}
		LOG_TRACE(AmFmPmBasebandDemod_i, "AmFmPmBasebandDemod_i::AmFmPmBasebandDemod_i() pushing FM" << outputBuffer.size());
		fm_dataFloat_out->pushPacket(outputBuffer, pkt->T, pkt->EOS, pkt->streamID);
		outputBuffer.clear();
	}
	if (pmBuf!=NULL)
	{
		for (size_t j=0; j<pmOutput.size(); j++) {
			if(pmOutput[j]*pmOutput[j] <= squelchThreshold)
				pmOutput[j]=0.0;
			outputBuffer.push_back(pmOutput[j]);
		}
		LOG_TRACE(AmFmPmBasebandDemod_i, "AmFmPmBasebandDemod_i::AmFmPmBasebandDemod_i() pushing PM" << outputBuffer.size());
		pm_dataFloat_out->pushPacket(outputBuffer, pkt->T, pkt->EOS, pkt->streamID);
		outputBuffer.clear();
	}

}

void AmFmPmBasebandDemod_i::remakeDemods()
{
	for (map_type::iterator i = demods.begin(); i!=demods.end(); i++)
		remakeDemod(&i->second);
}

void AmFmPmBasebandDemod_i::remakeDemod(AmFmPmProcessor* processor)
{
	LOG_DEBUG(AmFmPmBasebandDemod_i, "AmFmPmBasebandDemod::remakeDemod() entry, freqDeviation = " << freqDeviation << ", phaseDeviation = " << phaseDeviation);
	processor->setup(this, freqDeviation, phaseDeviation, fmBuf, pmBuf, amBuf);
}


void AmFmPmBasebandDemod_i::callBackFunc( const char* connectionId)
{
	LOG_DEBUG(AmFmPmBasebandDemod_i, "got connection " << connectionId);
	bool newDemod;
	bool amConnected(am_dataFloat_out->state()!=BULKIO::IDLE);
	bool pmConnected(pm_dataFloat_out->state()!=BULKIO::IDLE);
	bool fmConnected(fm_dataFloat_out->state()!=BULKIO::IDLE);
	bool amWasConnected(amBuf != NULL);
	bool pmWasConnected(pmBuf != NULL);
	bool fmWasConnected(fmBuf != NULL);
	if (amWasConnected != amConnected)
	{
		LOG_DEBUG(AmFmPmBasebandDemod_i, "update am connection ");
		if (amConnected)
			amBuf = &amOutput;
		else
			amBuf = NULL;
		newDemod=true;
	}
	if (pmWasConnected != pmConnected)
	{
		LOG_DEBUG(AmFmPmBasebandDemod_i, "update pm connection ");
		if (pmConnected)
			pmBuf = &pmOutput;
		else
			pmBuf = NULL;
		newDemod=true;
	}
	if (fmWasConnected != fmConnected)
	{
		LOG_DEBUG(AmFmPmBasebandDemod_i, "update fm connection ");
		if (fmConnected)
			fmBuf = &fmOutput;
		else
			fmBuf = NULL;
		newDemod=true;
	}
	if (newDemod) {
		remakeDemods();
	}
}
