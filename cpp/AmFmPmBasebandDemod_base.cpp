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

#include "AmFmPmBasebandDemod_base.h"

/*******************************************************************************************

    AUTO-GENERATED CODE. DO NOT MODIFY

    The following class functions are for the base class for the component class. To
    customize any of these functions, do not modify them here. Instead, overload them
    on the child class

******************************************************************************************/

AmFmPmBasebandDemod_base::AmFmPmBasebandDemod_base(const char *uuid, const char *label) :
    Resource_impl(uuid, label),
    ThreadedComponent()
{
    loadProperties();

    dataFloat_In = new bulkio::InFloatPort("dataFloat_In");
    addPort("dataFloat_In", dataFloat_In);
    pm_dataFloat_out = new bulkio::OutFloatPort("pm_dataFloat_out");
    addPort("pm_dataFloat_out", pm_dataFloat_out);
    fm_dataFloat_out = new bulkio::OutFloatPort("fm_dataFloat_out");
    addPort("fm_dataFloat_out", fm_dataFloat_out);
    am_dataFloat_out = new bulkio::OutFloatPort("am_dataFloat_out");
    addPort("am_dataFloat_out", am_dataFloat_out);
}

AmFmPmBasebandDemod_base::~AmFmPmBasebandDemod_base()
{
    delete dataFloat_In;
    dataFloat_In = 0;
    delete pm_dataFloat_out;
    pm_dataFloat_out = 0;
    delete fm_dataFloat_out;
    fm_dataFloat_out = 0;
    delete am_dataFloat_out;
    am_dataFloat_out = 0;
}

/*******************************************************************************************
    Framework-level functions
    These functions are generally called by the framework to perform housekeeping.
*******************************************************************************************/
void AmFmPmBasebandDemod_base::start() throw (CORBA::SystemException, CF::Resource::StartError)
{
    Resource_impl::start();
    ThreadedComponent::startThread();
}

void AmFmPmBasebandDemod_base::stop() throw (CORBA::SystemException, CF::Resource::StopError)
{
    Resource_impl::stop();
    if (!ThreadedComponent::stopThread()) {
        throw CF::Resource::StopError(CF::CF_NOTSET, "Processing thread did not die");
    }
}

void AmFmPmBasebandDemod_base::releaseObject() throw (CORBA::SystemException, CF::LifeCycle::ReleaseError)
{
    // This function clears the component running condition so main shuts down everything
    try {
        stop();
    } catch (CF::Resource::StopError& ex) {
        // TODO - this should probably be logged instead of ignored
    }

    Resource_impl::releaseObject();
}

void AmFmPmBasebandDemod_base::loadProperties()
{
    addProperty(freqDeviation,
                0.0,
                "freqDeviation",
                "",
                "readwrite",
                "Hz",
                "external",
                "configure");

    addProperty(squelch,
                -150,
                "squelch",
                "",
                "readwrite",
                "dB",
                "external",
                "configure");

    addProperty(phaseDeviation,
                1.0,
                "phaseDeviation",
                "",
                "readwrite",
                "cycles",
                "external",
                "configure");

}


