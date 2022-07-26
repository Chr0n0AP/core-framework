#ifndef STRUCTPROPS_H
#define STRUCTPROPS_H

/*******************************************************************************************

    AUTO-GENERATED CODE. DO NOT MODIFY

*******************************************************************************************/

#include <ossie/CorbaUtils.h>
#include <CF/cf.h>
#include <ossie/PropertyMap.h>

struct nic_allocation_struct {
    nic_allocation_struct ()
    {
        identifier = "";
        data_rate = 0.0;
        data_size = 1;
        multicast_support = "False";
        ip_addressable = "";
        interface = "";
    }

    static std::string getId() {
        return std::string("nic_allocation");
    }

    static const char* getFormat() {
        return "sfhsss";
    }

    std::string identifier;
    float data_rate;
    short data_size;
    std::string multicast_support;
    std::string ip_addressable;
    std::string interface;
};

inline bool operator>>= (const CORBA::Any& a, nic_allocation_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("nic_allocation::identifier")) {
        if (!(props["nic_allocation::identifier"] >>= s.identifier)) return false;
    }
    if (props.contains("nic_allocation::data_rate")) {
        if (!(props["nic_allocation::data_rate"] >>= s.data_rate)) return false;
    }
    if (props.contains("nic_allocation::data_size")) {
        if (!(props["nic_allocation::data_size"] >>= s.data_size)) return false;
    }
    if (props.contains("nic_allocation::multicast_support")) {
        if (!(props["nic_allocation::multicast_support"] >>= s.multicast_support)) return false;
    }
    if (props.contains("nic_allocation::ip_addressable")) {
        if (!(props["nic_allocation::ip_addressable"] >>= s.ip_addressable)) return false;
    }
    if (props.contains("nic_allocation::interface")) {
        if (!(props["nic_allocation::interface"] >>= s.interface)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const nic_allocation_struct& s) {
    redhawk::PropertyMap props;
 
    props["nic_allocation::identifier"] = s.identifier;
 
    props["nic_allocation::data_rate"] = s.data_rate;
 
    props["nic_allocation::data_size"] = s.data_size;
 
    props["nic_allocation::multicast_support"] = s.multicast_support;
 
    props["nic_allocation::ip_addressable"] = s.ip_addressable;
 
    props["nic_allocation::interface"] = s.interface;
    a <<= props;
}

inline bool operator== (const nic_allocation_struct& s1, const nic_allocation_struct& s2) {
    if (s1.identifier!=s2.identifier)
        return false;
    if (s1.data_rate!=s2.data_rate)
        return false;
    if (s1.data_size!=s2.data_size)
        return false;
    if (s1.multicast_support!=s2.multicast_support)
        return false;
    if (s1.ip_addressable!=s2.ip_addressable)
        return false;
    if (s1.interface!=s2.interface)
        return false;
    return true;
}

inline bool operator!= (const nic_allocation_struct& s1, const nic_allocation_struct& s2) {
    return !(s1==s2);
}

struct advanced_struct {
    advanced_struct ()
    {
        maximum_throughput_percentage = 80.0;
    }

    static std::string getId() {
        return std::string("advanced");
    }

    static const char* getFormat() {
        return "d";
    }

    double maximum_throughput_percentage;
};

inline bool operator>>= (const CORBA::Any& a, advanced_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("maximum_throughput_percentage")) {
        if (!(props["maximum_throughput_percentage"] >>= s.maximum_throughput_percentage)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const advanced_struct& s) {
    redhawk::PropertyMap props;
 
    props["maximum_throughput_percentage"] = s.maximum_throughput_percentage;
    a <<= props;
}

inline bool operator== (const advanced_struct& s1, const advanced_struct& s2) {
    if (s1.maximum_throughput_percentage!=s2.maximum_throughput_percentage)
        return false;
    return true;
}

inline bool operator!= (const advanced_struct& s1, const advanced_struct& s2) {
    return !(s1==s2);
}

namespace enums {
    // Enumerated values for threshold_event
    namespace threshold_event {
        // Enumerated values for threshold_event::type
        namespace type {
            static const std::string Threshold_Exceeded = "THRESHOLD_EXCEEDED";
            static const std::string Threshold_Not_Exceeded = "THRESHOLD_NOT_EXCEEDED";
        }
    }
}

struct threshold_event_struct {
    threshold_event_struct ()
    {
    }

    static std::string getId() {
        return std::string("threshold_event");
    }

    static const char* getFormat() {
        return "sssssssd";
    }

    std::string source_id;
    std::string resource_id;
    std::string threshold_class;
    std::string type;
    std::string threshold_value;
    std::string measured_value;
    std::string message;
    double timestamp;
};

inline bool operator>>= (const CORBA::Any& a, threshold_event_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("threshold_event::source_id")) {
        if (!(props["threshold_event::source_id"] >>= s.source_id)) return false;
    }
    if (props.contains("threshold_event::resource_id")) {
        if (!(props["threshold_event::resource_id"] >>= s.resource_id)) return false;
    }
    if (props.contains("threshold_event::threshold_class")) {
        if (!(props["threshold_event::threshold_class"] >>= s.threshold_class)) return false;
    }
    if (props.contains("threshold_event::type")) {
        if (!(props["threshold_event::type"] >>= s.type)) return false;
    }
    if (props.contains("threshold_event::threshold_value")) {
        if (!(props["threshold_event::threshold_value"] >>= s.threshold_value)) return false;
    }
    if (props.contains("threshold_event::measured_value")) {
        if (!(props["threshold_event::measured_value"] >>= s.measured_value)) return false;
    }
    if (props.contains("threshold_event::message")) {
        if (!(props["threshold_event::message"] >>= s.message)) return false;
    }
    if (props.contains("threshold_event::timestamp")) {
        if (!(props["threshold_event::timestamp"] >>= s.timestamp)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const threshold_event_struct& s) {
    redhawk::PropertyMap props;
 
    props["threshold_event::source_id"] = s.source_id;
 
    props["threshold_event::resource_id"] = s.resource_id;
 
    props["threshold_event::threshold_class"] = s.threshold_class;
 
    props["threshold_event::type"] = s.type;
 
    props["threshold_event::threshold_value"] = s.threshold_value;
 
    props["threshold_event::measured_value"] = s.measured_value;
 
    props["threshold_event::message"] = s.message;
 
    props["threshold_event::timestamp"] = s.timestamp;
    a <<= props;
}

inline bool operator== (const threshold_event_struct& s1, const threshold_event_struct& s2) {
    if (s1.source_id!=s2.source_id)
        return false;
    if (s1.resource_id!=s2.resource_id)
        return false;
    if (s1.threshold_class!=s2.threshold_class)
        return false;
    if (s1.type!=s2.type)
        return false;
    if (s1.threshold_value!=s2.threshold_value)
        return false;
    if (s1.measured_value!=s2.measured_value)
        return false;
    if (s1.message!=s2.message)
        return false;
    if (s1.timestamp!=s2.timestamp)
        return false;
    return true;
}

inline bool operator!= (const threshold_event_struct& s1, const threshold_event_struct& s2) {
    return !(s1==s2);
}

struct thresholds_struct {
    thresholds_struct ()
    {
        ignore = false;
        cpu_idle = 10;
        load_avg = 80;
        mem_free = 10LL;
        nic_usage = 900;
        files_available = 3;
        threads = 3;
        shm_free = 10LL;
    }

    static std::string getId() {
        return std::string("thresholds");
    }

    static const char* getFormat() {
        return "bffliffl";
    }

    bool ignore;
    float cpu_idle;
    float load_avg;
    CORBA::LongLong mem_free;
    CORBA::Long nic_usage;
    float files_available;
    float threads;
    CORBA::LongLong shm_free;
};

inline bool operator>>= (const CORBA::Any& a, thresholds_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("ignore")) {
        if (!(props["ignore"] >>= s.ignore)) return false;
    }
    if (props.contains("cpu_idle")) {
        if (!(props["cpu_idle"] >>= s.cpu_idle)) return false;
    }
    if (props.contains("load_avg")) {
        if (!(props["load_avg"] >>= s.load_avg)) return false;
    }
    if (props.contains("mem_free")) {
        if (!(props["mem_free"] >>= s.mem_free)) return false;
    }
    if (props.contains("nic_usage")) {
        if (!(props["nic_usage"] >>= s.nic_usage)) return false;
    }
    if (props.contains("files_available")) {
        if (!(props["files_available"] >>= s.files_available)) return false;
    }
    if (props.contains("threads")) {
        if (!(props["threads"] >>= s.threads)) return false;
    }
    if (props.contains("shm_free")) {
        if (!(props["shm_free"] >>= s.shm_free)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const thresholds_struct& s) {
    redhawk::PropertyMap props;
 
    props["ignore"] = s.ignore;
 
    props["cpu_idle"] = s.cpu_idle;
 
    props["load_avg"] = s.load_avg;
 
    props["mem_free"] = s.mem_free;
 
    props["nic_usage"] = s.nic_usage;
 
    props["files_available"] = s.files_available;
 
    props["threads"] = s.threads;
 
    props["shm_free"] = s.shm_free;
    a <<= props;
}

inline bool operator== (const thresholds_struct& s1, const thresholds_struct& s2) {
    if (s1.ignore!=s2.ignore)
        return false;
    if (s1.cpu_idle!=s2.cpu_idle)
        return false;
    if (s1.load_avg!=s2.load_avg)
        return false;
    if (s1.mem_free!=s2.mem_free)
        return false;
    if (s1.nic_usage!=s2.nic_usage)
        return false;
    if (s1.files_available!=s2.files_available)
        return false;
    if (s1.threads!=s2.threads)
        return false;
    if (s1.shm_free!=s2.shm_free)
        return false;
    return true;
}

inline bool operator!= (const thresholds_struct& s1, const thresholds_struct& s2) {
    return !(s1==s2);
}

struct loadAverage_struct {
    loadAverage_struct ()
    {
    }

    static std::string getId() {
        return std::string("DCE:9da85ebc-6503-48e7-af36-b77c7ad0c2b4");
    }

    static const char* getFormat() {
        return "ddd";
    }

    double onemin;
    double fivemin;
    double fifteenmin;
};

inline bool operator>>= (const CORBA::Any& a, loadAverage_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("onemin")) {
        if (!(props["onemin"] >>= s.onemin)) return false;
    }
    if (props.contains("fivemin")) {
        if (!(props["fivemin"] >>= s.fivemin)) return false;
    }
    if (props.contains("fifteenmin")) {
        if (!(props["fifteenmin"] >>= s.fifteenmin)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const loadAverage_struct& s) {
    redhawk::PropertyMap props;
 
    props["onemin"] = s.onemin;
 
    props["fivemin"] = s.fivemin;
 
    props["fifteenmin"] = s.fifteenmin;
    a <<= props;
}

inline bool operator== (const loadAverage_struct& s1, const loadAverage_struct& s2) {
    if (s1.onemin!=s2.onemin)
        return false;
    if (s1.fivemin!=s2.fivemin)
        return false;
    if (s1.fifteenmin!=s2.fifteenmin)
        return false;
    return true;
}

inline bool operator!= (const loadAverage_struct& s1, const loadAverage_struct& s2) {
    return !(s1==s2);
}

struct gpp_limits_struct {
    gpp_limits_struct ()
    {
    }

    static std::string getId() {
        return std::string("gpp_limits");
    }

    static const char* getFormat() {
        return "iiii";
    }

    CORBA::Long current_threads;
    CORBA::Long max_threads;
    CORBA::Long current_open_files;
    CORBA::Long max_open_files;
};

inline bool operator>>= (const CORBA::Any& a, gpp_limits_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("current_threads")) {
        if (!(props["current_threads"] >>= s.current_threads)) return false;
    }
    if (props.contains("max_threads")) {
        if (!(props["max_threads"] >>= s.max_threads)) return false;
    }
    if (props.contains("current_open_files")) {
        if (!(props["current_open_files"] >>= s.current_open_files)) return false;
    }
    if (props.contains("max_open_files")) {
        if (!(props["max_open_files"] >>= s.max_open_files)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const gpp_limits_struct& s) {
    redhawk::PropertyMap props;
 
    props["current_threads"] = s.current_threads;
 
    props["max_threads"] = s.max_threads;
 
    props["current_open_files"] = s.current_open_files;
 
    props["max_open_files"] = s.max_open_files;
    a <<= props;
}

inline bool operator== (const gpp_limits_struct& s1, const gpp_limits_struct& s2) {
    if (s1.current_threads!=s2.current_threads)
        return false;
    if (s1.max_threads!=s2.max_threads)
        return false;
    if (s1.current_open_files!=s2.current_open_files)
        return false;
    if (s1.max_open_files!=s2.max_open_files)
        return false;
    return true;
}

inline bool operator!= (const gpp_limits_struct& s1, const gpp_limits_struct& s2) {
    return !(s1==s2);
}

struct sys_limits_struct {
    sys_limits_struct ()
    {
    }

    static std::string getId() {
        return std::string("sys_limits");
    }

    static const char* getFormat() {
        return "iiii";
    }

    CORBA::Long current_threads;
    CORBA::Long max_threads;
    CORBA::Long current_open_files;
    CORBA::Long max_open_files;
};

inline bool operator>>= (const CORBA::Any& a, sys_limits_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("sys_limits::current_threads")) {
        if (!(props["sys_limits::current_threads"] >>= s.current_threads)) return false;
    }
    if (props.contains("sys_limits::max_threads")) {
        if (!(props["sys_limits::max_threads"] >>= s.max_threads)) return false;
    }
    if (props.contains("sys_limits::current_open_files")) {
        if (!(props["sys_limits::current_open_files"] >>= s.current_open_files)) return false;
    }
    if (props.contains("sys_limits::max_open_files")) {
        if (!(props["sys_limits::max_open_files"] >>= s.max_open_files)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const sys_limits_struct& s) {
    redhawk::PropertyMap props;
 
    props["sys_limits::current_threads"] = s.current_threads;
 
    props["sys_limits::max_threads"] = s.max_threads;
 
    props["sys_limits::current_open_files"] = s.current_open_files;
 
    props["sys_limits::max_open_files"] = s.max_open_files;
    a <<= props;
}

inline bool operator== (const sys_limits_struct& s1, const sys_limits_struct& s2) {
    if (s1.current_threads!=s2.current_threads)
        return false;
    if (s1.max_threads!=s2.max_threads)
        return false;
    if (s1.current_open_files!=s2.current_open_files)
        return false;
    if (s1.max_open_files!=s2.max_open_files)
        return false;
    return true;
}

inline bool operator!= (const sys_limits_struct& s1, const sys_limits_struct& s2) {
    return !(s1==s2);
}

struct redhawk__reservation_request_struct {
    redhawk__reservation_request_struct ()
    {
    }

    static std::string getId() {
        return std::string("redhawk::reservation_request");
    }

    static const char* getFormat() {
        return "s[s][s]";
    }

    std::string obj_id;
    std::vector<std::string> kinds;
    std::vector<std::string> values;
};

inline bool operator>>= (const CORBA::Any& a, redhawk__reservation_request_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("redhawk::reservation_request::obj_id")) {
        if (!(props["redhawk::reservation_request::obj_id"] >>= s.obj_id)) return false;
    }
    if (props.contains("redhawk::reservation_request::kinds")) {
        if (!(props["redhawk::reservation_request::kinds"] >>= s.kinds)) return false;
    }
    if (props.contains("redhawk::reservation_request::values")) {
        if (!(props["redhawk::reservation_request::values"] >>= s.values)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const redhawk__reservation_request_struct& s) {
    redhawk::PropertyMap props;
 
    props["redhawk::reservation_request::obj_id"] = s.obj_id;
 
    props["redhawk::reservation_request::kinds"] = s.kinds;
 
    props["redhawk::reservation_request::values"] = s.values;
    a <<= props;
}

inline bool operator== (const redhawk__reservation_request_struct& s1, const redhawk__reservation_request_struct& s2) {
    if (s1.obj_id!=s2.obj_id)
        return false;
    if (s1.kinds!=s2.kinds)
        return false;
    if (s1.values!=s2.values)
        return false;
    return true;
}

inline bool operator!= (const redhawk__reservation_request_struct& s1, const redhawk__reservation_request_struct& s2) {
    return !(s1==s2);
}

namespace enums {
    // Enumerated values for affinity
    namespace affinity {
        // Enumerated values for affinity::exec_directive_class
        namespace exec_directive_class {
            static const std::string socket = "socket";
            static const std::string nic = "nic";
            static const std::string cpu = "cpu";
            static const std::string cgroup = "cgroup";
        }
    }
}

struct affinity_struct {
    affinity_struct ()
    {
        exec_directive_value = "0";
        exec_directive_class = "socket";
        force_override = false;
        blacklist_cpus = "";
        deploy_per_socket = false;
        disabled = true;
    }

    static std::string getId() {
        return std::string("affinity");
    }

    static const char* getFormat() {
        return "ssbsbb";
    }

    std::string exec_directive_value;
    std::string exec_directive_class;
    bool force_override;
    std::string blacklist_cpus;
    bool deploy_per_socket;
    bool disabled;
};

inline bool operator>>= (const CORBA::Any& a, affinity_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("affinity::exec_directive_value")) {
        if (!(props["affinity::exec_directive_value"] >>= s.exec_directive_value)) return false;
    }
    if (props.contains("affinity::exec_directive_class")) {
        if (!(props["affinity::exec_directive_class"] >>= s.exec_directive_class)) return false;
    }
    if (props.contains("affinity::force_override")) {
        if (!(props["affinity::force_override"] >>= s.force_override)) return false;
    }
    if (props.contains("affinity::blacklist_cpus")) {
        if (!(props["affinity::blacklist_cpus"] >>= s.blacklist_cpus)) return false;
    }
    if (props.contains("affinity::deploy_per_socket")) {
        if (!(props["affinity::deploy_per_socket"] >>= s.deploy_per_socket)) return false;
    }
    if (props.contains("affinity::disabled")) {
        if (!(props["affinity::disabled"] >>= s.disabled)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const affinity_struct& s) {
    redhawk::PropertyMap props;
 
    props["affinity::exec_directive_value"] = s.exec_directive_value;
 
    props["affinity::exec_directive_class"] = s.exec_directive_class;
 
    props["affinity::force_override"] = s.force_override;
 
    props["affinity::blacklist_cpus"] = s.blacklist_cpus;
 
    props["affinity::deploy_per_socket"] = s.deploy_per_socket;
 
    props["affinity::disabled"] = s.disabled;
    a <<= props;
}

inline bool operator== (const affinity_struct& s1, const affinity_struct& s2) {
    if (s1.exec_directive_value!=s2.exec_directive_value)
        return false;
    if (s1.exec_directive_class!=s2.exec_directive_class)
        return false;
    if (s1.force_override!=s2.force_override)
        return false;
    if (s1.blacklist_cpus!=s2.blacklist_cpus)
        return false;
    if (s1.deploy_per_socket!=s2.deploy_per_socket)
        return false;
    if (s1.disabled!=s2.disabled)
        return false;
    return true;
}

inline bool operator!= (const affinity_struct& s1, const affinity_struct& s2) {
    return !(s1==s2);
}

struct plugin_registration_struct {
    plugin_registration_struct ()
    {
    }

    static std::string getId() {
        return std::string("plugin::registration");
    }

    static const char* getFormat() {
        return "ssss";
    }

    std::string id;
    std::string name;
    std::string description;
    std::string metric_port;
};

inline bool operator>>= (const CORBA::Any& a, plugin_registration_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("plugin::registration::id")) {
        if (!(props["plugin::registration::id"] >>= s.id)) return false;
    }
    if (props.contains("plugin::registration::name")) {
        if (!(props["plugin::registration::name"] >>= s.name)) return false;
    }
    if (props.contains("plugin::registration::description")) {
        if (!(props["plugin::registration::description"] >>= s.description)) return false;
    }
    if (props.contains("plugin::registration::metric_port")) {
        if (!(props["plugin::registration::metric_port"] >>= s.metric_port)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const plugin_registration_struct& s) {
    redhawk::PropertyMap props;
 
    props["plugin::registration::id"] = s.id;
 
    props["plugin::registration::name"] = s.name;
 
    props["plugin::registration::description"] = s.description;
 
    props["plugin::registration::metric_port"] = s.metric_port;
    a <<= props;
}

inline bool operator== (const plugin_registration_struct& s1, const plugin_registration_struct& s2) {
    if (s1.id!=s2.id)
        return false;
    if (s1.name!=s2.name)
        return false;
    if (s1.description!=s2.description)
        return false;
    if (s1.metric_port!=s2.metric_port)
        return false;
    return true;
}

inline bool operator!= (const plugin_registration_struct& s1, const plugin_registration_struct& s2) {
    return !(s1==s2);
}

struct plugin_heartbeat_struct {
    plugin_heartbeat_struct ()
    {
    }

    static std::string getId() {
        return std::string("plugin::heartbeat");
    }

    static const char* getFormat() {
        return "s";
    }

    std::string plugin_id;
};

inline bool operator>>= (const CORBA::Any& a, plugin_heartbeat_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("plugin::heartbeat::plugin_id")) {
        if (!(props["plugin::heartbeat::plugin_id"] >>= s.plugin_id)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const plugin_heartbeat_struct& s) {
    redhawk::PropertyMap props;
 
    props["plugin::heartbeat::plugin_id"] = s.plugin_id;
    a <<= props;
}

inline bool operator== (const plugin_heartbeat_struct& s1, const plugin_heartbeat_struct& s2) {
    if (s1.plugin_id!=s2.plugin_id)
        return false;
    return true;
}

inline bool operator!= (const plugin_heartbeat_struct& s1, const plugin_heartbeat_struct& s2) {
    return !(s1==s2);
}

struct plugin_message_struct {
    plugin_message_struct ()
    {
    }

    static std::string getId() {
        return std::string("plugin::message");
    }

    static const char* getFormat() {
        return "ssbusss";
    }

    std::string plugin_id;
    std::string metric_name;
    bool busy;
    CF::UTCTime metric_timestamp;
    std::string metric_reason;
    std::string metric_threshold_value;
    std::string metric_recorded_value;
};

inline bool operator>>= (const CORBA::Any& a, plugin_message_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("plugin::message::plugin_id")) {
        if (!(props["plugin::message::plugin_id"] >>= s.plugin_id)) return false;
    }
    if (props.contains("plugin::message::metric_name")) {
        if (!(props["plugin::message::metric_name"] >>= s.metric_name)) return false;
    }
    if (props.contains("plugin::message::busy")) {
        if (!(props["plugin::message::busy"] >>= s.busy)) return false;
    }
    if (props.contains("plugin::message::metric_timestamp")) {
        if (!(props["plugin::message::metric_timestamp"] >>= s.metric_timestamp)) return false;
    }
    if (props.contains("plugin::message::metric_reason")) {
        if (!(props["plugin::message::metric_reason"] >>= s.metric_reason)) return false;
    }
    if (props.contains("plugin::message::metric_threshold_value")) {
        if (!(props["plugin::message::metric_threshold_value"] >>= s.metric_threshold_value)) return false;
    }
    if (props.contains("plugin::message::metric_recorded_value")) {
        if (!(props["plugin::message::metric_recorded_value"] >>= s.metric_recorded_value)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const plugin_message_struct& s) {
    redhawk::PropertyMap props;
 
    props["plugin::message::plugin_id"] = s.plugin_id;
 
    props["plugin::message::metric_name"] = s.metric_name;
 
    props["plugin::message::busy"] = s.busy;
 
    props["plugin::message::metric_timestamp"] = s.metric_timestamp;
 
    props["plugin::message::metric_reason"] = s.metric_reason;
 
    props["plugin::message::metric_threshold_value"] = s.metric_threshold_value;
 
    props["plugin::message::metric_recorded_value"] = s.metric_recorded_value;
    a <<= props;
}

inline bool operator== (const plugin_message_struct& s1, const plugin_message_struct& s2) {
    if (s1.plugin_id!=s2.plugin_id)
        return false;
    if (s1.metric_name!=s2.metric_name)
        return false;
    if (s1.busy!=s2.busy)
        return false;
    if (s1.metric_timestamp!=s2.metric_timestamp)
        return false;
    if (s1.metric_reason!=s2.metric_reason)
        return false;
    if (s1.metric_threshold_value!=s2.metric_threshold_value)
        return false;
    if (s1.metric_recorded_value!=s2.metric_recorded_value)
        return false;
    return true;
}

inline bool operator!= (const plugin_message_struct& s1, const plugin_message_struct& s2) {
    return !(s1==s2);
}

struct plugin_set_threshold_struct {
    plugin_set_threshold_struct ()
    {
    }

    static std::string getId() {
        return std::string("plugin::set_threshold");
    }

    static const char* getFormat() {
        return "sss";
    }

    std::string plugin_id;
    std::string metric_name;
    std::string metric_threshold_value;
};

inline bool operator>>= (const CORBA::Any& a, plugin_set_threshold_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("plugin::set_threshold::plugin_id")) {
        if (!(props["plugin::set_threshold::plugin_id"] >>= s.plugin_id)) return false;
    }
    if (props.contains("plugin::set_threshold::metric_name")) {
        if (!(props["plugin::set_threshold::metric_name"] >>= s.metric_name)) return false;
    }
    if (props.contains("plugin::set_threshold::metric_threshold_value")) {
        if (!(props["plugin::set_threshold::metric_threshold_value"] >>= s.metric_threshold_value)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const plugin_set_threshold_struct& s) {
    redhawk::PropertyMap props;
 
    props["plugin::set_threshold::plugin_id"] = s.plugin_id;
 
    props["plugin::set_threshold::metric_name"] = s.metric_name;
 
    props["plugin::set_threshold::metric_threshold_value"] = s.metric_threshold_value;
    a <<= props;
}

inline bool operator== (const plugin_set_threshold_struct& s1, const plugin_set_threshold_struct& s2) {
    if (s1.plugin_id!=s2.plugin_id)
        return false;
    if (s1.metric_name!=s2.metric_name)
        return false;
    if (s1.metric_threshold_value!=s2.metric_threshold_value)
        return false;
    return true;
}

inline bool operator!= (const plugin_set_threshold_struct& s1, const plugin_set_threshold_struct& s2) {
    return !(s1==s2);
}

struct nic_allocation_status_struct_struct {
    nic_allocation_status_struct_struct ()
    {
    }

    static std::string getId() {
        return std::string("nic_allocation_status_struct");
    }

    static const char* getFormat() {
        return "sfhsss";
    }

    std::string identifier;
    float data_rate;
    short data_size;
    std::string multicast_support;
    std::string ip_addressable;
    std::string interface;
};

inline bool operator>>= (const CORBA::Any& a, nic_allocation_status_struct_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("nic_allocation_status::identifier")) {
        if (!(props["nic_allocation_status::identifier"] >>= s.identifier)) return false;
    }
    if (props.contains("nic_allocation_status::data_rate")) {
        if (!(props["nic_allocation_status::data_rate"] >>= s.data_rate)) return false;
    }
    if (props.contains("nic_allocation_status::data_size")) {
        if (!(props["nic_allocation_status::data_size"] >>= s.data_size)) return false;
    }
    if (props.contains("nic_allocation_status::multicast_support")) {
        if (!(props["nic_allocation_status::multicast_support"] >>= s.multicast_support)) return false;
    }
    if (props.contains("nic_allocation_status::ip_addressable")) {
        if (!(props["nic_allocation_status::ip_addressable"] >>= s.ip_addressable)) return false;
    }
    if (props.contains("nic_allocation_status::interface")) {
        if (!(props["nic_allocation_status::interface"] >>= s.interface)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const nic_allocation_status_struct_struct& s) {
    redhawk::PropertyMap props;
 
    props["nic_allocation_status::identifier"] = s.identifier;
 
    props["nic_allocation_status::data_rate"] = s.data_rate;
 
    props["nic_allocation_status::data_size"] = s.data_size;
 
    props["nic_allocation_status::multicast_support"] = s.multicast_support;
 
    props["nic_allocation_status::ip_addressable"] = s.ip_addressable;
 
    props["nic_allocation_status::interface"] = s.interface;
    a <<= props;
}

inline bool operator== (const nic_allocation_status_struct_struct& s1, const nic_allocation_status_struct_struct& s2) {
    if (s1.identifier!=s2.identifier)
        return false;
    if (s1.data_rate!=s2.data_rate)
        return false;
    if (s1.data_size!=s2.data_size)
        return false;
    if (s1.multicast_support!=s2.multicast_support)
        return false;
    if (s1.ip_addressable!=s2.ip_addressable)
        return false;
    if (s1.interface!=s2.interface)
        return false;
    return true;
}

inline bool operator!= (const nic_allocation_status_struct_struct& s1, const nic_allocation_status_struct_struct& s2) {
    return !(s1==s2);
}

struct nic_metrics_struct_struct {
    nic_metrics_struct_struct ()
    {
        interface = "";
        mac_address = "";
        rate = 0.0;
        ipv4_address = "";
        ipv4_netmask = "";
        ipv4_broadcast = "";
        ipv6_address = "";
        ipv6_netmask = "";
        ipv6_scope = "";
        flags = "";
        module = "";
        mtu = "";
        state = "";
        rx_bytes = "";
        rx_compressed = "";
        rx_crc_errors = "";
        rx_dropped = "";
        rx_errors = "";
        rx_packets = "";
        tx_bytes = "";
        tx_compressed = "";
        tx_dropped = "";
        tx_errors = "";
        tx_packets = "";
        tx_queue_len = "";
        vlans = "";
        multicast_support = false;
        rate_allocated = 0;
        time_string_utc = "";
        time = 0;
        current_throughput = 0;
    }

    static std::string getId() {
        return std::string("nic_metrics_struct");
    }

    static const char* getFormat() {
        return "ssdsssssssssssssssssssssssbdsdd";
    }

    std::string interface;
    std::string mac_address;
    double rate;
    std::string ipv4_address;
    std::string ipv4_netmask;
    std::string ipv4_broadcast;
    std::string ipv6_address;
    std::string ipv6_netmask;
    std::string ipv6_scope;
    std::string flags;
    std::string module;
    std::string mtu;
    std::string state;
    std::string rx_bytes;
    std::string rx_compressed;
    std::string rx_crc_errors;
    std::string rx_dropped;
    std::string rx_errors;
    std::string rx_packets;
    std::string tx_bytes;
    std::string tx_compressed;
    std::string tx_dropped;
    std::string tx_errors;
    std::string tx_packets;
    std::string tx_queue_len;
    std::string vlans;
    bool multicast_support;
    double rate_allocated;
    std::string time_string_utc;
    double time;
    double current_throughput;
};

inline bool operator>>= (const CORBA::Any& a, nic_metrics_struct_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("nic_metrics::interface")) {
        if (!(props["nic_metrics::interface"] >>= s.interface)) return false;
    }
    if (props.contains("nic_metrics::mac_address")) {
        if (!(props["nic_metrics::mac_address"] >>= s.mac_address)) return false;
    }
    if (props.contains("nic_metrics::rate")) {
        if (!(props["nic_metrics::rate"] >>= s.rate)) return false;
    }
    if (props.contains("nic_metrics::ipv4_address")) {
        if (!(props["nic_metrics::ipv4_address"] >>= s.ipv4_address)) return false;
    }
    if (props.contains("nic_metrics::ipv4_netmask")) {
        if (!(props["nic_metrics::ipv4_netmask"] >>= s.ipv4_netmask)) return false;
    }
    if (props.contains("nic_metrics::ipv4_broadcast")) {
        if (!(props["nic_metrics::ipv4_broadcast"] >>= s.ipv4_broadcast)) return false;
    }
    if (props.contains("nic_metrics::ipv6_address")) {
        if (!(props["nic_metrics::ipv6_address"] >>= s.ipv6_address)) return false;
    }
    if (props.contains("nic_metrics::ipv6_netmask")) {
        if (!(props["nic_metrics::ipv6_netmask"] >>= s.ipv6_netmask)) return false;
    }
    if (props.contains("nic_metrics::ipv6_scope")) {
        if (!(props["nic_metrics::ipv6_scope"] >>= s.ipv6_scope)) return false;
    }
    if (props.contains("nic_metrics::flags")) {
        if (!(props["nic_metrics::flags"] >>= s.flags)) return false;
    }
    if (props.contains("nic_metrics::module")) {
        if (!(props["nic_metrics::module"] >>= s.module)) return false;
    }
    if (props.contains("nic_metrics::mtu")) {
        if (!(props["nic_metrics::mtu"] >>= s.mtu)) return false;
    }
    if (props.contains("nic_metrics::state")) {
        if (!(props["nic_metrics::state"] >>= s.state)) return false;
    }
    if (props.contains("nic_metrics::rx_bytes")) {
        if (!(props["nic_metrics::rx_bytes"] >>= s.rx_bytes)) return false;
    }
    if (props.contains("nic_metrics::rx_compressed")) {
        if (!(props["nic_metrics::rx_compressed"] >>= s.rx_compressed)) return false;
    }
    if (props.contains("nic_metrics::rx_crc_errors")) {
        if (!(props["nic_metrics::rx_crc_errors"] >>= s.rx_crc_errors)) return false;
    }
    if (props.contains("nic_metrics::rx_dropped")) {
        if (!(props["nic_metrics::rx_dropped"] >>= s.rx_dropped)) return false;
    }
    if (props.contains("nic_metrics::rx_errors")) {
        if (!(props["nic_metrics::rx_errors"] >>= s.rx_errors)) return false;
    }
    if (props.contains("nic_metrics::rx_packets")) {
        if (!(props["nic_metrics::rx_packets"] >>= s.rx_packets)) return false;
    }
    if (props.contains("nic_metrics::tx_bytes")) {
        if (!(props["nic_metrics::tx_bytes"] >>= s.tx_bytes)) return false;
    }
    if (props.contains("nic_metrics::tx_compressed")) {
        if (!(props["nic_metrics::tx_compressed"] >>= s.tx_compressed)) return false;
    }
    if (props.contains("nic_metrics::tx_dropped")) {
        if (!(props["nic_metrics::tx_dropped"] >>= s.tx_dropped)) return false;
    }
    if (props.contains("nic_metrics::tx_errors")) {
        if (!(props["nic_metrics::tx_errors"] >>= s.tx_errors)) return false;
    }
    if (props.contains("nic_metrics::tx_packets")) {
        if (!(props["nic_metrics::tx_packets"] >>= s.tx_packets)) return false;
    }
    if (props.contains("nic_metrics::tx_queue_len")) {
        if (!(props["nic_metrics::tx_queue_len"] >>= s.tx_queue_len)) return false;
    }
    if (props.contains("nic_metrics::vlans")) {
        if (!(props["nic_metrics::vlans"] >>= s.vlans)) return false;
    }
    if (props.contains("nic_metrics::multicast_support")) {
        if (!(props["nic_metrics::multicast_support"] >>= s.multicast_support)) return false;
    }
    if (props.contains("nic_metrics::rate_allocated")) {
        if (!(props["nic_metrics::rate_allocated"] >>= s.rate_allocated)) return false;
    }
    if (props.contains("nic_metrics::time_string_utc")) {
        if (!(props["nic_metrics::time_string_utc"] >>= s.time_string_utc)) return false;
    }
    if (props.contains("nic_metrics::time")) {
        if (!(props["nic_metrics::time"] >>= s.time)) return false;
    }
    if (props.contains("nic_metrics::current_throughput")) {
        if (!(props["nic_metrics::current_throughput"] >>= s.current_throughput)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const nic_metrics_struct_struct& s) {
    redhawk::PropertyMap props;
 
    props["nic_metrics::interface"] = s.interface;
 
    props["nic_metrics::mac_address"] = s.mac_address;
 
    props["nic_metrics::rate"] = s.rate;
 
    props["nic_metrics::ipv4_address"] = s.ipv4_address;
 
    props["nic_metrics::ipv4_netmask"] = s.ipv4_netmask;
 
    props["nic_metrics::ipv4_broadcast"] = s.ipv4_broadcast;
 
    props["nic_metrics::ipv6_address"] = s.ipv6_address;
 
    props["nic_metrics::ipv6_netmask"] = s.ipv6_netmask;
 
    props["nic_metrics::ipv6_scope"] = s.ipv6_scope;
 
    props["nic_metrics::flags"] = s.flags;
 
    props["nic_metrics::module"] = s.module;
 
    props["nic_metrics::mtu"] = s.mtu;
 
    props["nic_metrics::state"] = s.state;
 
    props["nic_metrics::rx_bytes"] = s.rx_bytes;
 
    props["nic_metrics::rx_compressed"] = s.rx_compressed;
 
    props["nic_metrics::rx_crc_errors"] = s.rx_crc_errors;
 
    props["nic_metrics::rx_dropped"] = s.rx_dropped;
 
    props["nic_metrics::rx_errors"] = s.rx_errors;
 
    props["nic_metrics::rx_packets"] = s.rx_packets;
 
    props["nic_metrics::tx_bytes"] = s.tx_bytes;
 
    props["nic_metrics::tx_compressed"] = s.tx_compressed;
 
    props["nic_metrics::tx_dropped"] = s.tx_dropped;
 
    props["nic_metrics::tx_errors"] = s.tx_errors;
 
    props["nic_metrics::tx_packets"] = s.tx_packets;
 
    props["nic_metrics::tx_queue_len"] = s.tx_queue_len;
 
    props["nic_metrics::vlans"] = s.vlans;
 
    props["nic_metrics::multicast_support"] = s.multicast_support;
 
    props["nic_metrics::rate_allocated"] = s.rate_allocated;
 
    props["nic_metrics::time_string_utc"] = s.time_string_utc;
 
    props["nic_metrics::time"] = s.time;
 
    props["nic_metrics::current_throughput"] = s.current_throughput;
    a <<= props;
}

inline bool operator== (const nic_metrics_struct_struct& s1, const nic_metrics_struct_struct& s2) {
    if (s1.interface!=s2.interface)
        return false;
    if (s1.mac_address!=s2.mac_address)
        return false;
    if (s1.rate!=s2.rate)
        return false;
    if (s1.ipv4_address!=s2.ipv4_address)
        return false;
    if (s1.ipv4_netmask!=s2.ipv4_netmask)
        return false;
    if (s1.ipv4_broadcast!=s2.ipv4_broadcast)
        return false;
    if (s1.ipv6_address!=s2.ipv6_address)
        return false;
    if (s1.ipv6_netmask!=s2.ipv6_netmask)
        return false;
    if (s1.ipv6_scope!=s2.ipv6_scope)
        return false;
    if (s1.flags!=s2.flags)
        return false;
    if (s1.module!=s2.module)
        return false;
    if (s1.mtu!=s2.mtu)
        return false;
    if (s1.state!=s2.state)
        return false;
    if (s1.rx_bytes!=s2.rx_bytes)
        return false;
    if (s1.rx_compressed!=s2.rx_compressed)
        return false;
    if (s1.rx_crc_errors!=s2.rx_crc_errors)
        return false;
    if (s1.rx_dropped!=s2.rx_dropped)
        return false;
    if (s1.rx_errors!=s2.rx_errors)
        return false;
    if (s1.rx_packets!=s2.rx_packets)
        return false;
    if (s1.tx_bytes!=s2.tx_bytes)
        return false;
    if (s1.tx_compressed!=s2.tx_compressed)
        return false;
    if (s1.tx_dropped!=s2.tx_dropped)
        return false;
    if (s1.tx_errors!=s2.tx_errors)
        return false;
    if (s1.tx_packets!=s2.tx_packets)
        return false;
    if (s1.tx_queue_len!=s2.tx_queue_len)
        return false;
    if (s1.vlans!=s2.vlans)
        return false;
    if (s1.multicast_support!=s2.multicast_support)
        return false;
    if (s1.rate_allocated!=s2.rate_allocated)
        return false;
    if (s1.time_string_utc!=s2.time_string_utc)
        return false;
    if (s1.time!=s2.time)
        return false;
    if (s1.current_throughput!=s2.current_throughput)
        return false;
    return true;
}

inline bool operator!= (const nic_metrics_struct_struct& s1, const nic_metrics_struct_struct& s2) {
    return !(s1==s2);
}

struct interfaces_struct {
    interfaces_struct ()
    {
    }

    static std::string getId() {
        return std::string("interfaces");
    }

    static const char* getFormat() {
        return "sfs";
    }

    std::string interface;
    float throughput;
    std::string vlans;
};

inline bool operator>>= (const CORBA::Any& a, interfaces_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("interface")) {
        if (!(props["interface"] >>= s.interface)) return false;
    }
    if (props.contains("throughput")) {
        if (!(props["throughput"] >>= s.throughput)) return false;
    }
    if (props.contains("vlans")) {
        if (!(props["vlans"] >>= s.vlans)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const interfaces_struct& s) {
    redhawk::PropertyMap props;
 
    props["interface"] = s.interface;
 
    props["throughput"] = s.throughput;
 
    props["vlans"] = s.vlans;
    a <<= props;
}

inline bool operator== (const interfaces_struct& s1, const interfaces_struct& s2) {
    if (s1.interface!=s2.interface)
        return false;
    if (s1.throughput!=s2.throughput)
        return false;
    if (s1.vlans!=s2.vlans)
        return false;
    return true;
}

inline bool operator!= (const interfaces_struct& s1, const interfaces_struct& s2) {
    return !(s1==s2);
}

struct utilization_entry_struct {
    utilization_entry_struct ()
    {
    }

    static std::string getId() {
        return std::string("utilization_entry");
    }

    static const char* getFormat() {
        return "sffff";
    }

    std::string description;
    float component_load;
    float system_load;
    float subscribed;
    float maximum;
};

inline bool operator>>= (const CORBA::Any& a, utilization_entry_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("description")) {
        if (!(props["description"] >>= s.description)) return false;
    }
    if (props.contains("component_load")) {
        if (!(props["component_load"] >>= s.component_load)) return false;
    }
    if (props.contains("system_load")) {
        if (!(props["system_load"] >>= s.system_load)) return false;
    }
    if (props.contains("subscribed")) {
        if (!(props["subscribed"] >>= s.subscribed)) return false;
    }
    if (props.contains("maximum")) {
        if (!(props["maximum"] >>= s.maximum)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const utilization_entry_struct& s) {
    redhawk::PropertyMap props;
 
    props["description"] = s.description;
 
    props["component_load"] = s.component_load;
 
    props["system_load"] = s.system_load;
 
    props["subscribed"] = s.subscribed;
 
    props["maximum"] = s.maximum;
    a <<= props;
}

inline bool operator== (const utilization_entry_struct& s1, const utilization_entry_struct& s2) {
    if (s1.description!=s2.description)
        return false;
    if (s1.component_load!=s2.component_load)
        return false;
    if (s1.system_load!=s2.system_load)
        return false;
    if (s1.subscribed!=s2.subscribed)
        return false;
    if (s1.maximum!=s2.maximum)
        return false;
    return true;
}

inline bool operator!= (const utilization_entry_struct& s1, const utilization_entry_struct& s2) {
    return !(s1==s2);
}

struct component_monitor_struct {
    component_monitor_struct ()
    {
    }

    static std::string getId() {
        return std::string("component_monitor::component_monitor");
    }

    static const char* getFormat() {
        return "ssHfffIII";
    }

    std::string component_id;
    std::string waveform_id;
    unsigned short pid;
    float cores;
    float mem_rss;
    float mem_percent;
    CORBA::ULong num_processes;
    CORBA::ULong num_threads;
    CORBA::ULong num_files;
};

inline bool operator>>= (const CORBA::Any& a, component_monitor_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("component_monitor::component_monitor::component_id")) {
        if (!(props["component_monitor::component_monitor::component_id"] >>= s.component_id)) return false;
    }
    if (props.contains("component_monitor::component_monitor::waveform_id")) {
        if (!(props["component_monitor::component_monitor::waveform_id"] >>= s.waveform_id)) return false;
    }
    if (props.contains("component_monitor::component_monitor::pid")) {
        if (!(props["component_monitor::component_monitor::pid"] >>= s.pid)) return false;
    }
    if (props.contains("component_monitor::component_monitor::cores")) {
        if (!(props["component_monitor::component_monitor::cores"] >>= s.cores)) return false;
    }
    if (props.contains("component_monitor::component_monitor::mem_rss")) {
        if (!(props["component_monitor::component_monitor::mem_rss"] >>= s.mem_rss)) return false;
    }
    if (props.contains("component_monitor::component_monitor::mem_percent")) {
        if (!(props["component_monitor::component_monitor::mem_percent"] >>= s.mem_percent)) return false;
    }
    if (props.contains("component_monitor::component_monitor::num_processes")) {
        if (!(props["component_monitor::component_monitor::num_processes"] >>= s.num_processes)) return false;
    }
    if (props.contains("component_monitor::component_monitor::num_threads")) {
        if (!(props["component_monitor::component_monitor::num_threads"] >>= s.num_threads)) return false;
    }
    if (props.contains("component_monitor::component_monitor::num_files")) {
        if (!(props["component_monitor::component_monitor::num_files"] >>= s.num_files)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const component_monitor_struct& s) {
    redhawk::PropertyMap props;
 
    props["component_monitor::component_monitor::component_id"] = s.component_id;
 
    props["component_monitor::component_monitor::waveform_id"] = s.waveform_id;
 
    props["component_monitor::component_monitor::pid"] = s.pid;
 
    props["component_monitor::component_monitor::cores"] = s.cores;
 
    props["component_monitor::component_monitor::mem_rss"] = s.mem_rss;
 
    props["component_monitor::component_monitor::mem_percent"] = s.mem_percent;
 
    props["component_monitor::component_monitor::num_processes"] = s.num_processes;
 
    props["component_monitor::component_monitor::num_threads"] = s.num_threads;
 
    props["component_monitor::component_monitor::num_files"] = s.num_files;
    a <<= props;
}

inline bool operator== (const component_monitor_struct& s1, const component_monitor_struct& s2) {
    if (s1.component_id!=s2.component_id)
        return false;
    if (s1.waveform_id!=s2.waveform_id)
        return false;
    if (s1.pid!=s2.pid)
        return false;
    if (s1.cores!=s2.cores)
        return false;
    if (s1.mem_rss!=s2.mem_rss)
        return false;
    if (s1.mem_percent!=s2.mem_percent)
        return false;
    if (s1.num_processes!=s2.num_processes)
        return false;
    if (s1.num_threads!=s2.num_threads)
        return false;
    if (s1.num_files!=s2.num_files)
        return false;
    return true;
}

inline bool operator!= (const component_monitor_struct& s1, const component_monitor_struct& s2) {
    return !(s1==s2);
}

struct plugin_status_template_struct {
    plugin_status_template_struct ()
    {
    }

    static std::string getId() {
        return std::string("plugin::status::template");
    }

    static const char* getFormat() {
        return "sssbbI[s]";
    }

    std::string id;
    std::string name;
    std::string description;
    bool busy;
    bool alive;
    CORBA::ULong pid;
    std::vector<std::string> metric_names;
};

inline bool operator>>= (const CORBA::Any& a, plugin_status_template_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("plugin::status::id")) {
        if (!(props["plugin::status::id"] >>= s.id)) return false;
    }
    if (props.contains("plugin::status::name")) {
        if (!(props["plugin::status::name"] >>= s.name)) return false;
    }
    if (props.contains("plugin::status::description")) {
        if (!(props["plugin::status::description"] >>= s.description)) return false;
    }
    if (props.contains("plugin::status::busy")) {
        if (!(props["plugin::status::busy"] >>= s.busy)) return false;
    }
    if (props.contains("plugin::status::alive")) {
        if (!(props["plugin::status::alive"] >>= s.alive)) return false;
    }
    if (props.contains("plugin::status::pid")) {
        if (!(props["plugin::status::pid"] >>= s.pid)) return false;
    }
    if (props.contains("plugin::status::metric_names")) {
        if (!(props["plugin::status::metric_names"] >>= s.metric_names)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const plugin_status_template_struct& s) {
    redhawk::PropertyMap props;
 
    props["plugin::status::id"] = s.id;
 
    props["plugin::status::name"] = s.name;
 
    props["plugin::status::description"] = s.description;
 
    props["plugin::status::busy"] = s.busy;
 
    props["plugin::status::alive"] = s.alive;
 
    props["plugin::status::pid"] = s.pid;
 
    props["plugin::status::metric_names"] = s.metric_names;
    a <<= props;
}

inline bool operator== (const plugin_status_template_struct& s1, const plugin_status_template_struct& s2) {
    if (s1.id!=s2.id)
        return false;
    if (s1.name!=s2.name)
        return false;
    if (s1.description!=s2.description)
        return false;
    if (s1.busy!=s2.busy)
        return false;
    if (s1.alive!=s2.alive)
        return false;
    if (s1.pid!=s2.pid)
        return false;
    if (s1.metric_names!=s2.metric_names)
        return false;
    return true;
}

inline bool operator!= (const plugin_status_template_struct& s1, const plugin_status_template_struct& s2) {
    return !(s1==s2);
}

struct plugin_metric_status_template_struct {
    plugin_metric_status_template_struct ()
    {
    }

    static std::string getId() {
        return std::string("plugin::metric_status::template");
    }

    static const char* getFormat() {
        return "ssbusss";
    }

    std::string plugin_id;
    std::string metric_name;
    bool busy;
    CF::UTCTime metric_timestamp;
    std::string metric_reason;
    std::string metric_threshold_value;
    std::string metric_recorded_value;
};

inline bool operator>>= (const CORBA::Any& a, plugin_metric_status_template_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("plugin::metric_status::plugin_id")) {
        if (!(props["plugin::metric_status::plugin_id"] >>= s.plugin_id)) return false;
    }
    if (props.contains("plugin::metric_status::metric_name")) {
        if (!(props["plugin::metric_status::metric_name"] >>= s.metric_name)) return false;
    }
    if (props.contains("plugin::metric_status::busy")) {
        if (!(props["plugin::metric_status::busy"] >>= s.busy)) return false;
    }
    if (props.contains("plugin::metric_status::metric_timestamp")) {
        if (!(props["plugin::metric_status::metric_timestamp"] >>= s.metric_timestamp)) return false;
    }
    if (props.contains("plugin::metric_status::metric_reason")) {
        if (!(props["plugin::metric_status::metric_reason"] >>= s.metric_reason)) return false;
    }
    if (props.contains("plugin::metric_status::metric_threshold_value")) {
        if (!(props["plugin::metric_status::metric_threshold_value"] >>= s.metric_threshold_value)) return false;
    }
    if (props.contains("plugin::metric_status::metric_recorded_value")) {
        if (!(props["plugin::metric_status::metric_recorded_value"] >>= s.metric_recorded_value)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const plugin_metric_status_template_struct& s) {
    redhawk::PropertyMap props;
 
    props["plugin::metric_status::plugin_id"] = s.plugin_id;
 
    props["plugin::metric_status::metric_name"] = s.metric_name;
 
    props["plugin::metric_status::busy"] = s.busy;
 
    props["plugin::metric_status::metric_timestamp"] = s.metric_timestamp;
 
    props["plugin::metric_status::metric_reason"] = s.metric_reason;
 
    props["plugin::metric_status::metric_threshold_value"] = s.metric_threshold_value;
 
    props["plugin::metric_status::metric_recorded_value"] = s.metric_recorded_value;
    a <<= props;
}

inline bool operator== (const plugin_metric_status_template_struct& s1, const plugin_metric_status_template_struct& s2) {
    if (s1.plugin_id!=s2.plugin_id)
        return false;
    if (s1.metric_name!=s2.metric_name)
        return false;
    if (s1.busy!=s2.busy)
        return false;
    if (s1.metric_timestamp!=s2.metric_timestamp)
        return false;
    if (s1.metric_reason!=s2.metric_reason)
        return false;
    if (s1.metric_threshold_value!=s2.metric_threshold_value)
        return false;
    if (s1.metric_recorded_value!=s2.metric_recorded_value)
        return false;
    return true;
}

inline bool operator!= (const plugin_metric_status_template_struct& s1, const plugin_metric_status_template_struct& s2) {
    return !(s1==s2);
}

#endif // STRUCTPROPS_H
