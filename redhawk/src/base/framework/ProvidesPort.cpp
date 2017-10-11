/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of REDHAWK core.
 *
 * REDHAWK core is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * REDHAWK core is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 */

#include <ossie/ProvidesPort.h>
#include <ossie/PropertyMap.h>
#include <ossie/ossieSupport.h>

namespace redhawk {

    NegotiableProvidesPortBase::NegotiableProvidesPortBase(const std::string& name) :
        Port_Provides_base_impl(name)
    {
    }
    
    NegotiableProvidesPortBase::~NegotiableProvidesPortBase()
    {
    }
     
    void NegotiableProvidesPortBase::initializePort()
    {
        const std::string repid = getRepid();
        TransportFactory* transport = TransportRegistry::GetTransport(repid);
        RH_NL_INFO("NegotiableProvidesPortBase", "Adding provides transport '" << transport->transportType()
                   << "' for '" << transport->repid() << "'");
        _addTransportManager(transport->transportType(), transport->createProvidesManager(this));
    }

    ExtendedCF::TransportInfoSequence* NegotiableProvidesPortBase::supportedTransports()
    {
        ExtendedCF::TransportInfoSequence_var transports = new ExtendedCF::TransportInfoSequence;
        for (TransportManagerMap::iterator manager = _transportManagers.begin(); manager != _transportManagers.end(); ++manager) {
            ExtendedCF::TransportInfo transport;
            transport.transportName = manager->first.c_str();
            transport.transportProperties = manager->second->transportProperties();
            ossie::corba::push_back(transports, transport);
        }
        return transports._retn();
    }

    ExtendedCF::NegotiationResult* NegotiableProvidesPortBase::negotiateTransport(const char* protocol, const CF::Properties& props)
    {
        ProvidesTransportManager* manager = _getTransportManager(protocol);
        if (!manager) {
            std::string message = "Cannot negotiate protocol '" + std::string(protocol) + "'";
            throw ExtendedCF::NegotiationError(message.c_str());
        }

        ProvidesTransport* transport = manager->createInput(props);
        transport->start();

        std::string negotiationId = ossie::generateUUID();
        _transports[negotiationId] = transport;

        ExtendedCF::NegotiationResult_var result = new ExtendedCF::NegotiationResult;
        result->negotiationId = negotiationId.c_str();
        return result._retn();
    }

    void NegotiableProvidesPortBase::disconnectTransport(const char* connectionId)
    {
    }

    ProvidesTransportManager* NegotiableProvidesPortBase::_getTransportManager(const std::string& protocol)
    {
        TransportManagerMap::iterator manager = _transportManagers.find(protocol);
        if (manager == _transportManagers.end()) {
            return 0;
        }
        return manager->second;
    }

    void NegotiableProvidesPortBase::_addTransportManager(const std::string& name, ProvidesTransportManager* manager)
    {
        _transportManagers[name] = manager;
    }
}
