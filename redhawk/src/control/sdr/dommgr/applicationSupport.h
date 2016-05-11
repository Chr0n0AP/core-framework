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


#ifndef APPLICATIONSUPPORT_H
#define APPLICATIONSUPPORT_H

#include <string>
#include <vector>
#include <map>
#include <list>
#include <omniORB4/CORBA.h>

#include <ossie/CF/cf.h>
#include <ossie/CF/StandardEvent.h>
#include <ossie/SoftPkg.h>
#include <ossie/SoftwareAssembly.h>
#include <ossie/ComponentDescriptor.h>
#include <ossie/Properties.h>
#include <ossie/exceptions.h>
#include <ossie/ossieparser.h>
#include <ossie/componentProfile.h>

// Application support routines

// Base class to contain data for the components required to
// create an application
// Application support routines

namespace ossie
{
    class DeviceNode;

    struct ApplicationComponent {
        std::string identifier;
        std::string softwareProfile;
        std::string namingContext;
        std::string implementationId;
        std::vector<std::string> loadedFiles;
        unsigned long processId;
        CORBA::Object_var componentObject;
        CF::Device_var assignedDevice;
    };
    typedef std::list<ApplicationComponent> ComponentList;

    /*
     *
     */
    class UsesDeviceInfo
    {

    public:
        typedef std::vector< UsesDeviceInfo * >    List;

        UsesDeviceInfo(const std::string& id, const std::string& type, const std::vector<SPD::PropertyRef>& _properties);
        UsesDeviceInfo(const std::string& id, const std::string& type, const std::vector<SoftwareAssembly::PropertyRef>& _sadDeps);
        ~UsesDeviceInfo();

        const std::string& getId() const;
        const std::string& getType() const;
        const std::vector<SPD::PropertyRef>& getProperties() const;
        const std::vector<SoftwareAssembly::PropertyRef>& getSadDeps() const;
        const std::string& getAssignedDeviceId() const;

        void setAssignedDeviceId(const std::string& deviceId);
        void clearAssignedDeviceId();

    private:
        std::string id;
        std::string type;
        std::vector<SPD::PropertyRef> properties;
        std::vector<SoftwareAssembly::PropertyRef> sadDeps;
        std::string assignedDeviceId;
    };

    /* Base class to contain data for components and implementations
     *  - Used to store information about about UsesDevice relationships
     *    since these are handled the same for both components and
     *    component implementations.
     */
    class UsesDeviceContext
    {
        ENABLE_LOGGING;

    public:
        UsesDeviceContext();
        virtual ~UsesDeviceContext();

        void addUsesDevice(UsesDeviceInfo* usesDevice);
        const UsesDeviceInfo::List & getUsesDevices() const;
        virtual const UsesDeviceInfo* getUsesDeviceById(const std::string& id) const;


    protected:
        UsesDeviceInfo::List usesDevices;
    };

    class SoftpkgInfo;

    /* Base class to contain data for implementations
     *  - Used to store information about about implementations
     */
    class ImplementationInfo : public UsesDeviceContext
    {
        ENABLE_LOGGING;

    public:
        typedef std::vector< ImplementationInfo* >  List;

        ImplementationInfo(const SPD::Implementation& spdImpl);
        ~ImplementationInfo();

        const std::string& getId() const;
        CF::LoadableDevice::LoadType getCodeType() const;
        const std::vector<std::string>& getProcessorDeps() const;
        const std::vector<ossie::SPD::NameVersionPair>& getOsDeps() const;
        const std::string& getLocalFileName() const;
        const std::string& getEntryPoint() const;
        const CORBA::ULong getStackSize() const;
        const CORBA::ULong getPriority() const;
        const bool hasStackSize() const;
        const bool hasPriority() const;
        const std::vector<SPD::PropertyRef>& getDependencyProperties() const;
        const std::vector<SoftpkgInfo*>& getSoftPkgDependency() const;

        bool checkProcessorAndOs(const ossie::Properties& prf) const;

        static ImplementationInfo* buildImplementationInfo(CF::FileManager_ptr fileMgr, const SPD::Implementation& spdImpl);

    private:
        ImplementationInfo (const ImplementationInfo&);
        void setLocalFileName(const char* fileName);
        void setEntryPoint(const char* fileName);
        void setCodeType(const char* _type);
        void setStackSize(const unsigned long long *_stackSize);
        void setPriority(const unsigned long long *_priority);
        void addDependencyProperty(const ossie::SPD::PropertyRef& property);
        void addSoftPkgDependency(SoftpkgInfo* softpkg);

        std::string id;
        CF::LoadableDevice::LoadType codeType;
        std::string localFileName;
        std::string entryPoint;
        CORBA::ULong stackSize;
        CORBA::ULong priority;
        bool _hasStackSize;
        bool _hasPriority;
        std::vector<std::string> processorDeps;
        std::vector<ossie::SPD::NameVersionPair> osDeps;
        std::vector<SPD::PropertyRef> dependencyProperties;
        std::vector<SoftpkgInfo*> softPkgDependencies;

    };

    class SoftpkgInfo : public UsesDeviceContext
    {
        ENABLE_LOGGING

    public:
        
        SoftpkgInfo (const std::string& spdFileName);
        ~SoftpkgInfo ();

        const std::string& getSpdFileName() const;
        const std::string& getName() const;

        void addImplementation(ImplementationInfo* impl);
        const ImplementationInfo::List& getImplementations() const;

        virtual const UsesDeviceInfo* getUsesDeviceById(const std::string& id) const;

        static SoftpkgInfo* buildSoftpkgInfo (CF::FileManager_ptr fileMgr, const char* spdFileName);

        SoftPkg spd;

    protected:
        bool parseProfile (CF::FileManager_ptr fileMgr);

        const std::string _spdFileName;
        std::string _name; // Component name from SPD File

        ImplementationInfo::List _implementations;
    };

    /* Base class to contain data for components
     *  - Used to store information about about components
     */
    class ComponentInfo : public SoftpkgInfo
    {
        ENABLE_LOGGING

    public:
      typedef ossie::ComponentInstantiation::AffinityProperties AffinityProperties;
      typedef ossie::ComponentInstantiation::LoggingConfig    LoggingConfig;

        ComponentInfo (const std::string& spdFileName);
        ~ComponentInfo ();

        void setIdentifier(const char* identifier, std::string instance_id);
        void setNamingService(const bool isNamingService);
        void setNamingServiceName(const char* NamingServiceName);
        void setUsageName(const char* usageName);
        void setIsAssemblyController(bool isAssemblyController);
        void setIsScaCompliant(bool isScaCompliant);
        void setAffinity( const AffinityProperties &affinity );
        void setLoggingConfig( const LoggingConfig &logcfg );
        void setStartOrder(int index);

        void addFactoryParameter(CF::DataType dt);
        void addExecParameter(CF::DataType dt);
        void addDependencyProperty(std::string implId, CF::DataType dt);
        void addConfigureProperty(CF::DataType dt);
        void addConstructProperty(CF::DataType dt);

        void overrideProperty(const ossie::ComponentProperty* propref);
        void overrideProperty(const char* id, const CORBA::Any& value);

        const std::string& getInstantiationIdentifier() const;
        const std::string& getIdentifier() const;
        bool isNamingService() const;
        const char* getUsageName() const;
        const char* getNamingServiceName() const;
        bool isResource() const;
        bool isConfigurable() const;
        bool isAssemblyController() const;
        bool isScaCompliant() const;
        bool hasStartOrder() const;
        int getStartOrder() const;

        bool isAssignedToDevice() const;
        CF::Properties containsPartialStructConfig() const;
        CF::Properties containsPartialStructConstruct() const;
        CF::Properties iteratePartialStruct(const CF::Properties &props) const;
        bool checkStruct(const CF::Properties &props) const;

        CF::Properties getNonNilConfigureProperties() const;
        CF::Properties getInitializeProperties() const;
        CF::Properties getConfigureProperties() const;
        CF::Properties getConstructProperties() const;
        CF::Properties getOptions();
        CF::Properties getAffinityOptions() const;
        CF::Properties getExecParameters();
        CF::Properties getCommandLineParameters() const;

        static ComponentInfo* buildComponentInfoFromSPDFile(CF::FileManager_ptr fileMgr, const char* _SPDFile);
        ComponentDescriptor scd;
        ossie::Properties prf;

    private:
        ComponentInfo (const ComponentInfo&);

        void process_overrides(CF::Properties* props, const char* id, CORBA::Any value);
        bool _isAssemblyController;
        bool _isConfigurable;
        bool _isScaCompliant;
        bool _isNamingService;

        std::string usageName;
        std::string identifier;
        std::string instantiationId;
        std::string namingServiceName;
        int startOrder;

        ossie::Properties _affinity_prf;
        LoggingConfig    loggingConfig;

        CF::Properties configureProperties;
        CF::Properties ctorProperties;
        CF::Properties options;
        CF::Properties factoryParameters;
        CF::Properties execParameters;
        CF::Properties affinityOptions;
        
    };

    /*
     * Class to store information for device assignment support
     */
    class DeviceAssignmentInfo
    {

    public:
        CF::DeviceAssignmentType deviceAssignment;
        CF::Device_var  device;
    };

    /* Base class to contain data for applications
     *  - Used to store information about about:
     *       -> ExternalPorts
     *       -> External Properties
     *       -> UsesDevice relationships
     */
    class ApplicationInfo : public UsesDeviceContext
    {
        ENABLE_LOGGING;

    public:
        ApplicationInfo();
        ~ApplicationInfo();

        const std::vector<SoftwareAssembly::Port>& getExternalPorts() const;
        const std::vector<SoftwareAssembly::Property>& getExternalProperties() const;
        void setACProperties(const CF::Properties& props);
        const CF::Properties getACProperties() const;
        void populateApplicationInfo(const SoftwareAssembly& sad);
        void populateExternalProperties(CF::Properties& props);
        void addComponent(ComponentInfo* comp);
        ComponentInfo* findComponentByInstantiationId(const std::string id);

    protected:
        std::vector<SoftwareAssembly::Port> externalPorts;
        std::vector<SoftwareAssembly::Property> externalProperties;
        CF::Properties acProps;
        std::vector<ComponentInfo*> components;
    };

    class PlacementPlan
    {
    public:
        PlacementPlan();
        PlacementPlan(const std::string& id, const std::string& name);

        const std::string& getId() const;
        const std::string& getName() const;

        const std::vector<ComponentInfo*>& getComponents() const;

        void addComponent(ComponentInfo* component);

    protected:
        std::string id;
        std::string name;
        std::vector<ComponentInfo*> components;
    };

    class ApplicationPlacement
    {
    public:
        typedef std::vector<PlacementPlan*> PlacementList;

        ApplicationPlacement();
        ~ApplicationPlacement();

        void addPlacement(PlacementPlan* placement);

        const PlacementList& getPlacements() const;

    protected:
        PlacementList placements;
    };

}
#endif
