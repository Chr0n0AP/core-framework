/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of REDHAWK codegenTesting.
 *
 * REDHAWK codegenTesting is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * REDHAWK codegenTesting is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 */

/**************************************************************************

    This is the component code. This file contains the child class where
    custom functionality can be added to the component. Custom
    functionality to the base class can be extended here. Access to
    the ports can also be done from this class

 	Source: event_props.spd.xml
 	Generated on: Mon Apr 30 15:18:54 EDT 2012
 	Redhawk IDE
 	Version:T.1.X.X
 	Build id: v201204260944-r8153

**************************************************************************/

#include "event_props.h"

PREPARE_LOGGING(event_props_i)

event_props_i::event_props_i(const char *uuid, const char *label) : 
    event_props_base(uuid, label)
{
	setPropertyChangeListener("propToSend", this, &event_props_i::propToSendChanged);
}

event_props_i::~event_props_i()
{

}

void event_props_i::propToSendChanged (const std::string& id) {
	eventSent = false;
	if (propToSend == "eventShortSimple") {
		propEvent->sendPropertyEvent("eventShortSimple");
	} else if (propToSend == "eventStringSimple") {
		propEvent->sendPropertyEvent("eventStringSimple");
	} else if (propToSend == "eventBoolSimple") {
		propEvent->sendPropertyEvent("eventBoolSimple");
	} else if (propToSend == "eventUlongSimple") {
		propEvent->sendPropertyEvent("eventUlongSimple");
	} else if (propToSend == "eventFloatSimple") {
		propEvent->sendPropertyEvent("eventFloatSimple");
	} else if (propToSend == "eventOctetSimple") {
		propEvent->sendPropertyEvent("eventOctetSimple");
	} else if (propToSend == "eventCharSimple") {
		propEvent->sendPropertyEvent("eventCharSimple");
	} else if (propToSend == "eventUshortSimple") {
		propEvent->sendPropertyEvent("eventUshortSimple");
	} else if (propToSend == "eventDoubleSimple") {
		propEvent->sendPropertyEvent("eventDoubleSimple");
	} else if (propToSend == "eventLongSimple") {
		propEvent->sendPropertyEvent("eventLongSimple");
	} else if (propToSend == "eventLonglongSimple") {
		propEvent->sendPropertyEvent("eventLonglongSimple");
	} else if (propToSend == "eventUlonglongSimple") {
		propEvent->sendPropertyEvent("eventUlonglongSimple");
	} else if (propToSend == "eventStringSeq") {
		propEvent->sendPropertyEvent("eventStringSeq");
	} else if (propToSend == "eventBoolSeq") {
		propEvent->sendPropertyEvent("eventBoolSeq");
	} else if (propToSend == "eventUlongSeq") {
		propEvent->sendPropertyEvent("eventUlongSeq");
	} else if (propToSend == "eventFloatSeq") {
		propEvent->sendPropertyEvent("eventFloatSeq");
	} else if (propToSend == "eventOctetSeq") {
		propEvent->sendPropertyEvent("eventOctetSeq");
	} else if (propToSend == "eventCharSeq") {
		propEvent->sendPropertyEvent("eventCharSeq");
	} else if (propToSend == "eventUshortSeq") {
		propEvent->sendPropertyEvent("eventUshortSeq");
	} else if (propToSend == "eventDoubleSeq") {
		propEvent->sendPropertyEvent("eventDoubleSeq");
	} else if (propToSend == "eventLongSeq") {
		propEvent->sendPropertyEvent("eventLongSeq");
	} else if (propToSend == "eventLonglongSeq") {
		propEvent->sendPropertyEvent("eventLonglongSeq");
	} else if (propToSend == "eventUlonglongSeq") {
		propEvent->sendPropertyEvent("eventUlonglongSeq");
	} else if (propToSend == "eventShortSeq") {
		propEvent->sendPropertyEvent("eventShortSeq");
	} else if (propToSend == "eventStruct") {
		propEvent->sendPropertyEvent("eventStruct");
	} else if (propToSend == "eventStructSeq") {
		propEvent->sendPropertyEvent("eventStructSeq");
	}
	eventSent = true;
}

/***********************************************************************************************

    Basic functionality:

        The service function is called by the serviceThread object (of type ProcessThread).
        This call happens immediately after the previous call if the return value for
        the previous call was NORMAL.
        If the return value for the previous call was NOOP, then the serviceThread waits
        an amount of time defined in the serviceThread's constructor.
        
    SRI:
        To create a StreamSRI object, use the following code:
        	stream_id = "";
	    	sri = BULKIO::StreamSRI();
	    	sri.hversion = 1;
	    	sri.xstart = 0.0;
	    	sri.xdelta = 0.0;
	    	sri.xunits = BULKIO::UNITS_TIME;
	    	sri.subsize = 0;
	    	sri.ystart = 0.0;
	    	sri.ydelta = 0.0;
	    	sri.yunits = BULKIO::UNITS_NONE;
	    	sri.mode = 0;
	    	sri.streamID = this->stream_id.c_str();

	Time:
	    To create a PrecisionUTCTime object, use the following code:
	        struct timeval tmp_time;
	        struct timezone tmp_tz;
	        gettimeofday(&tmp_time, &tmp_tz);
	        double wsec = tmp_time.tv_sec;
	        double fsec = tmp_time.tv_usec / 1e6;;
	        BULKIO::PrecisionUTCTime tstamp = BULKIO::PrecisionUTCTime();
	        tstamp.tcmode = BULKIO::TCM_CPU;
	        tstamp.tcstatus = (short)1;
	        tstamp.toff = 0.0;
	        tstamp.twsec = wsec;
	        tstamp.tfsec = fsec;
        
    Ports:

        Data is passed to the serviceFunction through the getPacket call (BULKIO only).
        The dataTransfer class is a port-specific class, so each port implementing the
        BULKIO interface will have its own type-specific dataTransfer.

        The argument to the getPacket function is a floating point number that specifies
        the time to wait in seconds. A zero value is non-blocking. A negative value
        is blocking.

        Each received dataTransfer is owned by serviceFunction and *MUST* be
        explicitly deallocated.

        To send data using a BULKIO interface, a convenience interface has been added 
        that takes a std::vector as the data input

        NOTE: If you have a BULKIO dataSDDS port, you must manually call 
              "port->updateStats()" to update the port statistics when appropriate.

        Example:
            // this example assumes that the component has two ports:
            //  A provides (input) port of type BULKIO::dataShort called short_in
            //  A uses (output) port of type BULKIO::dataFloat called float_out
            // The mapping between the port and the class is found
            // in the component base class header file

            BULKIO_dataShort_In_i::dataTransfer *tmp = short_in->getPacket(-1);
            if (not tmp) { // No data is available
                return NOOP;
            }

            std::vector<float> outputData;
            outputData.resize(tmp->dataBuffer.size());
            for (unsigned int i=0; i<tmp->dataBuffer.size(); i++) {
                outputData[i] = (float)tmp->dataBuffer[i];
            }

            // NOTE: You must make at least one valid pushSRI call
            if (tmp->sriChanged) {
                float_out->pushSRI(tmp->SRI);
            }
            float_out->pushPacket(outputData, tmp->T, tmp->EOS, tmp->streamID);

            delete tmp; // IMPORTANT: MUST RELEASE THE RECEIVED DATA BLOCK
            return NORMAL;

        Interactions with non-BULKIO ports are left up to the component developer's discretion

    Properties:
        
        Properties are accessed directly as member variables. For example, if the
        property name is "baudRate", it may be accessed within member functions as
        "baudRate". Unnamed properties are given a generated name of the form
        "prop_n", where "n" is the ordinal number of the property in the PRF file.
        Property types are mapped to the nearest C++ type, (e.g. "string" becomes
        "std::string"). All generated properties are declared in the base class
        (event_props_base).
    
        Simple sequence properties are mapped to "std::vector" of the simple type.
        Struct properties, if used, are mapped to C++ structs defined in the
        generated file "struct_props.h". Field names are taken from the name in
        the properties file; if no name is given, a generated name of the form
        "field_n" is used, where "n" is the ordinal number of the field.
        
        Example:
            // This example makes use of the following Properties:
            //  - A float value called scaleValue
            //  - A boolean called scaleInput
            
            if (scaleInput) {
                dataOut[i] = dataIn[i] * scaleValue;
            } else {
                dataOut[i] = dataIn[i];
            }
            
        
************************************************************************************************/
int event_props_i::serviceFunction()
{
    LOG_DEBUG(event_props_i, "serviceFunction() example log message");
	return NOOP;
}
