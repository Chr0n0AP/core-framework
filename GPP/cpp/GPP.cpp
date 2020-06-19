/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of REDHAWK GPP.
 *
 * REDHAWK GPP is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * REDHAWK GPP is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 */
/**************************************************************************

    This is the device code. This file contains the child class where
    custom functionality can be added to the device. Custom
    functionality to the base class can be extended here. Access to
    the ports can also be done from this class

**************************************************************************/
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <limits>
#include <linux/limits.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include <libgen.h>
#include <glob.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/signalfd.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/epoll.h>
#include <boost/filesystem/path.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/regex.hpp>
#include <boost/foreach.hpp>
#ifdef  HAVE_LIBNUMA
#include <numa.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <ossie/Events.h>
#include <ossie/affinity.h>
#include <ossie/shm/System.h>
#include <ossie/shm/Allocator.h>
#include <ossie/shm/SuperblockFile.h>

#include "GPP.h"
#include "utils/affinity.h"
#include "utils/SymlinkReader.h"
#include "parsers/PidProcStatParser.h"
#include "states/ProcStat.h"
#include "states/ProcMeminfo.h"
#include "statistics/CpuUsageStats.h"


class SigChildThread : public ThreadedComponent {
  friend class GPP_i;
public:
  SigChildThread( GPP_i &p):
    parent(p)
 {};
  int serviceFunction() {
    return parent.sigchld_handler(0);
  }
private:
  GPP_i &parent;
};


class RedirectedIO : public ThreadedComponent {
  friend class GPP_i;
public:
  RedirectedIO( GPP_i &p):
    parent(p)
 {};
  int serviceFunction() {
    return parent.redirected_io_handler();
  }
private:
  GPP_i &parent;
};


static const uint64_t MB_TO_BYTES = 1024*1024;

uint64_t conv_units( const std::string &units ) {
  uint64_t unit_m=1024*1024;
  if ( units == "Kb" ) unit_m = 1e3;
  if ( units == "Mb" ) unit_m = 1e6;
  if ( units == "Gb" ) unit_m = 1e9;
  if ( units == "Tb" ) unit_m = 1e12;
  if ( units == "KB" ) unit_m = 1024;
  if ( units == "MB" || units == "MiB" ) unit_m = MB_TO_BYTES;
  if ( units == "GB" ) unit_m = 1024*1024*1024;
  if ( units == "TB" ) unit_m = (uint64_t)1024*1024*1024*1024;
  return unit_m;
}


const std::string  __ExpandEnvVars(const std::string& original) {

  typedef std::list< std::pair<const std::string,const std::string> > t2StrLst;

  std::string result = original;
  const boost::regex envscan("\\$([0-9A-Za-z_]*)");
  const boost::sregex_iterator end;
  t2StrLst replacements;
  for (boost::sregex_iterator rit(result.begin(), result.end(), envscan); rit != end; ++rit)
      replacements.push_back(std::make_pair((*rit)[0],(*rit)[1]));
  for (t2StrLst::const_iterator lit = replacements.begin(); lit != replacements.end(); ++lit)  {
    const char* expanded = std::getenv(lit->second.c_str());
    if (expanded == NULL)
      continue;
    boost::replace_all(result, lit->first, expanded);
  }

  replacements.clear();

  const boost::regex envscan2("\\$\\{([0-9A-Za-z_]*)\\}");
  for (boost::sregex_iterator rit(result.begin(), result.end(), envscan2); rit != end; ++rit)
    replacements.push_back(std::make_pair((*rit)[0],(*rit)[1]));
  for (t2StrLst::const_iterator lit = replacements.begin(); lit != replacements.end(); ++lit)
    {
      const char* expanded = std::getenv(lit->second.c_str());
      if (expanded == NULL)
        continue;
      boost::replace_all(result, lit->first, expanded);
    }

  return result;
}


const std::string  __ExpandProperties(const std::string& original, const CF::Properties &props) {
  std::string result = original;
  const boost::regex envscan("@([0-9A-Za-z_]*)@");
  const boost::sregex_iterator end;
  typedef std::list< std::pair<const std::string,const std::string> > t2StrLst;
  t2StrLst replacements;
  for (boost::sregex_iterator rit(result.begin(), result.end(), envscan); rit != end; ++rit)
    replacements.push_back(std::make_pair((*rit)[0],(*rit)[1]));
  const redhawk::PropertyMap& pmap = redhawk::PropertyMap::cast(props);
  for (t2StrLst::const_iterator lit = replacements.begin(); lit != replacements.end(); ++lit)
    {
      if ( pmap.find(lit->second.c_str()) != pmap.end()) {
        std::string expanded = pmap[lit->second.c_str()].toString();
        boost::replace_all(result, lit->first, expanded);
      }
    }
  return result;
}



//
// resolve StdOutLogger symbol for compiler...
//
namespace rh_logger {
  class StdOutLogger : public Logger {
  public:
    static  LoggerPtr  getRootLogger( );
  };
};


//
//  proc_redirect class and helpers
//
class FindRedirect : public std::binary_function< GPP_i::ProcRedirectPtr, int, bool >  {

public:
    //    bool operator() ( const GPP_i::proc_redirect &a, const int &pid ) const {
  bool operator() ( const GPP_i::ProcRedirectPtr a, const int &pid ) const {
    return a->pid == pid;
  };
};


GPP_i::proc_redirect::proc_redirect( int _pid, int _cout, int _cerr ):
  pid(_pid), cout(_cout), cerr(_cerr), fname("")
  { 
  };

GPP_i::proc_redirect::proc_redirect( const std::string &_fname, int _pid, int _cout, int _cerr ):
  pid(_pid), cout(_cout), cerr(_cerr), fname(_fname)
  { 
  };

void GPP_i::proc_redirect::close() {
  if ( cout > -1 ) ::close(cout);
  if ( cerr > -1 ) ::close(cerr);
}

//
//  component_description class and helpers
//

class FindPid : public std::binary_function< GPP_i::component_description, int, bool >  {

public:
  bool operator() ( const GPP_i::component_description &a, const int &pid ) const {
    return a.pid == pid;
  }
};

class FindApp : public std::binary_function< GPP_i::component_description, std::string, bool  >  {

public:
  bool operator() ( const GPP_i::component_description &a, const std::string &appName ) const {
    return a.appName == appName;
  }
};

inline bool operator== (const GPP_i::component_description& s1, 
			const GPP_i::component_description& s2) {
    if (s1.appName!=s2.appName)
        return false;
    if (s1.identifier!=s2.identifier)
        return false;
    return true;
};


GPP_i::component_description::component_description() :
  pid(-1),
  appName(""), 
  identifier(""),
  app_started(false),
  reservation(-1.0),
  terminated(false),
  reaped(false),
  pstat_idx(0)
{ memset(pstat_history, 0, sizeof(pstat_history) ); }


GPP_i::component_description::component_description( const std::string &appId) :
  pid(-1),
  appName(appId), 
  identifier(""),
  app_started(false),
  reservation(-1.0),
  terminated(false),
  reaped(false),
  pstat_idx(0)
{ memset(pstat_history, 0, sizeof(pstat_history) ); }


int64_t GPP_i::component_description::get_process_time() 
{   
  int64_t retval = 0;
  if (parent->grp_children.find(pid) == parent->grp_children.end())
      return retval;
  BOOST_FOREACH(const int &_pid, parent->grp_children[pid].pids) {
    PidProcStatParser pstat_file(_pid);
    if ( pstat_file.parse() < 0 ) {
        return -1;
    }
    retval += pstat_file.get_ticks();
  }
  return retval;
}

void GPP_i::component_description::add_history( int64_t ptime ) {
  if ( ptime == -1 ) ptime=0; /// Log error ..
  pstat_idx = (pstat_idx + 1)% pstat_history_len;
  pstat_history[pstat_idx] = ptime;
}

void GPP_i::component_description::add_history( ) {
  int64_t ptime = get_process_time();
  if ( ptime < 0 ) ptime=0; /// Log error ..
  pstat_idx = (pstat_idx + 1)% pstat_history_len;
  pstat_history[pstat_idx] = ptime;
}

int64_t  GPP_i::component_description::get_pstat_usage( bool refresh) {
  if ( refresh) add_history();
  int64_t retval=0;
  int8_t p1_idx = pstat_idx -1;
  if ( p1_idx < 0 ) p1_idx = pstat_history_len-1;
  uint64_t p1=pstat_history[p1_idx];
  uint64_t p2=pstat_history[pstat_idx];
  retval=p2-p1; 
  if ( (p2-p1) < 0 )retval=p1-p2;
  return retval;
}

int64_t  GPP_i::component_description::get_pstat_usage( uint64_t &p2,  uint64_t &p1 ){
  int64_t retval=0;
  p2 = pstat_history[pstat_idx];
  int8_t p1_idx = pstat_idx -1;
  if ( p1_idx < 0 ) p1_idx = pstat_history_len-1;
  p1=pstat_history[p1_idx];
  retval=p2-p1; 
  if ( (p2-p1) < 0 )retval=p1-p2;
  return retval;
}


PREPARE_LOGGING(GPP_i)

extern GPP_i *devicePtr;

std::string GPP_i::format_up_time(unsigned long secondsUp)
{
  std::stringstream formattedUptime;
  int days;
  int hours;
  int minutes;
  int seconds;

  int leftover;

  days = (int) secondsUp / (60 * 60 * 24);
  leftover = (int) secondsUp - (days * (60 * 60 * 24) );
  hours = (int) leftover / (60 * 60);
  leftover = leftover - (hours * (60 * 60) );
  minutes = (int) leftover / 60;
  seconds = leftover - (minutes * 60);

  formattedUptime << days << "d " << hours << "h " << minutes << "m " << seconds << "s";

  return formattedUptime.str();
}

GPP_i::GPP_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl) :
  GPP_base(devMgr_ior, id, lbl, sftwrPrfl),
  _signalThread( new SigChildThread(*this), 0.1 ),
  _redirectedIO( new RedirectedIO(*this), 0.1 )
{
  _init();
}

GPP_i::GPP_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev) :
  GPP_base(devMgr_ior, id, lbl, sftwrPrfl, compDev),
  _signalThread( new SigChildThread(*this), 0.1 ),
  _redirectedIO( new RedirectedIO(*this), 0.1 )
{
 _init();
}

GPP_i::GPP_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities) :
  GPP_base(devMgr_ior, id, lbl, sftwrPrfl, capacities),
  _signalThread( new SigChildThread(*this), 0.1 ),
  _redirectedIO( new RedirectedIO(*this), 0.1 )
{
  _init();
}

GPP_i::GPP_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev) :
  GPP_base(devMgr_ior, id, lbl, sftwrPrfl, capacities, compDev),
  _signalThread( new SigChildThread(*this), 0.1 ),
  _redirectedIO( new RedirectedIO(*this), 0.1 )
{
  _init();
}

GPP_i::~GPP_i()
{

}

void GPP_i::_init() {

  // get the user id
  uid_t tmp_user_id = getuid();
  std::ostringstream s;
  s << tmp_user_id;
  user_id = s.str();
  n_reservations =0;
  sig_fd = -1;
  _forkMsg=FORK_GO;
  
  //
  // io redirection for child processes
  //
  _handle_io_redirects = false;
  _componentOutputLog ="";
  epfd=epoll_create(400);

  //
  // add our local set affinity method that performs numa library calls
  //
  redhawk::affinity::override_set( &gpp::affinity::set_affinity );

  //
  // get canonical machine context
  //
  hostName = boost::asio::ip::host_name();

  struct utsname _uts;
  if (uname(&_uts) != -1) {
    os_name = _uts.sysname;
    os_version = _uts.release;
  }

  // figure out the location of the binary
  std::ostringstream lpath;
  lpath << "/proc/"<<getpid()<<"/exe";
  try {
    std::string binary_file = SymlinkReader::ReadLink(lpath.str());
    binary_location = binary_file.substr(0, binary_file.find_last_of('/')+1);    
  }
  catch(...){
  }

  // default cycle time setting for updating data model, metrics and state
  threshold_cycle_time = 500;
  thresholds.ignore = false;

  //
  // Add property change listeners and allocation modifiers
  //

  // Add property change listeners for affinity information...
  addPropertyListener(affinity, this, &GPP_i::_affinity_changed);

  // add property change listener
  addPropertyListener(reserved_capacity_per_component, this, &GPP_i::reservedChanged);

  // add property change listener
  addPropertyListener(componentOutputLog, this, &GPP_i::_component_output_changed);

  // add property change listener
  addPropertyListener(mcastnicThreshold, this, &GPP_i::mcastnicThreshold_changed);

  // add property change listener thresholds
  addPropertyListener(thresholds, this, &GPP_i::thresholds_changed);

  utilization_entry_struct cpu;
  cpu.description = "CPU cores";
  cpu.component_load = 0;
  cpu.system_load = 0;
  cpu.subscribed = 0;
  cpu.maximum = 0;
  utilization.push_back(cpu);

  setPropertyQueryImpl(this->component_monitor, this, &GPP_i::get_component_monitor);

  // tie allocation modifier callbacks to identifiers

  // nic allocation 
  setAllocationImpl("nic_allocation", this, &GPP_i::allocateCapacity_nic_allocation, &GPP_i::deallocateCapacity_nic_allocation);

  // support for older nic ingress/egress allocators
  setAllocationImpl("DCE:eb08e43f-11c7-45a0-8750-edff439c8b24", this, &GPP_i::allocate_mcastegress_capacity, &GPP_i::deallocate_mcastegress_capacity);

  // support for older nic ingress/egress allocators
  setAllocationImpl("DCE:506102d6-04a9-4532-9420-a323d818ddec", this, &GPP_i::allocate_mcastingress_capacity, &GPP_i::deallocate_mcastingress_capacity);

  // load capacity allocations
  setAllocationImpl("DCE:72c1c4a9-2bcf-49c5-bafd-ae2c1d567056", this, &GPP_i::allocate_loadCapacity, &GPP_i::deallocate_loadCapacity);

  // check  memory capacity allocations 
  setAllocationImpl("DCE:8dcef419-b440-4bcf-b893-cab79b6024fb", this, &GPP_i::allocate_memCapacity, &GPP_i::deallocate_memCapacity);

  //setAllocationImpl("diskCapacity", this, &GPP_i::allocate_diskCapacity, &GPP_i::deallocate_diskCapacity);
  
  // check  reservation allocations 
  setAllocationImpl(this->redhawk__reservation_request, this, &GPP_i::allocate_reservation_request, &GPP_i::deallocate_reservation_request);

}

void GPP_i::constructor()
{
    // Get the initial working directory
    char buf[PATH_MAX+1];
    getcwd(buf, sizeof(buf));
    std::string path = buf;

    // If a working directory was given, change to that
    if (!workingDirectory.empty()) {
        if (chdir(workingDirectory.c_str())) {
            RH_ERROR(_baseLog, "Cannot change working directory to " << workingDirectory);
            workingDirectory = "";
        }
    }
    // Otherwise, default to the initial working directory
    if (workingDirectory.empty()) {
        workingDirectory = path;
    }

    // If no cache directory given, use initial working directory
    if (cacheDirectory.empty()) {
        cacheDirectory = path;
    }

    RH_DEBUG(_baseLog, "Working directory: " << workingDirectory);
    RH_DEBUG(_baseLog, "Cache directory: " << cacheDirectory);

    shmCapacity = redhawk::shm::getSystemTotalMemory() / MB_TO_BYTES;

    // Initialize system and user CPU ticks
    ProcStat::GetTicks(_systemTicks, _userTicks);
}

void GPP_i::postConstruction (std::string &profile, 
                                     std::string &registrar_ior, 
                                     const std::string &idm_channel_ior,
                                     const std::string &nic,
                                     const int sigfd )
{

  Device_impl::postConstruction(profile,registrar_ior,idm_channel_ior,nic,sigfd );
  // if sigfd > 0 then signalfd method was establish via command line USESIGFD
  if ( sigfd > 0 ) {
    sig_fd = sigfd;
  }
  else {
    // require signalfd to be configured before orb init call.... 
    throw std::runtime_error("unable configure signal handler");
  }

  _pluginBusy = false;
  metrics_in->registerMessage("plugin::registration", this, &GPP_i::pluginRegistration);
  metrics_in->registerMessage("plugin::heartbeat", this, &GPP_i::pluginHeartbeat);
  metrics_in->registerMessage("plugin::message", this, &GPP_i::pluginMessage);
  launchPlugins();

  _signalThread.start();

}

void GPP_i::pluginRegistration(const std::string& messageId, const plugin_registration_struct& msgData)
{
    ReadLock rlock(monitorLock);

    plugin_status_template_struct new_plugin;
    new_plugin.id = msgData.id;
    new_plugin.name = msgData.name;
    new_plugin.description = msgData.description;
    plugin_status.push_back(new_plugin);

    plugin_description new_registration;
    new_registration.name = msgData.name;
    new_registration.description = msgData.description;
    new_registration.status_idx = plugin_status.size()-1;
    new_registration.alive = true;
    _plugins[msgData.id] = new_registration;
}

void GPP_i::pluginHeartbeat(const std::string& messageId, const plugin_heartbeat_struct& msgData)
{
    ReadLock rlock(monitorLock);
}

void GPP_i::pluginMessage(const std::string& messageId, const plugin_message_struct& msgData)
{
    ReadLock rlock(monitorLock);

    std::pair<std::string, std::string> plugin_metric_tuple = std::make_pair(msgData.id, msgData.metric_name);
    size_t metrics_idx = 0;
    if (_plugin_metrics.find(plugin_metric_tuple) == _plugin_metrics.end()) {
        plugin_metric_status.resize(plugin_metric_status.size()+1);
        _plugin_metrics[plugin_metric_tuple].status_idx = plugin_metric_status.size()-1;
    }

    metrics_idx = _plugin_metrics[plugin_metric_tuple].status_idx;
    plugin_metric_status[metrics_idx].ok = msgData.ok;
    plugin_metric_status[metrics_idx].id = msgData.id;
    plugin_metric_status[metrics_idx].metric_name = msgData.metric_name;
    plugin_metric_status[metrics_idx].metric_timestamp = msgData.metric_timestamp;
    plugin_metric_status[metrics_idx].metric_reason = msgData.metric_reason;
    plugin_metric_status[metrics_idx].metric_threshold_value = msgData.metric_threshold_value;
    plugin_metric_status[metrics_idx].metric_recorded_value = msgData.metric_recorded_value;

    _plugin_metrics[plugin_metric_tuple].ok = msgData.ok;
    _plugin_metrics[plugin_metric_tuple].name = msgData.metric_name;
    _plugin_metrics[plugin_metric_tuple].metric_timestamp = msgData.metric_timestamp;
    _plugin_metrics[plugin_metric_tuple].metric_reason = msgData.metric_reason;
    _plugin_metrics[plugin_metric_tuple].metric_threshold_value = msgData.metric_threshold_value;
    _plugin_metrics[plugin_metric_tuple].metric_recorded_value = msgData.metric_recorded_value;
    if (not msgData.ok) {
        if (not _pluginBusy) {
            _pluginBusy = true;
            std::ostringstream oss;
            oss << "Threshold: " <<  msgData.metric_threshold_value << " Actual: " << msgData.metric_recorded_value;
            _setBusyReason(msgData.metric_reason, oss.str());
        }
    }
    bool all_ok = true;
    for (unsigned int i=0; i<plugin_status.size(); i++) {
        if (not plugin_status[i].ok) {
            all_ok = false;
            break;
        }
    }
    if (all_ok) {
        _pluginBusy = false;
    }

    size_t plugin_idx = _plugins[msgData.id].status_idx;
    all_ok = true;
    for (std::map< std::pair<std::string, std::string>, metric_description>::iterator it=_plugin_metrics.begin();it!=_plugin_metrics.end();++it) {
        if (it->first.first == msgData.id) {
            if (not it->second.ok) {
                all_ok = false;
                break;
            }
        }
    }
    plugin_status[plugin_idx].ok = all_ok;
    bool found_metric = false;
    for (std::vector<std::string>::iterator it=plugin_status[plugin_idx].metric_names.begin();it!=plugin_status[plugin_idx].metric_names.end();++it) {
        if (*it == msgData.metric_name) {
            found_metric = true;
        }
    }
    if (not found_metric) {
        plugin_status[plugin_idx].metric_names.push_back(msgData.metric_name);
    }
}

void GPP_i::launchPlugins() {

    boost::filesystem::path dirPath(getenv( "SDRROOT" ));
    dirPath /= "/dev/devices/GPP/plugins";
    
    int gpp_pid = getpid();

    const boost::filesystem::directory_iterator end_itr; // an end iterator (by boost definition)
    for (boost::filesystem::directory_iterator itr = boost::filesystem::directory_iterator(dirPath); itr != end_itr; ++itr) {
        if (boost::filesystem::is_directory(itr->path())) {
            boost::filesystem::path found_plugin(itr->path());
            found_plugin /= itr->path().filename();
            if ((not boost::filesystem::is_directory(found_plugin)) and boost::filesystem::exists(found_plugin)) {
                std::vector<std::string> args;
                args.push_back(found_plugin.string());
                args.push_back(ossie::corba::objectToString(this->metrics_in->_this()));
                args.push_back(ossie::generateUUID());

                std::vector<char*> argv(args.size()+1, NULL);
                for (std::size_t i = 0; i < args.size(); ++i) {
                    argv[i] = const_cast<char*> (args[i].c_str());
                }
                int pid = fork();
                if (pid == 0) {
                    setpgid(gpp_pid, 0);
                    int returnval = execv(argv[0], &argv[0]);
                    if (errno) {
                        std::string error_msg("Unable to launch plugin '");
                        error_msg += found_plugin.string();
                        error_msg += "': ";
                        switch (errno) {
                            case E2BIG:
                                std::cout<<error_msg<<"Argument list too long"<<std::endl;
                                break;
                            case EACCES:
                                std::cout<<error_msg<<"Permission denied"<<std::endl;
                                break;
                            case ENAMETOOLONG:
                                std::cout<<error_msg<<"File name too long"<<std::endl;
                                break;
                            case ENOENT:
                                std::cout<<error_msg<<"No such file or directory"<<std::endl;
                                break;
                            case ENOEXEC:
                                std::cout<<error_msg<<"Exec format error"<<std::endl;
                                break;
                            case ENOMEM:
                                std::cout<<error_msg<<"Out of memory"<<std::endl;
                                break;
                            case ENOTDIR:
                                std::cout<<error_msg<<"Not a directory"<<std::endl;
                                break;
                            case EPERM:
                                std::cout<<error_msg<<"Operation not permitted"<<std::endl;
                                break;
                            default:
                                std::cout<<error_msg<<"Error on fork with error number "<<errno<<std::endl;
                                break;
                        }
                    }
                    exit(returnval);
                } else if (pid < 0) {
                    std::cout<<"failed to launch"<<std::endl;
                }
            }
        }
    }

    // scan directory
    // loop over plugins
    // fork plugins
}

void GPP_i::update_grp_child_pids() {
    glob_t globbuf;
    std::vector<int> pids_now;
    glob("/proc/[0-9]*", GLOB_NOSORT, NULL, &globbuf);
    for (unsigned int i = 0; i < globbuf.gl_pathc; i++) {
        std::string stat_filename(globbuf.gl_pathv[globbuf.gl_pathc - i - 1]);
        std::string proc_id(stat_filename.substr(stat_filename.rfind("/")+1));
        try {
            pids_now.push_back(boost::lexical_cast<int>(proc_id));
        }
        catch(...){
            std::stringstream errstr;
            errstr << "Unable to process id:  "<<proc_id;
            RH_DEBUG(this->_baseLog, __FUNCTION__ << ": " << errstr.str() );
            continue;
        }
    }
    globfree(&globbuf);
    boost::regex re_("-?\\d+|[[:alpha:]]+|\\(.*\\)");
    BOOST_FOREACH(const int &_pid, pids_now) {
        if (parsed_stat.find(_pid) == parsed_stat.end()) { // it is not on the map
            std::stringstream stat_filename;
            stat_filename << "/proc/"<<_pid<<"/stat";
            std::string line;
            unsigned fcnt=0;
            proc_values tmp;
            int pid;
            try {
                std::ifstream istr(stat_filename.str().c_str());
                std::getline(istr, line);
                boost::sregex_token_iterator j;
                boost::sregex_token_iterator i(line.begin(), line.end(), re_);
                try {
                    for( fcnt=0; i != j; i++, fcnt++) {
                        if ( fcnt == 23 ) {  // rss pages
                            tmp.mem_rss = boost::lexical_cast<float>(*i) * getpagesize() / (1024*1024);
                            continue;
                        }
                        if ( fcnt == 19 ) { // threads
                            tmp.num_threads = boost::lexical_cast<CORBA::ULong>(*i);
                            continue;
                        }
                        if ( fcnt == 4 ) { // process group id
                            tmp.pgrpid = boost::lexical_cast<int>(*i);
                            continue;
                        }
                        if ( fcnt == 0 ) { // pid
                            pid = boost::lexical_cast<int>(*i);
                            continue;
                        }
                    }

                } catch ( ... ) {
                    std::stringstream errstr;
                    errstr << "Invalid line format in stat file, pid :" << _pid << " field number " << fcnt << " line " << line ;
                    RH_WARN(this->_baseLog, __FUNCTION__ << ": " << errstr.str() );
                    continue;
                }

            } catch ( ... ) {
                std::stringstream errstr;
                errstr << "Unable to read "<<stat_filename<<". The process is no longer there";
                RH_DEBUG(this->_baseLog, __FUNCTION__ << ": " << errstr.str() );
                continue;
            }
            if ( fcnt < 37 ) {
                std::stringstream errstr;
                errstr << "Insufficient fields proc/<pid>/stat: "<<stat_filename.str()<<" file (expected>=37 received=" << fcnt << ")";
                RH_DEBUG(this->_baseLog, __FUNCTION__ << ": " << errstr.str() );
                continue;
            }
            parsed_stat[pid] = tmp;
            if (grp_children.find(tmp.pgrpid) == grp_children.end()) {
                grp_children[tmp.pgrpid].num_processes = 1;
                grp_children[tmp.pgrpid].mem_rss = tmp.mem_rss;
                grp_children[tmp.pgrpid].num_threads = tmp.num_threads;
                grp_children[tmp.pgrpid].pgrpid = tmp.pgrpid;
                grp_children[tmp.pgrpid].pids.push_back(pid);
            } else {
                grp_children[tmp.pgrpid].num_processes += 1;
                grp_children[tmp.pgrpid].mem_rss += tmp.mem_rss;
                grp_children[tmp.pgrpid].num_threads += tmp.num_threads;
                grp_children[tmp.pgrpid].pids.push_back(pid);
            }
        }
    }
    std::vector<int> parsed_stat_to_erase;
    for(std::map<int, proc_values>::iterator _it = parsed_stat.begin(); _it != parsed_stat.end(); _it++) {
        if (std::find(pids_now.begin(), pids_now.end(), _it->first) == pids_now.end()) {  // it is not on the current process list
            if (grp_children.find(parsed_stat[_it->first].pgrpid) != grp_children.end()) {
                std::vector<int>::iterator it = std::find(grp_children[parsed_stat[_it->first].pgrpid].pids.begin(), grp_children[parsed_stat[_it->first].pgrpid].pids.end(), _it->first);
                if (it != grp_children[parsed_stat[_it->first].pgrpid].pids.end())
                    grp_children[parsed_stat[_it->first].pgrpid].pids.erase(it);
            }
            parsed_stat_to_erase.push_back(_it->first);
        }
    }
    BOOST_FOREACH(const int &_pid, parsed_stat_to_erase) {
        parsed_stat.erase(_pid);
    }
}

std::vector<component_monitor_struct> GPP_i::get_component_monitor() {
    ReadLock rlock(pidLock);
    std::vector<component_monitor_struct> retval;
    struct sysinfo info;
    sysinfo(&info);
    BOOST_FOREACH(const component_description &_pid, pids) {
        if ( !_pid.terminated ) {
            if ((grp_children.find(_pid.pid) == grp_children.end()) or (parsed_stat.find(_pid.pid) == parsed_stat.end())) {
                std::stringstream errstr;
                errstr << "Could not find /proc/"<<_pid.pid<<"/stat. The process corresponding to component "<<_pid.identifier<<" is no longer there";
                RH_WARN(this->_baseLog, __FUNCTION__ << ": " << errstr.str() );
                continue;
            }
            component_monitor_struct tmp;
            tmp.waveform_id = _pid.appName;
            tmp.pid = _pid.pid;
            tmp.component_id = _pid.identifier;
            tmp.num_processes = grp_children[_pid.pid].num_processes;
            tmp.cores = _pid.core_usage;

            tmp.mem_rss = grp_children[_pid.pid].mem_rss;
            tmp.mem_percent = (double) grp_children[_pid.pid].mem_rss * (1024*1024) / ((double)info.totalram * info.mem_unit) * 100;
            tmp.num_threads = grp_children[_pid.pid].num_threads;
            
            tmp.num_files = 0;
            BOOST_FOREACH(const int &actual_pid, grp_children[_pid.pid].pids) {
                std::stringstream fd_dirname;
                DIR * dirp;
                struct dirent * entry;
                fd_dirname <<"/proc/"<<actual_pid<<"/fd";
                dirp = opendir(fd_dirname.str().c_str());
                if (dirp != NULL) {
                    while ((entry = readdir(dirp)) != NULL) {
                        if (entry->d_type != DT_DIR) {  // If the entry is not a directory
                            tmp.num_files++;
                        }
                    }
                    closedir (dirp);
                }
            }

            retval.push_back(tmp);
        }
    }
    
    return retval;
}

void GPP_i::process_ODM(const CORBA::Any &data) {
    const ExtendedEvent::ResourceStateChangeEventType* app_state_change;
    if (data >>= app_state_change) {
        std::string appId = ossie::corba::returnString(app_state_change->sourceId);
        if (app_state_change->stateChangeTo == ExtendedEvent::STARTED) {
            RH_NL_TRACE("GPP", "ODM CHANNEL EVENT --> APP STARTED app: " << appId );
            ReadLock rlock(pidLock);
            ProcessList::iterator i = pids.begin();
            // set app_started ... turns off reservation
            while ( i != pids.end() ) {
              i=std::find_if( i, pids.end(), std::bind2nd( FindApp(), appId ) );
              if ( i != pids.end() ) {
                i->app_started = true;
                RH_TRACE(this->_baseLog, "Monitor_Processes.. APP STARTED:" << i->pid << " app: " << i->appName );
                i++;
              }
            }
        } else if (app_state_change->stateChangeTo == ExtendedEvent::STOPPED) {
            RH_NL_TRACE("GPP", "ODM CHANNEL EVENT --> APP STOPPED app: " << appId );
            ReadLock rlock(pidLock);
            ProcessList::iterator i = pids.begin();
            // set app_started ... turns on reservation
            while ( i != pids.end() ) {
              i=std::find_if( i, pids.end(), std::bind2nd( FindApp(), appId ) );
              if ( i != pids.end() ) {
                i->app_started = false;
                RH_TRACE(this->_baseLog, "Monitor_Processes.. APP STOPPED :" << i->pid << " app: " << i->appName );
                i++;
              }
            }
        }
    }
    const StandardEvent::DomainManagementObjectRemovedEventType* app_removed;
    if (data >>= app_removed) {
        if (app_removed->sourceCategory == StandardEvent::APPLICATION) {
            WriteLock rlock(pidLock);
            std::string producerId(app_removed->producerId);
            for (ApplicationReservationMap::iterator app_it=applicationReservations.begin(); app_it!=applicationReservations.end(); app_it++) {
                if (app_it->first == producerId) {
                    applicationReservations.erase(app_it);
                    break;
                }
            }
        }
    }
}

int GPP_i::_setupExecPartitions( const CpuList &bl_cpus ) {

#if HAVE_LIBNUMA
    if ( gpp::affinity::check_numa() == true ) {
        // fill in the exec partitions for each numa node identified on the system
        std::string nodestr("all");
        struct bitmask *node_mask = numa_parse_nodestring((char *)nodestr.c_str());     

        bitmask *cpu_mask = numa_allocate_cpumask(); 
    
        // for each node bit set in the mask then get cpu list
        int nbytes = numa_bitmask_nbytes(node_mask);
        for (int i=0; i < nbytes*8; i++ ){
            if ( numa_bitmask_isbitset( node_mask, i ) ) {
                numa_node_to_cpus( i, cpu_mask );
              
                // foreach cpu identified add to list
                int nb = numa_bitmask_nbytes(cpu_mask);
                CpuUsageStats::CpuList cpus;
                for (int j=0; j < nb*8; j++ ){
                    int count =  std::count( bl_cpus.begin(), bl_cpus.end(), j );
                    if ( numa_bitmask_isbitset( cpu_mask, j ) && count == 0 ) {
                        cpus.push_back( j );
                    }
                }
                CpuUsageStats cpu_usage(cpus);
                exec_socket soc;
                soc.id = i;
                soc.cpus = cpus;
                soc.stats = cpu_usage;
                if (!thresholds.ignore && (thresholds.cpu_idle >= 0.0)) {
                    soc.idle_threshold = thresholds.cpu_idle;
                } else {
                    soc.idle_threshold = 0.0;
                }
                soc.load_capacity.max =  cpus.size() * 1.0;
                soc.load_capacity.measured = 0.0;
                soc.load_capacity.allocated = 0.0;
                execPartitions.push_back( soc );
            }
        }
    }

#endif

    if ( execPartitions.size()  ) {
      ExecPartitionList::iterator iter =  execPartitions.begin();
      std::ostringstream ss;
      ss  << boost::format("%-6s %-4s %-7s %-7s %-7s ") % "SOCKET" % "CPUS" % "USER"  % "SYSTEM"  % "IDLE"  ;
      RH_TRACE(this->_baseLog, ss.str()  );
      ss.clear();
      ss.str("");
      for ( ; iter != execPartitions.end(); iter++ ) {
        iter->update();  iter->update();
        ss  << boost::format("%-6d %-4d %-7.2f %-7.2f %-7.2f ") % iter->id % iter->stats.get_ncpus() % iter->stats.get_user_percent()  % iter->stats.get_system_percent()  % iter->stats.get_idle_percent() ;
        RH_TRACE(this->_baseLog, ss.str()  );    
        ss.clear();
        ss.str("");
      }
    }

    return 0;
}


void
GPP_i::initializeNetworkMonitor()
{
    nic_facade.reset( new NicFacade(advanced.maximum_throughput_percentage,
                                    nic_interfaces,
                                    available_nic_interfaces,
                                    networkMonitor,
                                    nic_metrics,
                                    nic_allocation_status) );

    data_model.push_back( nic_facade );

    _allNicsThresholdMonitor = boost::make_shared<ThresholdMonitorSet>("nics", "NIC_THROUGHPUT");
    threshold_monitors.push_back(_allNicsThresholdMonitor);

    std::vector<std::string> nic_devices( nic_facade->get_devices() );
    std::vector<std::string> filtered_devices( nic_facade->get_filtered_devices() );
    for( size_t i=0; i<nic_devices.size(); ++i )
    {
        // Only use the filtered set of devices, which we can get away with
        // here because it cannot be updated after this method runs. In the
        // future, if the available NICs can be dynamically changed, all of the
        // possible NICs will need to be created here and selectively marked as
        // active/inactive (distinct from threshold enable/disable).
        const std::string& nic = nic_devices[i];
        if (std::find(filtered_devices.begin(), filtered_devices.end(), nic) == filtered_devices.end()) {
            RH_INFO(_baseLog, __FUNCTION__ << ": Skipping interface (" << nic << ")");
            continue;
        }

        RH_INFO(_baseLog, __FUNCTION__ << ": Adding interface (" << nic << ")");
        ThresholdMonitorPtr nic_m = boost::make_shared<FunctionThresholdMonitor>(nic, "NIC_THROUGHPUT", this, &GPP_i::_nicThresholdCheck);
        nic_m->add_listener(this, &GPP_i::_nicThresholdStateChanged);
        _allNicsThresholdMonitor->add_monitor(nic_m);
    }

}

void
GPP_i::initializeResourceMonitors()
{

  // add cpu utilization calculator
  RH_NL_INFO("GPP", " initialize CPU Monitor --- wl size " << wl_cpus.size());

  // request a system monitor for this GPP
  system_monitor.reset( new SystemMonitor( wl_cpus ) );

  // seed system monitor history
  for ( int i=0; i<5; i++ ) { 
    system_monitor->report(); 
    boost::this_thread::sleep( boost::posix_time::milliseconds( 200 ) );
  }

  data_model.push_back( system_monitor );

  // add system limits reader
  process_limits.reset( new ProcessLimits( getpid() ) );

  data_model.push_back( process_limits );

  //  observer to monitor when cpu idle pass threshold value
  _cpuIdleThresholdMonitor = boost::make_shared<FunctionThresholdMonitor>("cpu", "CPU_IDLE", this, &GPP_i::_cpuIdleThresholdCheck);
  _cpuIdleThresholdMonitor->add_listener(this, &GPP_i::_cpuIdleThresholdStateChanged);
  threshold_monitors.push_back(_cpuIdleThresholdMonitor);

  _loadAvgThresholdMonitor = boost::make_shared<FunctionThresholdMonitor>("cpu", "LOAD_AVG", this, &GPP_i::_loadAvgThresholdCheck);
  _loadAvgThresholdMonitor->add_listener(this, &GPP_i::_loadAvgThresholdStateChanged);
  threshold_monitors.push_back(_loadAvgThresholdMonitor);

  _freeMemThresholdMonitor = boost::make_shared<FunctionThresholdMonitor>("physical_ram", "MEMORY_FREE", this, &GPP_i::_freeMemThresholdCheck);
  _freeMemThresholdMonitor->add_listener(this, &GPP_i::_freeMemThresholdStateChanged);
  threshold_monitors.push_back(_freeMemThresholdMonitor);

  _threadThresholdMonitor = boost::make_shared<FunctionThresholdMonitor>("ulimit", "THREADS", this, &GPP_i::_threadThresholdCheck);
  _threadThresholdMonitor->add_listener(this, &GPP_i::_threadThresholdStateChanged);
  threshold_monitors.push_back(_threadThresholdMonitor);

  _fileThresholdMonitor = boost::make_shared<FunctionThresholdMonitor>("ulimit", "OPEN_FILES", this, &GPP_i::_fileThresholdCheck);
  _fileThresholdMonitor->add_listener(this, &GPP_i::_fileThresholdStateChanged);
  threshold_monitors.push_back(_fileThresholdMonitor);

  _shmThresholdMonitor = boost::make_shared<FunctionThresholdMonitor>("shm", "SHM_FREE", this, &GPP_i::_shmThresholdCheck);
  _shmThresholdMonitor->add_listener(this, &GPP_i::_shmThresholdStateChanged);
  threshold_monitors.push_back(_shmThresholdMonitor);
}

//
// Threshold policy and event handling
//

bool GPP_i::_cpuIdleThresholdCheck(ThresholdMonitor* monitor)
{
    double sys_idle = system_monitor->get_idle_percent();
    double sys_idle_avg = system_monitor->get_idle_average();
    RH_TRACE(_baseLog, "Update CPU idle threshold monitor, threshold=" << modified_thresholds.cpu_idle
             << " current=" << sys_idle << " average=" << sys_idle_avg);
    return (sys_idle < modified_thresholds.cpu_idle) && (sys_idle_avg < modified_thresholds.cpu_idle);
}

void GPP_i::_cpuIdleThresholdStateChanged(ThresholdMonitor* monitor)
{
    _sendThresholdMessage(monitor, system_monitor->get_idle_percent(), modified_thresholds.cpu_idle);
}

bool GPP_i::_loadAvgThresholdCheck(ThresholdMonitor* monitor)
{
    double load_avg = system_monitor->get_loadavg();
    RH_TRACE(_baseLog, "Update load average threshold monitor, threshold=" << modified_thresholds.load_avg
             << " measured=" << load_avg);
    return (load_avg > modified_thresholds.load_avg);
}

void GPP_i::_loadAvgThresholdStateChanged(ThresholdMonitor* monitor)
{
    _sendThresholdMessage(monitor, system_monitor->get_loadavg(), modified_thresholds.load_avg);
}

bool GPP_i::_freeMemThresholdCheck(ThresholdMonitor* monitor)
{
    int64_t mem_free = system_monitor->get_phys_free();
    RH_TRACE(_baseLog, "Update free memory threshold monitor, threshold=" << modified_thresholds.mem_free
             << " measured=" << mem_free);
    return (mem_free < modified_thresholds.mem_free);
}

void GPP_i::_freeMemThresholdStateChanged(ThresholdMonitor* monitor)
{
    _sendThresholdMessage(monitor, system_monitor->get_phys_free(), modified_thresholds.mem_free);
}

bool GPP_i::_threadThresholdCheck(ThresholdMonitor* monitor)
{
    int gpp_max_threads = gpp_limits.max_threads * modified_thresholds.threads;
    if (gpp_limits.max_threads != -1) {
        RH_TRACE(_baseLog, "Update thread threshold monitor (GPP), threshold=" << gpp_max_threads
                 << " measured=" << gpp_limits.current_threads);
        if (gpp_limits.current_threads > gpp_max_threads) {
            return true;
        }
    }
    int sys_max_threads = sys_limits.max_threads * modified_thresholds.threads;
    if (sys_limits.max_threads != -1) {
        RH_TRACE(_baseLog, "Update thread threshold monitor (system), threshold=" << sys_max_threads
                 << " measured=" << sys_limits.current_threads);
        if (sys_limits.current_threads > sys_max_threads) {
            return true;
        }
    }

    return false;
}

void GPP_i::_threadThresholdStateChanged(ThresholdMonitor* monitor)
{
    _sendThresholdMessage(monitor, gpp_limits.current_threads, gpp_limits.max_threads * modified_thresholds.threads);
}

bool GPP_i::_fileThresholdCheck(ThresholdMonitor* monitor)
{
    int gpp_max_open_files = gpp_limits.max_open_files * modified_thresholds.files_available;
    int sys_max_open_files = sys_limits.max_open_files * modified_thresholds.files_available;
    RH_TRACE(_baseLog, "Update file threshold monitor (GPP), threshold=" << gpp_max_open_files
             << " measured=" << gpp_limits.current_open_files);
    RH_TRACE(_baseLog, "Update file threshold monitor (system), threshold=" << sys_max_open_files
             << " measured=" << sys_limits.current_open_files);
    if (gpp_limits.current_open_files > gpp_max_open_files) {
        return true;
    } else if (sys_limits.current_open_files > sys_max_open_files) {
        return true;
    }
    return false;
}

void GPP_i::_fileThresholdStateChanged(ThresholdMonitor* monitor)
{
    _sendThresholdMessage(monitor, gpp_limits.current_open_files,
                          gpp_limits.max_open_files * modified_thresholds.files_available);
}

bool GPP_i::_shmThresholdCheck(ThresholdMonitor* monitor)
{
    RH_TRACE(_baseLog, "Update shared memory threshold monitor, threshold=" << modified_thresholds.shm_free
             << " measured=" << shmFree);
    return shmFree < modified_thresholds.shm_free;
}

void GPP_i::_shmThresholdStateChanged(ThresholdMonitor* monitor)
{
    _sendThresholdMessage(monitor, shmFree, modified_thresholds.shm_free);
}

bool GPP_i::_nicThresholdCheck(ThresholdMonitor* monitor)
{
    const std::string& nic = monitor->get_resource_id();
    float measured = nic_facade->get_throughput_by_device(nic);
    RH_TRACE(_baseLog, "Update NIC threshold monitor " << nic
             << " threshold=" << modified_thresholds.nic_usage
             << " measured=" << measured);
    return measured >= modified_thresholds.nic_usage;
}

void GPP_i::_nicThresholdStateChanged(ThresholdMonitor* monitor)
{
    std::string nic = monitor->get_resource_id();
    float measured = nic_facade->get_throughput_by_device(nic);
    _sendThresholdMessage(monitor, measured, modified_thresholds.nic_usage);
}

template <typename T1, typename T2>
void GPP_i::_sendThresholdMessage(ThresholdMonitor* monitor, const T1& measured, const T2& threshold)
{
    bool exceeded = monitor->is_threshold_exceeded();

    threshold_event_struct message;
    message.source_id = _identifier;
    message.resource_id = monitor->get_resource_id();
    message.threshold_class = monitor->get_threshold_class();
    if (exceeded) {
        message.type = enums::threshold_event::type::Threshold_Exceeded;
    } else {
        message.type = enums::threshold_event::type::Threshold_Not_Exceeded;
    }
    if (monitor->is_enabled()) {
        message.threshold_value = boost::lexical_cast<std::string>(threshold);
    } else {
        message.threshold_value = "<disabled>";
    }
    message.measured_value = boost::lexical_cast<std::string>(measured);

    std::stringstream sstr;
    sstr << message.threshold_class << " threshold ";
    if (!exceeded) {
        sstr << "not ";
    }
    sstr << "exceeded "
         << "(resource_id=" << message.resource_id
         << " threshold_value=" << message.threshold_value
         << " measured_value=" << message.measured_value << ")";
    message.message = sstr.str();

    message.timestamp = time(NULL);

    send_threshold_event(message);
}

//
//  Device LifeCycle API
// 

void GPP_i::initialize() throw (CF::LifeCycle::InitializeError, CORBA::SystemException)
{

  if (processor_name.empty()) {
    RH_FATAL(_baseLog, "Property processor_name (DCE:fefb9c66-d14a-438d-ad59-2cfd1adb272b) must have a value. Please set this property value either in GPP.prf.xml or in the node's DeviceManager.dcd.xml file");
    throw CF::LifeCycle::InitializeError(1);
  }

  RH_NL_INFO("GPP", "initialize()");
  //
  // subscribe to ODM_Channel for receiving state changes from Application objects
  //
  try {
    mymgr = redhawk::events::Manager::GetManager(this);
    odm_consumer = mymgr->Subscriber("ODM_Channel");
    odm_consumer->setDataArrivedListener(this, &GPP_i::process_ODM);
  } catch ( ... ) {
    RH_WARN(this->_baseLog, "Unable to register with EventChannelManager, disabling domain event notification.");
  }

  //
  // check if componentOutputLog is set.. if so enable redirected io operations
  //
  if ( componentOutputLog != "" ) {
    _componentOutputLog =__ExpandEnvVars(componentOutputLog);
    _handle_io_redirects = true;
    RH_INFO(this->_baseLog, "Turning on Component Output Redirection file: " << _componentOutputLog );
  }
  else {
    RH_INFO(this->_baseLog, "Component Output Redirection is DISABLED." << componentOutputLog );
  }

  //
  // Setup affinity settings context
  //
  _affinity_changed( NULL, &affinity );

  //
  // setup execution partitions for performing socket based deployments, we need to know the current black list
  //
  _setupExecPartitions( bl_cpus );

  //
  // Get units for conversion operations
  //
  std::string munit("MB");
  PropertyInterface *p = getPropertyFromId("thresholds::mem_free");
  if (p) {
    munit = p->units;  
  }
  thresh_mem_free_units=conv_units(munit);

  munit="MB";
  p = getPropertyFromId("memFree");
  if (p) {
    munit = p->units;  
  }
  mem_free_units=conv_units(munit);

  munit="MB";
  p = getPropertyFromId("memCapacity");
  if (p) {
    munit = p->units;  
  }
  mem_cap_units=conv_units(munit);
    
  //
  // setup the data model for the GPP 
  //
  threshold_monitors.clear();
  initializeResourceMonitors();
  initializeNetworkMonitor();

  std::for_each( data_model.begin(), data_model.end(), boost::bind( &Updateable::update, _1 ) );
  std::for_each( execPartitions.begin(), execPartitions.end(), boost::bind( &Updateable::update, _1 ) );

  //
  // get monitored system values...
  //
  const SystemMonitor::Report &rpt = system_monitor->getReport();

  //
  // load average attributes
  //
  loadTotal = loadCapacityPerCore * (float)processor_cores;
  loadCapacity = loadTotal;
  if (!thresholds.ignore && thresholds.load_avg >= 0.0) {
      loadCapacity *= thresholds.load_avg / 100.0;
  }
  loadFree = loadCapacity;
  idle_capacity_modifier = 100.0 * reserved_capacity_per_component/((float)processor_cores);
 
  //
  // memory capacity tracking attributes
  //
  memInitFree=rpt.physical_memory_free;  // assume current state to be total available
  int64_t init_mem_free = (int64_t) memInitFree;
  if (!thresholds.ignore && thresholds.mem_free >= 0) {
      memInitCapacityPercent  =  (double)( (int64_t)init_mem_free - (int64_t)(thresholds.mem_free*thresh_mem_free_units) )/ (double)init_mem_free;
      if (memInitCapacityPercent < 0.0) {
          memInitCapacityPercent = 100.0;
      }
  } else {
      memInitCapacityPercent = 100.0;
  }
  memFree =  init_mem_free / mem_free_units;
  memCapacity = ((int64_t)( init_mem_free * memInitCapacityPercent)) / mem_cap_units ;
  memCapacityThreshold = memCapacity;

  shmFree = redhawk::shm::getSystemFreeMemory() / MB_TO_BYTES;

  //
  // set initial modified thresholds
  //
  modified_thresholds = thresholds;
  thresholds_changed(thresholds, thresholds);

  loadAverage.onemin = rpt.load.one_min;
  loadAverage.fivemin = rpt.load.five_min;
  loadAverage.fifteenmin = rpt.load.fifteen_min;

  //
  // transfer limits to properties
  //
  const Limits::Contents &sys_rpt = rpt.sys_limits;
  sys_limits.current_threads = sys_rpt.threads;
  sys_limits.max_threads = sys_rpt.threads_limit;
  sys_limits.current_open_files = sys_rpt.files;
  sys_limits.max_open_files = sys_rpt.files_limit;

  const Limits::Contents &pid_rpt = process_limits->get();
  gpp_limits.current_threads = pid_rpt.threads;
  gpp_limits.max_threads = pid_rpt.threads_limit;
  gpp_limits.current_open_files = pid_rpt.files;
  gpp_limits.max_open_files = pid_rpt.files_limit;

  //
  // setup mcast interface allocations, used by older systems -- need to deprecate
  //
  mcastnicIngressThresholdValue = mcastnicIngressTotal * ( mcastnicThreshold / 100.0) ;
  mcastnicIngressCapacity = mcastnicIngressThresholdValue;
  mcastnicIngressFree = mcastnicIngressCapacity;
  mcastnicEgressThresholdValue = mcastnicEgressTotal * ( mcastnicThreshold / 100.0) ;
  mcastnicEgressCapacity = mcastnicEgressThresholdValue;
  mcastnicEgressFree = mcastnicEgressCapacity;
  
  // grab nic_metrics for the specified interface
  _set_vlan_property();

  // use by service function to mark update time for monitors, states, and stats
  time_mark = boost::posix_time::microsec_clock::local_time();

  // start capturing IO redirections
  _redirectedIO.start();

  GPP_base::start();
  GPP_base::initialize();

}


void GPP_i::thresholds_changed(const thresholds_struct& ov, const thresholds_struct& nv)
{
    WriteLock wlock(monitorLock);
    if (nv.ignore || (nv.mem_free < 0)) {
        RH_DEBUG(_baseLog, __FUNCTION__ << " THRESHOLDS.MEM_FREE DISABLED");
        modified_thresholds.mem_free = 0;
        _freeMemThresholdMonitor->disable();
    } else {
        if (ov.mem_free != nv.mem_free) {
            RH_DEBUG(_baseLog, __FUNCTION__ << " THRESHOLDS.MEM_FREE CHANGED  old/new " << ov.mem_free << "/" << nv.mem_free);
        }
        int64_t init_mem_free = (int64_t) memInitFree;
        // type cast required for correct calc on 32bit os
        memInitCapacityPercent  =  (double)( (int64_t)init_mem_free - (int64_t)(nv.mem_free*thresh_mem_free_units) )/ (double) init_mem_free;
        if ( memInitCapacityPercent < 0.0 )  memInitCapacityPercent = 100.0;
        memCapacity = ((int64_t)( init_mem_free * memInitCapacityPercent) ) / mem_cap_units;
        memCapacityThreshold = memCapacity;
        modified_thresholds.mem_free = nv.mem_free*thresh_mem_free_units;
        _freeMemThresholdMonitor->enable();
    }

    if (nv.ignore || (nv.load_avg < 0.0)) {
        RH_DEBUG(_baseLog, __FUNCTION__ << " THRESHOLDS.LOAD_AVG DISABLED");
        modified_thresholds.load_avg = loadTotal;
        _loadAvgThresholdMonitor->disable();
    } else {
        if (fabs(ov.load_avg - nv.load_avg) >= std::numeric_limits<double>::epsilon()) {
            RH_DEBUG(_baseLog, __FUNCTION__ << " THRESHOLDS.LOAD_AVG CHANGED  old/new " << ov.load_avg << "/" << nv.load_avg);
        }
        loadCapacity = loadTotal * ((double)nv.load_avg / 100.0);
        loadFree = loadCapacity;
        modified_thresholds.load_avg = loadTotal * ( (double)nv.load_avg / 100.0);
        _loadAvgThresholdMonitor->enable();
    }

    if (nv.ignore || (nv.cpu_idle < 0.0)) {
        RH_DEBUG(_baseLog, __FUNCTION__ << " THRESHOLDS.CPU_IDLE DISABLED");
        modified_thresholds.cpu_idle = 0.0;
        _cpuIdleThresholdMonitor->disable();
    } else {
        if (fabs(ov.cpu_idle - nv.cpu_idle) >= std::numeric_limits<double>::epsilon()) {
            RH_DEBUG(_baseLog, __FUNCTION__ << " THRESHOLDS.CPU_IDLE CHANGED  old/new " << ov.cpu_idle << "/" << nv.cpu_idle);
        }
        modified_thresholds.cpu_idle = nv.cpu_idle;
        _cpuIdleThresholdMonitor->enable();
    }

    if (nv.ignore || (nv.nic_usage < 0)) {
        RH_DEBUG(_baseLog, __FUNCTION__ << " THRESHOLDS.NIC_USAGE DISABLED");
        _allNicsThresholdMonitor->disable();
    } else {
        if (ov.nic_usage != nv.nic_usage) {
            RH_DEBUG(_baseLog, __FUNCTION__ << " THRESHOLDS.NIC_USAGE CHANGED  old/new " << ov.nic_usage << "/" << nv.nic_usage);
        }
        modified_thresholds.nic_usage = nv.nic_usage;
        _allNicsThresholdMonitor->enable();
    }

    if (nv.ignore || (nv.shm_free < 0)) {
        RH_DEBUG(_baseLog, __FUNCTION__ << " THRESHOLDS.SHM_FREE DISABLED");
        _shmThresholdMonitor->disable();
    } else {
        if (ov.shm_free != nv.shm_free) {
            RH_DEBUG(_baseLog, __FUNCTION__ << " THRESHOLDS.SHM_FREE CHANGED  old/new "
                      << ov.shm_free << "/" << nv.shm_free);
        }
        modified_thresholds.shm_free = nv.shm_free;
        _shmThresholdMonitor->enable();
    }

    if (nv.ignore || (nv.threads < 0.0)) {
        RH_DEBUG(_baseLog, __FUNCTION__ << " THRESHOLDS.THREADS DISABLED");
        _threadThresholdMonitor->disable();
    } else {
        if (fabs(ov.threads - nv.threads) >= std::numeric_limits<double>::epsilon()) {
            RH_DEBUG(_baseLog, __FUNCTION__ << " THRESHOLDS.THREADS CHANGED  old/new " << ov.threads << "/" << nv.threads);
        }
        modified_thresholds.threads = 1.0 - (nv.threads * .01);
        _threadThresholdMonitor->enable();
    }

    if (nv.ignore || (nv.files_available < 0.0)) {
        RH_DEBUG(_baseLog, __FUNCTION__ << " THRESHOLDS.FILES_AVAILABLE DISABLED");
        _fileThresholdMonitor->disable();
    } else {
        if (fabs(ov.files_available - nv.files_available) >= std::numeric_limits<double>::epsilon()) {
            RH_DEBUG(_baseLog, __FUNCTION__ << " THRESHOLDS.FILES_AVAILABLE CHANGED  old/new " << ov.files_available << "/" << nv.files_available);
        }
        modified_thresholds.files_available = 1.0 - (nv.files_available * .01);
        _fileThresholdMonitor->enable();
    }
}

void GPP_i::releaseObject() throw (CORBA::SystemException, CF::LifeCycle::ReleaseError) {
  _signalThread.stop();
  _signalThread.release();
  _handle_io_redirects = false;
  _redirectedIO.stop();
  _redirectedIO.release();
  if ( odm_consumer ) odm_consumer.reset();
  GPP_base::releaseObject();
}


CF::ExecutableDevice::ProcessID_Type GPP_i::execute (const char* name, const CF::Properties& options, const CF::Properties& parameters)
    throw (CORBA::SystemException, CF::Device::InvalidState, CF::ExecutableDevice::InvalidFunction, 
           CF::ExecutableDevice::InvalidParameters, CF::ExecutableDevice::InvalidOptions, 
           CF::InvalidFileName, CF::ExecutableDevice::ExecuteFail)
{
    boost::recursive_mutex::scoped_lock lock;
    try
    {
        lock = boost::recursive_mutex::scoped_lock(load_execute_lock);
    }
    catch( const boost::thread_resource_error& e )
    {
        std::stringstream errstr;
        errstr << "Error acquiring lock (errno=" << e.native_error() << " msg=\"" << e.what() << "\")";
        RH_ERROR(this->_baseLog, __FUNCTION__ << ": " << errstr.str() );
        throw CF::Device::InvalidState(errstr.str().c_str());
    }

    redhawk::PropertyMap tmp_params(parameters);
    float reservation_value = -1;
    if (tmp_params.find("RH::GPP::MODIFIED_CPU_RESERVATION_VALUE") != tmp_params.end()) {
        try {
            reservation_value = tmp_params["RH::GPP::MODIFIED_CPU_RESERVATION_VALUE"].toFloat();
        } catch (const std::exception&) {
            reservation_value = -1;
        }
        tmp_params.erase("RH::GPP::MODIFIED_CPU_RESERVATION_VALUE");
    }

    std::string component_id = tmp_params.get("COMPONENT_IDENTIFIER", std::string()).toString();
    if (applicationReservations.find(component_id) != applicationReservations.end()) {
        applicationReservations.erase(component_id);
    }

    std::string naming_context_ior = tmp_params.get("NAMING_CONTEXT_IOR", std::string()).toString();
    std::string app_id;
    if (!naming_context_ior.empty()) {
        CORBA::Object_var obj = ossie::corba::Orb()->string_to_object(naming_context_ior.c_str());
        if (CORBA::is_nil(obj)) {
            RH_WARN(_baseLog, "Invalid application registrar IOR");
        } else {
            CF::ApplicationRegistrar_var app_registrar = CF::ApplicationRegistrar::_narrow(obj);
            if (CORBA::is_nil(app_registrar)) {
                RH_WARN(_baseLog, "Invalid application registrar IOR");
            } else {
                CF::Application_var application = app_registrar->app();
                if (!CORBA::is_nil(application)) {
                    app_id = ossie::corba::returnString(application->identifier());
                }
            }
        }
    }

    std::vector<std::string> prepend_args;
    if (useScreen) {
        std::string ld_lib_path(getenv("LD_LIBRARY_PATH"));
        setenv("GPP_LD_LIBRARY_PATH",ld_lib_path.c_str(),1);
        
        DIR *dir;
        struct dirent *ent;
        std::string search_bin("screen");
    
        std::string path(getenv( "PATH" ));
        bool foundScreen = false;
        while (not foundScreen) {
            size_t sub = path.find(":");
            if (path.size() == 0)
                break;
            std::string substr = path.substr(0, sub);
            if ((dir = opendir (substr.c_str())) != NULL) {
                while ((ent = readdir (dir)) != NULL) {
                    std::string filename(ent->d_name);
                    if (filename == search_bin) {
                        prepend_args.push_back(substr+"/"+filename);
                        foundScreen = true;
                        break;
                    }
                }
                closedir (dir);
            }
            if (sub != std::string::npos)
                path = path.substr(sub+1, std::string::npos);
            else
                path.clear();
        }
        prepend_args.push_back("-D");
        prepend_args.push_back("-m");
        prepend_args.push_back("-c");
        prepend_args.push_back(binary_location+"gpp.screenrc");

        std::string name_binding = tmp_params.get("NAME_BINDING", std::string()).toString();
        if ((not component_id.empty()) and (not name_binding.empty())) {
            if (component_id.find("DCE:") != std::string::npos) {
                component_id = component_id.substr(4, std::string::npos);
            }
            size_t waveform_boundary = component_id.find(":");
            std::string component_inst_id, waveform_name;
            component_inst_id = component_id.substr(0, waveform_boundary);
            waveform_name = component_id.substr(waveform_boundary+1, std::string::npos);
            prepend_args.push_back("-S");
            prepend_args.push_back(waveform_name+"."+name_binding);
            prepend_args.push_back("-t");
            prepend_args.push_back(waveform_name+"."+name_binding);
        }
    }
    CF::ExecutableDevice::ProcessID_Type ret_pid;
    try {
        ret_pid = do_execute(name, options, tmp_params, prepend_args);
        addProcess(ret_pid, app_id, component_id, reservation_value);
    } catch ( ... ) {
        throw;
    }
    return ret_pid;
}



/* execute *****************************************************************
    - executes a process on the device
************************************************************************* */
CF::ExecutableDevice::ProcessID_Type GPP_i::do_execute (const char* name, const CF::Properties& options, const CF::Properties& parameters, const std::vector<std::string> prepend_args) throw (CORBA::SystemException, CF::Device::InvalidState, CF::ExecutableDevice::InvalidFunction, CF::ExecutableDevice::InvalidParameters, CF::ExecutableDevice::InvalidOptions, CF::InvalidFileName, CF::ExecutableDevice::ExecuteFail)
{
    CF::Properties invalidOptions;
    std::string path;
    char* tmp;

    // throw and error if name does not begin with a /
    if (strncmp(name, "/", 1) != 0)
        throw CF::InvalidFileName(CF::CF_EINVAL, "Filename must be absolute");
    if (isLocked())
        throw CF::Device::InvalidState("System is locked down");
    if (isDisabled())
        throw CF::Device::InvalidState("System is disabled");

    //process options and throw InvalidOptions errors if they are not ULong
    for (CORBA::ULong i = 0; i < options.length(); ++i) {
        if (options[i].id == CF::ExecutableDevice::PRIORITY_ID) {
            CORBA::TypeCode_var atype = options[i].value.type();
            if (atype->kind() != CORBA::tk_ulong) {
                invalidOptions.length(invalidOptions.length() + 1);
                invalidOptions[invalidOptions.length() - 1].id = options[i].id;
                invalidOptions[invalidOptions.length() - 1].value
                        = options[i].value;
            } else
                RH_WARN(this->_baseLog, "Received a PRIORITY_ID execute option...ignoring.")
            }
        if (options[i].id == CF::ExecutableDevice::STACK_SIZE_ID) {
            CORBA::TypeCode_var atype = options[i].value.type();
            if (atype->kind() != CORBA::tk_ulong) {
                invalidOptions.length(invalidOptions.length() + 1);
                invalidOptions[invalidOptions.length() - 1].id = options[i].id;
                invalidOptions[invalidOptions.length() - 1].value
                        = options[i].value;
            } else
                RH_WARN(this->_baseLog, "Received a STACK_SIZE_ID execute option...ignoring.")
            }
    }

    if (invalidOptions.length() > 0) {
        throw CF::ExecutableDevice::InvalidOptions(invalidOptions);
    }

    // retrieve current working directory
    if (this->cacheDirectory.empty()) {
        tmp = getcwd(NULL, 200);
        if (tmp != NULL) {
            path = std::string(tmp);
            free(tmp);
        }
    } else {
        path = this->cacheDirectory;
        if (!path.compare(path.length()-1, 1, "/")) {
            path = path.erase(path.length()-1);
        }
    }

    // append relative path of the executable
    path.append(name);

    // check file existence
    if (access(path.c_str(), F_OK) == -1) {
        std::string errMsg = "File could not be found " + path;
        throw CF::InvalidFileName(CF::CF_EINVAL,
                CORBA::string_dup(errMsg.c_str()));
    }

    // change permissions to 7--
    if (chmod(path.c_str(), S_IRWXU) != 0) {
        RH_ERROR(this->_baseLog, "Unable to change permission on executable");
        throw CF::ExecutableDevice::ExecuteFail(CF::CF_EACCES,
                "Unable to change permission on executable");
    }

    // assemble argument list
    std::vector<std::string> args = prepend_args;
    if (getenv("VALGRIND")) {
        char* valgrind = getenv("VALGRIND");
        if (strlen(valgrind) == 0) {
            // Assume that valgrind is somewhere on the path
            args.push_back("valgrind");
        } else {
            // Environment variable is path to valgrind executable
            args.push_back(valgrind);
        }
        // Put the log file in the cache next to the component entrypoint;
        // include the pid to avoid clobbering existing files
        std::string logFile = "--log-file=";
        char* name_temp = strdup(path.c_str());
        logFile += dirname(name_temp);
        free(name_temp);
        logFile += "/valgrind.%p.log";
        args.push_back(logFile);
    }
    args.push_back(path);

    RH_DEBUG(this->_baseLog, "Building param list for process " << path);
    for (CORBA::ULong i = 0; i < parameters.length(); ++i) {
        RH_DEBUG(this->_baseLog, "id=" << ossie::corba::returnString(parameters[i].id) << " value=" << ossie::any_to_string(parameters[i].value));
        CORBA::TypeCode_var atype = parameters[i].value.type();
        args.push_back(ossie::corba::returnString(parameters[i].id));
        args.push_back(ossie::any_to_string(parameters[i].value));
    }

    RH_DEBUG(this->_baseLog, "Forking process " << path);

    std::vector<char*> argv(args.size() + 1, NULL);
    for (std::size_t i = 0; i < args.size(); ++i) {
        // const_cast because execv does not modify values in argv[].
        // See:  http://pubs.opengroup.org/onlinepubs/9699919799/functions/exec.html
        argv[i] = const_cast<char*> (args[i].c_str());
    }

    rh_logger::LevelPtr  lvl = GPP_i::__logger->getLevel();

    // setup to capture stdout and stderr from children.
    int comp_fd[2];
    std::string rfname;
    if ( _handle_io_redirects ) {
	rfname=__ExpandEnvVars(componentOutputLog);
	rfname=__ExpandProperties(rfname, parameters );
	if ( rfname == _componentOutputLog ) {
	    RH_TRACE(this->_baseLog, "Redirect to common file for :" << path << " file: " << rfname );
	    if ( pipe( comp_fd ) == -1 ) {
		RH_ERROR(this->_baseLog, "Failure to create redirected IO for:" << path);
		throw CF::ExecutableDevice::ExecuteFail(CF::CF_EPERM, "Failure to create redirected IO for component");
	    }
      
	    if ( fcntl( comp_fd[0], F_SETFD, FD_CLOEXEC ) == -1 ) {
		RH_ERROR(this->_baseLog, "Failure to support redirected IO for:" << path);
		throw CF::ExecutableDevice::ExecuteFail(CF::CF_EPERM, "Failure to support redirected IO for component");
	    }
	}
	else { // per process logging
	    RH_TRACE(this->_baseLog, "Redirect per process for :" << path << " file: " << rfname );
	    comp_fd[0]=-1;
	    comp_fd[1] = open(rfname.c_str(), O_RDWR | O_CREAT | O_APPEND , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH  );
	    if ( comp_fd[1] == -1 ) {
		RH_ERROR(this->_baseLog, "Failure to create redirected IO for:" << path);
		throw CF::ExecutableDevice::ExecuteFail(CF::CF_EPERM, "Failure to create redirected IO for component");
	    }
	}
    }

    {
        SCOPED_LOCK(_forkLock);
        boost::system_time const timeout=boost::get_system_time();
        if ( _forkReady.timed_wait( __slock, timeout, check_fork_msg(*this,FORK_WAIT)) ) {
            boost::system_time const timeout=boost::get_system_time() + boost::posix_time::milliseconds(1500);
            if ( !_forkReady.timed_wait( __slock, timeout, check_fork_msg(*this,FORK_GO)) ) {
                throw CF::ExecutableDevice::ExecuteFail(CF::CF_EPERM, "Failure when requesting fork operation");                    
            }
        }
    }
            
    // fork child process
    int pid = fork();

    if (pid == 0) {

      int num_retries = 5;
      int returnval = 0;

      //
      // log4cxx will cause dead locks between fork and execv, use the stdout logger object 
      // for all logging
      //
      rh_logger::LoggerPtr __logger = rh_logger::StdOutLogger::getRootLogger();
      __logger->setLevel(lvl);

      // set affinity logger method so we do not use log4cxx during affinity processing routine
      redhawk::affinity::set_affinity_logger( __logger ) ;
      RH_DEBUG(__logger, " Calling set resource affinity....exec:" << name << " options=" << options.length());

      // set affinity preference before exec
      try {
        RH_DEBUG(__logger, " Calling set resource affinity....exec:" << name << " options=" << options.length());
        set_resource_affinity( options, getpid(), name );
      }
      catch( redhawk::affinity::AffinityFailed &ex ) {
        RH_WARN(__logger, "Unable to satisfy affinity request for: " << name << " Reason: " << ex.what() );
        errno=EPERM<<2;
        returnval=-1;
        ossie::corba::OrbShutdown(true);
        exit(returnval);
      }
      catch( ... ) {
        RH_WARN(__logger,  "Unhandled exception during affinity processing for resource: " << name  );
        ossie::corba::OrbShutdown(true);
        exit(returnval);
      }

      // reset mutex in child...
      pthread_mutex_init(load_execute_lock.native_handle(),0);
      
      // set the forked component as the process group leader
      if ( setpgid(getpid(), 0) != 0 ) {
          int e=errno;
          RH_ERROR(__logger,  "SETPGID failed for pid " << getpid() << " errno: " << e );
      }

      // apply io redirection for stdout and stderr
      if ( _handle_io_redirects ) {

        if( dup2(comp_fd[1],STDERR_FILENO) ==-1 ) {
            RH_ERROR(__logger,  "Failure to dup2 stderr for resource: " << name  );
            ossie::corba::OrbShutdown(true);
            exit(-1);
        } 

        if( dup2(comp_fd[1],STDOUT_FILENO) ==-1 ) {
            RH_ERROR(__logger,  "Failure to dup2 stdout for resource: " << name  );
            ossie::corba::OrbShutdown(true);
            exit(-1);
        } 

        if ( comp_fd[0] != -1 ) close(comp_fd[0]);
        close(comp_fd[1]);
      }

      
      // Run executable
      while(true)
        {
          if (strcmp(argv[0], "valgrind") == 0) {
              // Find valgrind in the path
              returnval = execvp(argv[0], &argv[0]);
          } else {
              returnval = execv(argv[0], &argv[0]);
          }

          num_retries--;
          if( num_retries <= 0 || errno!=ETXTBSY)
                break;

          // Only retry on "text file busy" error
          RH_WARN(__logger, "execv() failed, retrying... (cmd=" << path << " msg=\"" << strerror(errno) << "\" retries=" << num_retries << ")");
          usleep(100000);
        }

        if( returnval ) {
            RH_ERROR(__logger, "Error when calling execv() (cmd=" << path << " errno=" << errno << " msg=\"" << strerror(errno) << "\")");
            ossie::corba::OrbShutdown(true);
        }

        RH_ERROR(__logger, "Exiting FAILED subprocess:" << returnval );
        exit(returnval);
    }
    else if (pid < 0 ){
        RH_ERROR(this->_baseLog, "Error forking child process (errno: " << errno << " msg=\"" << strerror(errno) << "\")" );
        switch (errno) {
            case E2BIG:
                throw CF::ExecutableDevice::ExecuteFail(CF::CF_E2BIG,
                        "Argument list too long");
            case EACCES:
                throw CF::ExecutableDevice::ExecuteFail(CF::CF_EACCES,
                        "Permission denied");
            case ENAMETOOLONG:
                throw CF::ExecutableDevice::ExecuteFail(CF::CF_ENAMETOOLONG,
                        "File name too long");
            case ENOENT:
                throw CF::ExecutableDevice::ExecuteFail(CF::CF_ENOENT,
                        "No such file or directory");
            case ENOEXEC:
                throw CF::ExecutableDevice::ExecuteFail(CF::CF_ENOEXEC,
                        "Exec format error");
            case ENOMEM:
                throw CF::ExecutableDevice::ExecuteFail(CF::CF_ENOMEM,
                        "Out of memory");
            case ENOTDIR:
                throw CF::ExecutableDevice::ExecuteFail(CF::CF_ENOTDIR,
                        "Not a directory");
            case EPERM:
                throw CF::ExecutableDevice::ExecuteFail(CF::CF_EPERM,
                        "Operation not permitted");
            default:
                throw CF::ExecutableDevice::ExecuteFail(CF::CF_NOTSET,
                        "ERROR ON FORK");
        }
    }

    if ( _handle_io_redirects ) {
      close(comp_fd[1]);
      RH_TRACE(this->_baseLog, "Adding Task for IO Redirection PID:" << pid << " : stdout "<< comp_fd[0] );
      WriteLock wlock(fdsLock);
      if ( comp_fd[0] != -1 ) {
	  ProcRedirectPtr rd = ProcRedirectPtr( new proc_redirect( rfname, pid, comp_fd[0] ) );
	  redirectedFds.push_front( rd  );
	  epoll_event event;
	  event.data.ptr = (void*)(rd.get());
	  event.events = EPOLLIN;
	  int ret __attribute__((unused)) = epoll_ctl (epfd, EPOLL_CTL_ADD, comp_fd[0], &event);
      }
    }


    RH_DEBUG(this->_baseLog, "Execute success: name:" << name << " : "<< path);

    return pid;
}


void GPP_i::terminate (CF::ExecutableDevice::ProcessID_Type processId) throw (CORBA::SystemException, CF::ExecutableDevice::InvalidProcess, CF::Device::InvalidState)
{
    RH_TRACE(this->_baseLog, " Terminate request, processID: " << processId);
    try {
      component_description comp = markPidTerminated( processId );
      if (!comp.reaped) {
        ExecutableDevice_impl::terminate(processId);
      }
    }
    catch(...){
    }
    removeProcess(processId);
}


bool GPP_i::_component_cleanup(const int pid, const int status)
{
    component_description comp;
    try {
        comp = markPidReaped(pid);
    } catch (...) {
        // pass.. could be a pid from and popen or system commands..
        return false;
    }

    if (!comp.terminated) {
        // release of component can exit process before terminate is called
        if (WIFEXITED(status) && (WEXITSTATUS(status) != 0)) {
            RH_ERROR(this->_baseLog, "Unexpected component exit with non-zero status " << WEXITSTATUS(status)
                      << ", App/Identifier/Process: " << comp.appName << "/" << comp.identifier << "/" << pid);
            sendChildNotification(comp.identifier, comp.appName);
        } else if (WIFSIGNALED(status)) {
            RH_ERROR(this->_baseLog, "Unexepected component termination with signal " << WTERMSIG(status)
                      << ", App/Identifier/Process: " << comp.appName << "/" << comp.identifier << "/" << pid);
            sendChildNotification(comp.identifier, comp.appName);
        }
    }

    removeProcess(pid);

    // Ensure that if the process created a shared memory heap, it gets removed
    // to avoid wasting shared memory
    _cleanupProcessShm(pid);

    return true;
}


//
//
//  Executable/Device method overrides...
//
//

void GPP_i::updateUsageState()
{
    // allow for global ignore of thresholds
    if ( thresholds.ignore == true  ) {
        _resetBusyReason();
        RH_TRACE(_baseLog, "Ignoring threshold checks ");
        if (getPids().size() == 0) {
            RH_TRACE(_baseLog, "Usage State IDLE (trigger) pids === 0...  ");
            setUsageState(CF::Device::IDLE);
        }
        else {
            RH_TRACE(_baseLog, "Usage State ACTIVE.....  ");
            setUsageState(CF::Device::ACTIVE);
        }
        return;
    }

  double sys_idle = system_monitor->get_idle_percent();
  double sys_idle_avg = system_monitor->get_idle_average();
  double sys_load = system_monitor->get_loadavg();
  int64_t mem_free = system_monitor->get_phys_free();
  
  // get reservation state
  double max_allowable_load =  utilization[0].maximum;
  double subscribed =  utilization[0].subscribed;

  uint64_t all_nics_threshold = 0;
  double all_nics_throughput = 0.0;

  ReadLock rlock(monitorLock);

  std::stringstream nic_message;
  std::vector<std::string> filtered_nics = nic_facade->get_filtered_devices();
  for (size_t index = 0; index < filtered_nics.size(); ++index) {
      const std::string& nic = filtered_nics[index];
      double throughput = nic_facade->get_throughput_by_device(nic);
      nic_message << "   Nic: " << nic
                  << " threshold=" << modified_thresholds.nic_usage
                  << " measured=" << throughput << std::endl;

      all_nics_threshold += modified_thresholds.nic_usage;
      all_nics_throughput += throughput;
  }

  RH_TRACE(_baseLog,  "USAGE STATE: " << std::endl <<
            " CPU:  threshold " <<  modified_thresholds.cpu_idle << " Actual: " << sys_idle  << " Avg: " << sys_idle_avg  << std::endl <<
            " MEM:  threshold " <<  modified_thresholds.mem_free << " Actual: " << mem_free  << std::endl <<
            " LOAD: threshold " <<  modified_thresholds.load_avg << " Actual: " << sys_load  << std::endl <<
            " RESRV: threshold " <<  max_allowable_load << " Actual: " << subscribed  << std::endl <<
            " Ingress threshold: " << mcastnicIngressThresholdValue << " capacity: " <<  mcastnicIngressCapacity  << std::endl <<
            " Egress threshold: " << mcastnicEgressThresholdValue << " capacity: " <<  mcastnicEgressCapacity  << std::endl  <<
            " Threads threshold: " << gpp_limits.max_threads << " Actual: " << gpp_limits.current_threads << std::endl <<
            " NIC: " << std::endl << nic_message.str()
            );
  
  if (_cpuIdleThresholdMonitor->is_threshold_exceeded()) {
      std::ostringstream oss;
      oss << "Threshold: " <<  modified_thresholds.cpu_idle << " Actual/Average: " << sys_idle << "/" << sys_idle_avg ;
      _setBusyReason("CPU IDLE", oss.str());
  }
  else if (_freeMemThresholdMonitor->is_threshold_exceeded()) {
      std::ostringstream oss;
      oss << "Threshold: " <<  modified_thresholds.mem_free << " Actual: " << mem_free;
      _setBusyReason("FREE MEMORY", oss.str());
  }
  else if (_loadAvgThresholdMonitor->is_threshold_exceeded()) {
      std::ostringstream oss;
      oss << "Threshold: " <<  modified_thresholds.load_avg << " Actual: " << sys_load;
      _setBusyReason("LOAD AVG", oss.str());
  }
  else if ((reserved_capacity_per_component != 0) && (subscribed > max_allowable_load)) {
      std::ostringstream oss;
      oss << "Threshold: " << max_allowable_load << " Actual(subscribed) " << subscribed;
      _setBusyReason("RESERVATION CAPACITY", oss.str());
  }
  else if (_shmThresholdMonitor->is_threshold_exceeded()) {
      std::ostringstream oss;
      oss << "Threshold: " << modified_thresholds.shm_free << " Actual: " << shmFree;
      _setBusyReason("SHARED MEMORY", oss.str());
  }
  else if (_allNicsThresholdMonitor->is_threshold_exceeded()) {
      std::ostringstream oss;
      oss << "Threshold (cumulative) : " << all_nics_threshold << " Actual (cumulative) : " << all_nics_throughput;
      _setBusyReason("NIC USAGE ", oss.str());
  }
  else if (_threadThresholdMonitor->is_threshold_exceeded()) {
      std::ostringstream oss;
      oss << "Threshold: " << gpp_limits.max_threads << " Actual: " << gpp_limits.current_threads;
      _setBusyReason("ULIMIT (MAX_THREADS)", oss.str());
  }
  else if (_fileThresholdMonitor->is_threshold_exceeded()) {
      std::ostringstream oss;
      oss << "Threshold: " << gpp_limits.max_open_files << " Actual: " << gpp_limits.current_open_files;
      _setBusyReason("ULIMIT (MAX_FILES)", oss.str());
  }
  else if (_pluginBusy) {
      // avoid going into idle or active
  }
  else if (getPids().size() == 0) {
    RH_TRACE(_baseLog, "Usage State IDLE (trigger) pids === 0...  ");
    _resetBusyReason();
    setUsageState(CF::Device::IDLE);
  }
  else {
    RH_TRACE(_baseLog, "Usage State ACTIVE.....  ");
    _resetBusyReason();
    setUsageState(CF::Device::ACTIVE);
  }
}


void GPP_i::_resetBusyReason() {
    _busy.mark = _busy.timestamp = boost::posix_time::not_a_date_time;
    _busy.resource.clear();
    busy_reason.clear();
}

void GPP_i::_setBusyReason(const std::string& resource, const std::string& message)
{
    if (resource != _busy.resource) {
        RH_INFO(_baseLog, "GPP BUSY, REASON: " << resource << " " << message);
        _busy.timestamp = boost::posix_time::microsec_clock::local_time();
        _busy.mark = boost::posix_time::microsec_clock::local_time();
        _busy.resource = resource;
        std::ostringstream oss;
        oss << "(time: " << _busy.timestamp << ") REASON: " << _busy.resource << " EXCEEDED " << message;
        busy_reason = oss.str();
    } else {
        boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
        boost::posix_time::time_duration dur = now - _busy.timestamp;
        boost::posix_time::time_duration last_msg = now - _busy.mark;
        std::ostringstream oss;
        oss << "(first/duration: " << _busy.timestamp << "/" << dur << ") REASON: " << _busy.resource << " EXCEEDED " << message;
        busy_reason = oss.str();
        if ( last_msg.total_seconds() >  2 ) {
            _busy.mark = now;
            RH_INFO(_baseLog, "GPP BUSY, " << oss.str() );
        }
    }
    setUsageState(CF::Device::BUSY);
}

/**
  override ExecutableDevice::set_resource_affinity to handle localized settings.
  
  NOTE: the get_affinity_logger method is required to get the rh_logger object used after the "fork" method is
  called.  ExecutableDevice will provide the logger to use.... 

  log4cxx will lock in the child process before execv is call for high frequency component deployments.
 */
void GPP_i::set_resource_affinity( const CF::Properties& options, const pid_t rsc_pid, const char *rsc_name, const std::vector<int> &bl )
 {

   RH_DEBUG( redhawk::affinity::get_affinity_logger(), "Affinity Options....GPP/Resource: " << label() << "/" << rsc_name << " options" << options.length()  );   

   // check if we override incoming affinity requests...
   if ( affinity.force_override ) {
     if ( redhawk::affinity::is_disabled() == false ) {
       RH_WARN(redhawk::affinity::get_affinity_logger(), "Enforcing GPP affinity property settings to resource, GPP/pid: " <<  label() << "/" << rsc_pid );
       if ( _apply_affinity( affinity, rsc_pid, rsc_name ) < 0 ) {
         throw redhawk::affinity::AffinityFailed("Failed to apply GPP affinity settings to resource");
       }
     }
     else {
       RH_WARN(redhawk::affinity::get_affinity_logger(), "Affinity processing disabled, unable to apply GPP affinity settings to resource, GPP/rsc/pid: " <<  label() << "/" << rsc_name << "/" << rsc_pid );
     }
   }
   else if ( affinity.deploy_per_socket && redhawk::affinity::has_nic_affinity(options) == false ) {

     if ( execPartitions.size() == 0 ) {
       RH_WARN(redhawk::affinity::get_affinity_logger(), "Skipping deploy_per_socket request. Reason: No execute partitions found, check numa settings. GPP/resource: " <<  label() << "/" << rsc_name );
       return;
     }

       // get next available socket to deploy on....
     if ( redhawk::affinity::is_disabled() == false ) {
       int psoc = _get_deploy_on_partition();
       if ( psoc < 0 ) {
         throw redhawk::affinity::AffinityFailed("Failed to apply PROCESSOR SOCKET affinity settings to resource");
       }
       RH_DEBUG(redhawk::affinity::get_affinity_logger(), "Enforcing PROCESSOR SOCKET affinity settings to resource, GPP/pid/socket: " <<  label() << "/" << rsc_pid << "/" << psoc );
       std::ostringstream os;
       os << psoc;
       if ( _apply_affinity( rsc_pid, rsc_name, "socket", os.str(), bl_cpus ) < 0 ) {
         throw redhawk::affinity::AffinityFailed("Failed to apply GPP affinity settings to resource");
       }
     }       
   }
   else {

     // create black list cpus
     redhawk::affinity::CpuList blcpus;
     blcpus.resize(bl_cpus.size());
     std::copy( bl_cpus.begin(), bl_cpus.end(), blcpus.begin() );

     //
     // Same flow as ExecutableDevice::set_resource_affinity
     //   
     try {
       if ( redhawk::affinity::has_affinity( options ) ) {
         RH_DEBUG(redhawk::affinity::get_affinity_logger(), "Has Affinity Options....GPP/Resource:" << label() << "/" << rsc_name);
         if ( redhawk::affinity::is_disabled() ) {
           RH_WARN(redhawk::affinity::get_affinity_logger(), "Resource has affinity directives but processing disabled, ExecDevice/Resource:" << 
                               label() << "/" << rsc_name);
         }
         else {
           RH_DEBUG(redhawk::affinity::get_affinity_logger(), "Calling set resource affinity.... GPP/Resource: " << label() << "/" << rsc_name);
           redhawk::affinity::AffinityDirectives spec = redhawk::affinity::convert_properties( options );
           gpp::affinity::set_affinity( spec, rsc_pid, blcpus );
         }
       }
       else {
         RH_TRACE(redhawk::affinity::get_affinity_logger(), "No Affinity Options Found....GPP/Resource: " << label() << "/" << rsc_name);
       }

     }
     catch( redhawk::affinity::AffinityFailed &e) {
       RH_WARN(redhawk::affinity::get_affinity_logger(), "AFFINITY REQUEST FAILED: " << e.what() );
       throw;
     }

   }
}


int GPP_i::serviceFunction()
{
  boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
  boost::posix_time::time_duration dur = now -time_mark;
  if ( dur.total_milliseconds() < threshold_cycle_time ) {
    return NOOP;
  }
  
  time_mark = now;

  // update data model for the GPP
  try {      
    std::for_each( data_model.begin(), data_model.end(), boost::bind( &Updateable::update, _1 ) );
    std::for_each( execPartitions.begin(), execPartitions.end(), boost::bind( &exec_socket::update, _1 ) );
  }
  catch( const boost::thread_resource_error& e ){
    std::stringstream errstr;
    errstr << "Error acquiring lock (errno=" << e.native_error() << " msg=\"" << e.what() << "\")";
    RH_ERROR(this->_baseLog, __FUNCTION__ << ": " << errstr.str() );
  }

  //
  // update state that affect device usage state
  //
  update();

  if ( execPartitions.size() ) {
    // dump execute partition status... 
    ExecPartitionList::iterator iter =  execPartitions.begin();
    std::ostringstream ss;
    ss  << boost::format("%-6s %-4s %-7s %-7s %-7s ") % "SOCKET" % "CPUS" % "USER"  % "SYSTEM"  % "IDLE";
    RH_TRACE(this->_baseLog, ss.str()  );
    ss.clear();
    ss.str("");
    for ( ; iter != execPartitions.end(); iter++ ) {
    
      ss  << boost::format("%-6d %-4d %-7.2f %-7.2f %-7.2f ") % iter->id % iter->stats.get_ncpus() % iter->stats.get_user_percent()  % iter->stats.get_system_percent()  % iter->stats.get_idle_percent() ;
      RH_TRACE(this->_baseLog, ss.str()  );    
      ss.clear();
      ss.str("");
    }
  }

  // update monitors to see if thresholds are exceeded
  updateThresholdMonitors();

  // update device usages state for the GPP
  updateUsageState();

  return NORMAL;
}


void GPP_i::_set_vlan_property() 
{
  mcastnicVLANs.clear();
  // grab nic_metrics for the specified interface
  if ( mcastnicInterface != "" ) {
    std::vector<nic_metrics_struct_struct>::iterator nic = nic_metrics.begin();
    for ( ; nic != nic_metrics.end(); nic++ ) {
      // found match
      if ( nic->interface == mcastnicInterface ) {
        std::vector<std::string> values;
        boost::algorithm::split( values, nic->vlans, boost::is_any_of(std::string(",")), boost::algorithm::token_compress_on );
        for ( size_t i=0; i< values.size(); i++){
          mcastnicVLANs.push_back( atoi(values[i].c_str()) );
        }
        break;
      }
    }
  }


}

//
//
//  Property Callback Methods
//
//


void GPP_i::_component_output_changed(const std::string& oldValue, const std::string& newValue)
{
    if (newValue.empty()) { 
        _componentOutputLog="";
        _handle_io_redirects = false; 
        return;
    }
    
    _componentOutputLog =__ExpandEnvVars(componentOutputLog);
    _handle_io_redirects = true;
}


void GPP_i::reservedChanged(float oldValue, float newValue)
{
    idle_capacity_modifier = 100.0 * reserved_capacity_per_component/((float)processor_cores);

    ExecPartitionList::iterator iter = execPartitions.begin();
    for( ; iter != execPartitions.end(); iter++ ) {
        iter->idle_cap_mod = 100.0 * reserved_capacity_per_component / ((float)iter->cpus.size());
    }
}


void  GPP_i::mcastnicThreshold_changed(int oldvalue, int newvalue)
{
    int threshold = newvalue;
    if ( threshold >= 0 && threshold <= 100 ) {
      double origIngressThreshold = mcastnicIngressThresholdValue;
      double origEgressThreshold = mcastnicIngressThresholdValue;
      mcastnicThreshold = threshold;
      mcastnicIngressThresholdValue = mcastnicIngressTotal * ( mcastnicThreshold / 100.0) ;
      mcastnicEgressThresholdValue = mcastnicEgressTotal * ( mcastnicThreshold / 100.0) ;

      if (  mcastnicIngressThresholdValue > origIngressThreshold  ) {
        // add extra to capacity
        mcastnicIngressCapacity += mcastnicIngressThresholdValue -origIngressThreshold;
        mcastnicIngressFree = mcastnicIngressCapacity;
      }
      else if ( mcastnicIngressThresholdValue < mcastnicIngressCapacity ){
        mcastnicIngressCapacity = mcastnicIngressThresholdValue;
        mcastnicIngressFree = mcastnicIngressCapacity;
      }

      if ( mcastnicEgressThresholdValue > origEgressThreshold ) {
        // add extra to capacity
        mcastnicEgressCapacity += mcastnicEgressThresholdValue-origEgressThreshold;
        mcastnicEgressFree = mcastnicEgressCapacity;
      }
      else if ( mcastnicEgressThresholdValue < mcastnicEgressCapacity ){
        mcastnicEgressCapacity = mcastnicEgressThresholdValue;
        mcastnicEgressFree = mcastnicEgressCapacity;
      }
    }
}


void GPP_i::_affinity_changed( const affinity_struct *ovp, const affinity_struct *nvp ) {

  if ( ovp && nvp && *ovp == *nvp ) return;
  
  const affinity_struct nv = *nvp;

  RH_NL_TRACE("GPP", "BEGIN Affinty Settings Changed.............");

  if ( ovp )  {
    const affinity_struct ov = *ovp;
    RH_DEBUG(this->_baseLog, "OV:                " );
    RH_DEBUG(this->_baseLog, "OV: ov.policy/context " << ov.exec_directive_class << "/" << ov.exec_directive_value );
    RH_DEBUG(this->_baseLog, "OV: ov.blacklist size " << ov.blacklist_cpus.size() );
    RH_DEBUG(this->_baseLog, "OV: ov.force_override " << ov.force_override );
    RH_DEBUG(this->_baseLog, "OV: ov.disabled       " << ov.disabled );
  }

  RH_DEBUG(this->_baseLog, "NV:                " );
  RH_DEBUG(this->_baseLog, "NV: nv.policy/context " << nv.exec_directive_class << "/" << nv.exec_directive_value );
  RH_DEBUG(this->_baseLog, "NV: nv.blacklist size " << nv.blacklist_cpus.size() );
  RH_DEBUG(this->_baseLog, "NV: nv.force_override " << nv.force_override );
  RH_DEBUG(this->_baseLog, "NV: nv.disabled       " << nv.disabled );

  // change affinity struct to affinity spec..
  redhawk::affinity::AffinityDirective value;
  if ( nv.exec_directive_class == "socket" ) {value.first ="socket"; value.second = nv.exec_directive_value; }
  if ( nv.exec_directive_class == "cpu" ) { value.first ="cpu"; value.second = nv.exec_directive_value; }
  if ( nv.exec_directive_class == "nic" ) { value.first ="nic"; value.second = nv.exec_directive_value; }
  if ( nv.exec_directive_class == "cgroup" ){ value.first ="cgroup"; value.second = nv.exec_directive_value; }


  // change affinity struct to affinity spec..
  bl_cpus.clear();
  try {
    redhawk::affinity::CpuList cpus = gpp::affinity::get_cpu_list( "cpu", nv.blacklist_cpus );
    bl_cpus.resize(cpus.size());
    std::copy( cpus.begin(), cpus.end(), bl_cpus.begin());
  }
  catch( redhawk::affinity::AffinityFailed &ex) {
    RH_NL_ERROR("GPP", "Unable to process blacklist cpu specification reason:"  << ex.what() );
  }

  {
    std::stringstream os;
    CpuList::const_iterator iter = bl_cpus.begin();
    for ( ; iter != bl_cpus.end(); iter++ ) {
      os << *iter;
      if ( iter +1 != bl_cpus.end() ) os << ",";
    }
    RH_NL_DEBUG("GPP", "Affinity Requested: Blacklist " << bl_cpus.size() << " str == " << os.str() );
  }

  // 
  // need to determine if cpu monitor list to watch is different...than current list
  // Right now we monitor all cpus on the host, but we can limit the view
  // by adding another property to the affinity_struct ... that follows format
  // from numa_parse_XXX methods
  //
  //
  redhawk::affinity::CpuList cpus;
  try {
    cpus = gpp::affinity::get_cpu_list( "node","all" );
  }
  catch( redhawk::affinity::AffinityFailed &ex) {
    RH_NL_ERROR("GPP", "Unable to process white listed cpu specification, reason:"  << ex.what() );
  }

  wl_cpus.clear();
  for( uint32_t i=0; i< cpus.size(); i++ ) {
    if ( std::count( bl_cpus.begin(), bl_cpus.end(), cpus[i] ) == 0 ) wl_cpus.push_back( cpus[i] );
  }

  // apply changes to member variable
  affinity = nv;
  RH_NL_DEBUG("GPP", "Affinity Force Override, force_override=" << nv.force_override);
  RH_NL_INFO("GPP", "Affinity Disable State,  disabled=" << nv.disabled);
  if ( nv.disabled || gpp::affinity::check_numa() == false ) {
    RH_NL_INFO("GPP", "Disabling affinity processing requests.");
    affinity.disabled=true;
    redhawk::affinity::set_affinity_state(affinity.disabled);
  }

  _set_processor_monitor_list( wl_cpus );

  // reset idle idle capcity monitor... idle threshold gets calculated during each loop of svc func.
  processor_cores = wl_cpus.size();
  if ( processor_cores == 0 ) processor_cores = boost::thread::hardware_concurrency();
  idle_capacity_modifier = 100.0 * reserved_capacity_per_component/((float)processor_cores);
  
  // reset capacity modifier for all execution partitions
  ExecPartitionList::iterator iter = execPartitions.begin();
  for( ; iter != execPartitions.end(); iter++ ) {
    iter->idle_cap_mod = 100.0 * reserved_capacity_per_component / ((float)iter->cpus.size());
  }

  RH_NL_TRACE("GPP", "END Affinty Settings Changed.............");
  
}

//
//
//  Allocation Callback Methods
//
//
bool GPP_i::allocate_mcastegress_capacity(const CORBA::Long &value)
{
    boost::mutex::scoped_lock lock(propertySetAccess);
    std::string  except_msg("Invalid allocation");
    bool retval=false;
    RH_DEBUG(this->_baseLog, __FUNCTION__ << ": Allocating mcastegress allocation " << value);

    if ( mcastnicInterface == "" )  {
      std::string msg = "mcastnicEgressCapacity request failed because no mcastnicInterface has been configured";
      RH_DEBUG(this->_baseLog, __FUNCTION__ <<  msg );
      throw CF::Device::InvalidState(msg.c_str());
      return retval;
    }

    // see if calculated capacity and measured capacity is avaliable
    if ( value > mcastnicEgressCapacity ) {
      std::ostringstream os;
      os << "mcastnicEgressCapacity request: " <<  value << " failed because of insufficent capacity available, current: " <<  mcastnicEgressCapacity;
      std::string msg = os.str();
      RH_DEBUG(this->_baseLog, __FUNCTION__ <<  msg );
      CF::Properties errprops;
      errprops.length(1);
      errprops[0].id = "mcastnicEgressCapacity";
      errprops[0].value <<= (CORBA::Long)value;
      throw CF::Device::InvalidCapacity(msg.c_str(), errprops);
    }
    
    // adjust property 
    retval= true;
    mcastnicEgressCapacity = mcastnicEgressCapacity - value;
    mcastnicEgressFree = mcastnicEgressCapacity;    
    return retval;
}


void GPP_i::deallocate_mcastegress_capacity(const CORBA::Long &value)
{
    boost::mutex::scoped_lock lock(propertySetAccess);
    RH_DEBUG(this->_baseLog, __FUNCTION__ << ": Deallocating mcastegress allocation " << value);

    mcastnicEgressCapacity = value + mcastnicEgressCapacity;
    if ( mcastnicEgressCapacity > mcastnicEgressThresholdValue ) {
      mcastnicEgressCapacity = mcastnicEgressThresholdValue;
    }

    mcastnicEgressFree = mcastnicEgressCapacity;
}

bool GPP_i::allocate_reservation_request(const redhawk__reservation_request_struct &value)
{
    if (isBusy()) {
        return false;
    }
    RH_DEBUG(this->_baseLog, __FUNCTION__ << ": allocating reservation_request allocation ");
    {
        WriteLock rlock(pidLock);
        if (applicationReservations.find(value.obj_id) != applicationReservations.end()){
            RH_INFO(_baseLog, __FUNCTION__ << ": Cannot make multiple reservations against the same application: "<<value.obj_id);
        } else {
            applicationReservations[value.obj_id] = application_reservation();
        }

        std::map<std::string,float>& reservations = applicationReservations[value.obj_id].reservation;
        for (unsigned int idx=0; idx<value.kinds.size(); idx++) {
            const std::string& kind = value.kinds[idx];
            float capacity = strtof(value.values[idx].c_str(), 0);
            if (reservations.count(kind) == 0) {
                reservations[kind] = capacity;
            } else {
                reservations[kind] += capacity;
            }
        }
    }
    updateUsageState();
    if (isBusy()) {
        WriteLock rlock(pidLock);
        for (unsigned int idx=0; idx<value.kinds.size(); idx++) {
            applicationReservations[value.obj_id].reservation[value.kinds[idx]] -= strtof(value.values[idx].c_str(), 0);
        }
        if (abs(applicationReservations[value.obj_id].reservation[value.kinds[0]]) <= 0.0001) {
            applicationReservations.erase(value.obj_id);
        }
        return false;
    }
    return true;
}

void GPP_i::deallocate_reservation_request(const redhawk__reservation_request_struct &value)
{
    WriteLock rlock(pidLock);
    RH_DEBUG(this->_baseLog, __FUNCTION__ << ": Deallocating reservation_request allocation ");
    for (ApplicationReservationMap::iterator app_it=applicationReservations.begin(); app_it!=applicationReservations.end(); app_it++) {
        if (app_it->first == value.obj_id) {
            applicationReservations.erase(app_it);
            break;
        }
    }
}



bool GPP_i::allocate_mcastingress_capacity(const CORBA::Long &value)
{
    boost::mutex::scoped_lock lock(propertySetAccess);
    std::string  except_msg("Invalid allocation");
    bool retval=false;
    RH_DEBUG(this->_baseLog, __FUNCTION__ << ": Allocating mcastingress allocation " << value);

    if ( mcastnicInterface == "" )  {
      std::string msg = "mcastnicIngressCapacity request failed because no mcastnicInterface has been configured" ;
      RH_DEBUG(this->_baseLog, __FUNCTION__ <<  msg );
      throw CF::Device::InvalidState(msg.c_str());
    }

    // see if calculated capacity and measured capacity is avaliable
    if ( value > mcastnicIngressCapacity ) {
      std::ostringstream os;
      os << "mcastnicIngressCapacity request: " << value << " failed because of insufficent capacity available, current: " <<  mcastnicIngressCapacity;
      std::string msg = os.str();
      RH_DEBUG(this->_baseLog, __FUNCTION__ <<  msg );
      CF::Properties errprops;
      errprops.length(1);
      errprops[0].id = "mcastnicIngressCapacity";
      errprops[0].value <<= (CORBA::Long)value;
      throw CF::Device::InvalidCapacity(msg.c_str(), errprops);
    }
    
    // adjust property 
    retval=true;
    mcastnicIngressCapacity = mcastnicIngressCapacity - value;
    mcastnicIngressFree = mcastnicIngressCapacity;
    
    return retval;
}


void GPP_i::deallocate_mcastingress_capacity(const CORBA::Long &value)
{
    boost::mutex::scoped_lock lock(propertySetAccess);
    RH_DEBUG(this->_baseLog, __FUNCTION__ << ": Deallocating mcastingress deallocation " << value);

    mcastnicIngressCapacity = value + mcastnicIngressCapacity;
    if ( mcastnicIngressCapacity > mcastnicIngressThresholdValue ) {
      mcastnicIngressCapacity = mcastnicIngressThresholdValue;
    }

    mcastnicIngressFree = mcastnicIngressCapacity;
}



bool GPP_i::allocateCapacity_nic_allocation(const nic_allocation_struct &alloc)
{
    WriteLock wlock(nicLock);
    std::string  except_msg("Invalid allocation");
    bool success=false;
    RH_TRACE(this->_baseLog, __FUNCTION__ << ": Allocating nic_allocation (identifier=" << alloc.identifier << ")");
    try
    {
        RH_TRACE(this->_baseLog, __FUNCTION__ << ": ALLOCATION: { identifier: \"" << alloc.identifier << "\", data_rate: " << alloc.data_rate << ", data_size: " << alloc.data_size << ", multicast_support: \"" << alloc.multicast_support << "\", ip_addressable: \"" << alloc.ip_addressable << "\", interface: \"" << alloc.interface << "\" }");
        success = nic_facade->allocate_capacity(alloc);
        
        if( success )
        {
            nic_allocation_status_struct_struct status;
            for( size_t i=0; i<nic_allocation_status.size(); ++i )
            {
                if( nic_allocation_status[i].identifier == alloc.identifier )
                {
                  status = nic_allocation_status[i];
                  // need to check if processor socket servicing interface has enough idle capacity
                  if ( _check_exec_partition( status.interface ) == true ) {
                      RH_TRACE(this->_baseLog, __FUNCTION__ << ": SUCCESS: { identifier: \"" << status.identifier << "\", data_rate: " << status.data_rate << ", data_size: " << status.data_size << ", multicast_support: \"" << status.multicast_support << "\", ip_addressable: \"" << status.ip_addressable << "\", interface: \"" << status.interface << "\" }");
                    break;
                  }
                  else {
                    except_msg = "Not enough processor capacity on socket that services NIC:" + status.interface;
                    throw -1;
                  }
                   
                }
            }
        }
        
        return success;
    }
    //catch( const NicAllocator::InvalidAllocation& e )
    catch( ... )
    {
      // if we made an allocation the unroll
      try {
        if ( success ) nic_facade->deallocate_capacity(alloc);
      }
      catch(...)
        {};

        CF::Properties errprops;
        errprops.length(1);
        errprops[0].id = "nic_allocation";
        errprops[0].value <<= alloc;
        throw CF::Device::InvalidCapacity(except_msg.c_str(), errprops);
    }
}


void GPP_i::deallocateCapacity_nic_allocation(const nic_allocation_struct &alloc)
{
  WriteLock wlock(nicLock);
  RH_TRACE(this->_baseLog, __FUNCTION__ << ": Deallocating nic_allocation (identifier=" << alloc.identifier << ")");
  try {
      RH_DEBUG(this->_baseLog, __FUNCTION__ << ": { identifier: \"" << alloc.identifier << "\", data_rate: " << alloc.data_rate << ", data_size: " << alloc.data_size << ", multicast_support: \"" << alloc.multicast_support << "\", ip_addressable: \"" << alloc.ip_addressable << "\", interface: \"" << alloc.interface << "\" }");
      nic_facade->deallocate_capacity(alloc);
    }
  catch( ... )
    {
      CF::Properties errprops;
      errprops.length(1);
      errprops[0].id = "nic_allocation";
      errprops[0].value <<= alloc;
      throw CF::Device::InvalidCapacity("Invalid allocation", errprops);
    }
}


bool GPP_i::allocate_diskCapacity(const double &value) {

  if (isBusy()) {
    return false;
  }
   
  return true;
}

void GPP_i::deallocate_diskCapacity(const double &value) {
  return;
}

bool GPP_i::allocate_memCapacity(const CORBA::LongLong &value) {

  if (isBusy()) {
    return false;
  }
  RH_DEBUG(this->_baseLog, "allocate memory (REQUEST) value: " << value << " memCapacity: " << memCapacity << " memFree:" << memFree  );
  if ( value > memCapacity or value > memCapacityThreshold )
    return false;

  memCapacity -= value;
  RH_DEBUG(this->_baseLog, "allocate memory (SUCCESS) value: " << value << " memCapacity: " << memCapacity << " memFree:" << memFree  );
  return true;
}

void GPP_i::deallocate_memCapacity(const CORBA::LongLong &value) {
  RH_DEBUG(this->_baseLog, "deallocate memory (REQUEST) value: " << value << " memCapacity: " << memCapacity << " memFree:" << memFree  );
  memCapacity += value;
  RH_DEBUG(this->_baseLog, "deallocate memory (SUCCESS) value: " << value << " memCapacity: " << memCapacity << " memFree:" << memFree  );
  if ( memCapacity > memCapacityThreshold ) {
    memCapacity  = memCapacityThreshold;
  }
  updateThresholdMonitors();
  updateUsageState();
  return;
}



bool GPP_i::allocate_loadCapacity(const double &value) {

  if (isBusy()) {
    return false;
  }

  // get current system load and calculated reservation load
  if ( reserved_capacity_per_component == 0.0 ) {

    RH_DEBUG(this->_baseLog, "allocate load capacity, (REQUEST) value: " << value << " loadCapacity: " << loadCapacity << " loadFree:" << loadFree );
    // get system monitor report...
    double load_threshold = modified_thresholds.load_avg;
    double sys_load = system_monitor->get_loadavg();
    if ( sys_load + value >  load_threshold   ) {
        RH_WARN(this->_baseLog, "Allocate load capacity would exceed measured system load, current loadavg: "  << sys_load << " requested: " << value << " threshold: " << load_threshold );
    }
    
    // perform classic load capacity
    if ( value > loadCapacity ) {
      std::ostringstream os;
      os << " Allocate load capacity failed due to insufficient capacity, available capacity:" << loadCapacity << " requested capacity: " << value;
      std::string msg = os.str();
      RH_DEBUG(this->_baseLog, msg );
      CF::Properties errprops;
      errprops.length(1);
      errprops[0].id = "DCE:72c1c4a9-2bcf-49c5-bafd-ae2c1d567056 (loadCapacity)";
      errprops[0].value <<= (CORBA::Long)value;
      throw CF::Device::InsufficientCapacity(errprops, msg.c_str());
    }

    loadCapacity -= value;
    RH_DEBUG(this->_baseLog, "allocate load capacity, (SUCCESS) value: " << value << " loadCapacity: " << loadCapacity << " loadFree:" << loadFree );
  }
  else { 
    // manage load capacity handled via reservation
    RH_WARN(this->_baseLog, "Allocate load capacity allowed, GPP using component reservations for managing load capacity." );

    loadCapacity -= value;
    if ( loadCapacity < 0.0 ) {
        loadCapacity = 0.0;
    }

    RH_DEBUG(this->_baseLog, "allocate load capacity, (SUCCESS) value: " << value << " loadCapacity: " << loadCapacity << " loadFree:" << loadFree );

  }
  
  updateUsageState();
  return true;
}

void GPP_i::deallocate_loadCapacity(const double &value) {
  RH_DEBUG(this->_baseLog, "deallocate load capacity, (REQUEST) value: " << value << " loadCapacity: " << loadCapacity << " loadFree:" << loadFree );
  loadCapacity += value;
  if ( loadCapacity > loadFree ) {
      loadCapacity = loadFree;
  }
  RH_DEBUG(this->_baseLog, "deallocate load capacity,  (SUCCESS) value: " << value << " loadCapacity: " << loadCapacity << " loadFree:" << loadFree );
  updateThresholdMonitors();
  updateUsageState();
  return;
}



//
//
// Support Methods
//
//


void GPP_i::send_threshold_event(const threshold_event_struct& message)
{
  RH_INFO(this->_baseLog, __FUNCTION__ << ": " << message.message );
  MessageEvent_out->sendMessage(message);
}

void GPP_i::sendChildNotification(const std::string &comp_id, const std::string &app_id)
{
  RH_INFO(this->_baseLog, "Child termination notification on the IDM channel : comp:" << comp_id  << " app:" <<app_id);
  redhawk::events::DomainEventWriter writer(idm_publisher);
  writer.sendComponentTermination( _identifier.c_str(), app_id.c_str(), comp_id.c_str() );
}



void GPP_i::updateThresholdMonitors()
{
  WriteLock wlock(monitorLock);
  std::for_each(threshold_monitors.begin(), threshold_monitors.end(), boost::bind(&Updateable::update, _1));
}


void GPP_i::updateProcessStats()
{
    // establish what the actual load is per floor_reservation
    // if the actual load -per is less than the reservation, compute the
    // different and add the difference to the cpu_idle
    {
        WriteLock rlock(pidLock);
        this->update_grp_child_pids();
    }

    // Update system and user clocks and determine how much time has elapsed
    // since the last measurement
    int64_t last_system_ticks = _systemTicks;
    int64_t last_user_ticks = _userTicks;
    ProcStat::GetTicks(_systemTicks, _userTicks);
    int64_t system_elapsed = _systemTicks - last_system_ticks;
    int64_t user_elapsed = _userTicks - last_user_ticks;

    float inverse_load_per_core = ((float)processor_cores)/(system_elapsed);
    float aggregate_usage = 0;
    float non_specialized_aggregate_usage = 0;

    ReadLock rlock(pidLock);
    for (ApplicationReservationMap::iterator app_it=applicationReservations.begin(); app_it!=applicationReservations.end(); app_it++) {
        app_it->second.usage = 0;
    }
    double reservation_set = 0;
    size_t nres=0;
    int usage_out=0;

    for (ProcessList::iterator i=this->pids.begin(); i!=pids.end(); i++, usage_out++) {
        if (i->terminated) {
            continue;
        }
        
        // get delta from last pstat
        int64_t usage = i->get_pstat_usage();

        double percent_core = (double)usage * inverse_load_per_core;
        i->core_usage = percent_core;
        double res =  i->reservation;

#if 0
        // debug assist
        if ( !(usage_out % 500) || usage < 0 || percent_core < 0.0 ) {  
            uint64_t u, p2, p1;
            u = i->get_pstat_usage(p2,p1);
            RH_INFO(_baseLog, __FUNCTION__ << std::endl<< "PROC SPEC PID: " << i->pid << std::endl << 
                     "  usage " << usage << std::endl << 
                     "  u " << usage << std::endl << 
                     "  p2 " << p2 << std::endl << 
                     "  p1 " << p1 << std::endl << 
                     "  percent_core: " << percent_core << std::endl << 
                     "  reservation: " << i->reservation << std::endl );
        }
#endif

        if ( applicationReservations.find(i->appName) != applicationReservations.end()) {
            if (applicationReservations[i->appName].reservation.find("cpucores") != applicationReservations[i->appName].reservation.end()) {
                applicationReservations[i->appName].usage += percent_core;
            }
        }

        if (i->app_started) {
            // if component is not using enough the add difference between minimum and current load
            if ( percent_core < res ) {
                reservation_set += 100.00 * ( res - percent_core)/((double)processor_cores);
            }
            // for components with non specific 
            if ( res == -1.0 ) {
                non_specialized_aggregate_usage +=  percent_core / inverse_load_per_core;
            }
            else {
                aggregate_usage += percent_core / inverse_load_per_core;
            }
        } else {
            if ( applicationReservations.find(i->appName) != applicationReservations.end()) {
                if (applicationReservations[i->appName].reservation.find("cpucores") != applicationReservations[i->appName].reservation.end()) {
                    continue;
                }
            }
            nres++;
            if ( i->reservation == -1) {
                reservation_set += idle_capacity_modifier;
            } else {
                reservation_set += 100.0 * i->reservation/((float)processor_cores);
            }
        }
    }

    // set number reservations that are not started
    n_reservations = nres;

    for (ApplicationReservationMap::iterator app_it=applicationReservations.begin(); app_it!=applicationReservations.end(); app_it++) {
        if (app_it->second.reservation.find("cpucores") != app_it->second.reservation.end()) {
            bool found_app = false;
            for ( ProcessList::iterator _pid_it=this->pids.begin();_pid_it!=pids.end(); _pid_it++) {
                if (applicationReservations.find(_pid_it->appName) != applicationReservations.end()) {
                    found_app = true;
                    break;
                }
            }
            if (not found_app) {
                if (app_it->second.reservation["cpucores"] == -1) {
                    reservation_set += idle_capacity_modifier;
                } else {
                    reservation_set += 100.0 * app_it->second.reservation["cpucores"]/((float)processor_cores);
                }
            } else {
                if (app_it->second.usage < app_it->second.reservation["cpucores"]) {
                    reservation_set += 100.00 * ( app_it->second.reservation["cpucores"] - app_it->second.usage)/((double)processor_cores);
                }
            }
        }
    }

    RH_TRACE(_baseLog, __FUNCTION__ << " Completed pass, record pstats for processes" );

    aggregate_usage *= inverse_load_per_core;
    non_specialized_aggregate_usage *= inverse_load_per_core;
    utilization[0].component_load = aggregate_usage + non_specialized_aggregate_usage;
    float estimate_total = (user_elapsed) * inverse_load_per_core;
    utilization[0].system_load = std::max(utilization[0].component_load, estimate_total); // for very light loads, sometimes there is a measurement mismatch because of timing
    utilization[0].subscribed = (reservation_set * (float)processor_cores) / 100.0 + utilization[0].component_load;

    // The maximum CPU utilization is in terms of cores; if a threshold is set,
    // normalize it to the range [0,1] and scale the maximum by that ratio
    float cpu_idle_threshold = 0.0;
    utilization[0].maximum = processor_cores;
    if (!thresholds.ignore && (thresholds.cpu_idle >= 0.0)) {
        utilization[0].maximum *= (1.0 - thresholds.cpu_idle * 0.01);
        modified_thresholds.cpu_idle = thresholds.cpu_idle + reservation_set;
        cpu_idle_threshold = thresholds.cpu_idle;
    }

    RH_DEBUG(_baseLog, __FUNCTION__ << " LOAD and IDLE : " << std::endl << 
              " modified_threshold(req+res)=" << modified_thresholds.cpu_idle << std::endl << 
              " system: idle: " << system_monitor->get_idle_percent() << std::endl << 
              "         idle avg: " << system_monitor->get_idle_average() << std::endl << 
              " threshold(req): " << cpu_idle_threshold << std::endl <<
              " idle modifier: " << idle_capacity_modifier << std::endl <<
              " reserved_cap_per_component: " << reserved_capacity_per_component << std::endl <<
              " number of reservations: " << n_reservations << std::endl <<
              " processes: " << pids.size() << std::endl <<
              " loadCapacity: " << loadCapacity  << std::endl <<
              " loadTotal: " << loadTotal  << std::endl <<
              " loadFree(Modified): " << loadFree <<std::endl );

    RH_TRACE(_baseLog, __FUNCTION__ << "  Reservation : " << std::endl << 
              "  total sys usage: " << system_elapsed << std::endl << 
              "  total user usage: " << user_elapsed << std::endl << 
              "  reservation_set: " << reservation_set << std::endl << 
              "  inverse_load_per_core: " << inverse_load_per_core << std::endl << 
              "  aggregate_usage: " << aggregate_usage << std::endl << 
              "  (non_spec) aggregate_usage: " << non_specialized_aggregate_usage << std::endl << 
              "  component_load: " << utilization[0].component_load << std::endl << 
              "  system_load: " << utilization[0].system_load << std::endl << 
              "  subscribed: " << utilization[0].subscribed << std::endl << 
              "  maximum: " << utilization[0].maximum << std::endl );
}

void GPP_i::update()
{
    updateProcessStats();

    const SystemMonitor::Report &rpt = system_monitor->getReport();
    RH_TRACE(_baseLog, __FUNCTION__ << " SysInfo Load : " << std::endl << 
              "  one: " << rpt.load.one_min << std::endl << 
              "  five: " << rpt.load.five_min << std::endl << 
              "  fifteen: " << rpt.load.fifteen_min << std::endl );

    loadAverage.onemin = rpt.load.one_min;
    loadAverage.fivemin = rpt.load.five_min;
    loadAverage.fifteenmin = rpt.load.fifteen_min;

    memFree = rpt.physical_memory_free / mem_free_units;
    RH_TRACE(_baseLog, __FUNCTION__ << "Memory : " << std::endl << 
              " sys_monitor.vit_total: " << rpt.virtual_memory_total  << std::endl << 
              " sys_monitor.vit_free: " << rpt.virtual_memory_free  << std::endl << 
              " sys_monitor.mem_total: " << rpt.physical_memory_total  << std::endl << 
              " sys_monitor.mem_free: " << rpt.physical_memory_free  << std::endl << 
              " memFree: " << memFree  << std::endl << 
              " memCapacity: " << memCapacity  << std::endl << 
              " memCapacityThreshold: " << memCapacityThreshold << std::endl << 
              " memInitCapacityPercent: " << memInitCapacityPercent << std::endl );

    shmFree = redhawk::shm::getSystemFreeMemory() / MB_TO_BYTES;

    //
    // transfer limits to properties
    //
    const Limits::Contents &sys_rpt =rpt.sys_limits;
    sys_limits.current_threads = sys_rpt.threads;
    sys_limits.max_threads = sys_rpt.threads_limit;
    sys_limits.current_open_files = sys_rpt.files;
    sys_limits.max_open_files = sys_rpt.files_limit;
    process_limits->update_state();
    const Limits::Contents &pid_rpt = process_limits->get();
    gpp_limits.current_threads = pid_rpt.threads;
    gpp_limits.max_threads = pid_rpt.threads_limit;
    gpp_limits.current_open_files = pid_rpt.files;
    gpp_limits.max_open_files = pid_rpt.files_limit;
}


int GPP_i::sigchld_handler(int sig)
{
  // Check if any children died....
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(sig_fd, &readfds);
  struct timeval tv = {0, 50};
  struct signalfd_siginfo si;
  ssize_t s;
  uint32_t cnt=1;

  if ( sig_fd > -1 ) {
    // don't care about writefds and exceptfds:
    while (true) {
      FD_ZERO(&readfds);
      FD_SET(sig_fd, &readfds);
      select(sig_fd+1, &readfds, NULL, NULL, &tv);
      if (FD_ISSET(sig_fd, &readfds)) {
        RH_TRACE(this->_baseLog, " Checking for signals from SIGNALFD(" << sig_fd << ") cnt:" << cnt++ );
        s = read(sig_fd, &si, sizeof(struct signalfd_siginfo));
        RH_TRACE(this->_baseLog, " RETURN from SIGNALFD(" << sig_fd << ") cnt/ret:" << cnt << "/" << s );
        if (s != sizeof(struct signalfd_siginfo)){
          RH_ERROR(this->_baseLog, "SIGCHLD handling error ...");
          break;
        }

        //
        // for sigchld and signalfd
        //   if there are many SIGCHLD events that occur at that same time, the kernel
        //   can compress the event into a single process id...there for we need to 
        //   try and waitpid for any process that have died.  The process id 
        //   reported might not require waitpid, so try and clean up outside the waitpid loop
        //
        //  If a child dies, try to clean up tracking state for the resource. The _component_cleanup
        //  will issue a notification event message for non domain terminated resources.. ie. segfaults..
        //
        if ( si.ssi_signo == SIGCHLD) {
          RH_TRACE(this->_baseLog, "Child died , pid .................................." << si.ssi_pid);
          int status;
          pid_t child_pid;
          bool reap=false;
          {
              SCOPED_LOCK(_forkLock);
              _forkMsg = FORK_WAIT;
              _forkReady.notify_all();
          }
          
          while((child_pid = waitpid(-1, &status, WNOHANG)) > 0 ) {
              RH_TRACE(this->_baseLog, "WAITPID died , pid .................................." << child_pid);
              if ( (uint)child_pid == si.ssi_pid ) reap=true;
              _component_cleanup( child_pid, status );
          }
          if ( !reap ) {
              _component_cleanup( si.ssi_pid, status );
          }          

          {
              SCOPED_LOCK(_forkLock);
              _forkMsg = FORK_GO;
              _forkReady.notify_all();
          }
        }
        else {
          RH_TRACE(this->_baseLog, "read from signalfd --> signo:" << si.ssi_signo);
        }
      } 
      else {
        break;
      }
    }
  }

  //RH_TRACE(this->_baseLog, "sigchld_handler RETURN.........loop cnt:" << cnt);
  return NOOP;
}


int GPP_i::redirected_io_handler()
{
  // check if we should be handling io redirects
  if ( !_handle_io_redirects || redirectedFds.size() == 0 )  {
    return NOOP;
  }

  // check we have a log file
  if ( _componentOutputLog == "" ) {
    RH_DEBUG(this->_baseLog, " Component IO redirect ON but no file specified. ");
    return NOOP;
  }

  RH_DEBUG(this->_baseLog, " Locking For Redirect Processing............. ");
  ReadLock lock(fdsLock);  

  int redirect_file = open(_componentOutputLog.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH  );
  if ( redirect_file != -1 )  {
    if ( lseek(redirect_file, 0, SEEK_END) == -1  )  {
      RH_DEBUG(this->_baseLog, " Unable to SEEK To file end, file: " << _componentOutputLog);
    }
  }
  else {
    RH_TRACE(this->_baseLog, " Unable to open up componentOutputLog,  fallback to /dev/null   tried log: " << _componentOutputLog);
    redirect_file = open("/dev/null", O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH  );
  }

  size_t   size = 0;
  uint64_t cnt = 0;
  uint64_t fcloses = 0;
  uint64_t nbytes = 0;
  size_t   result=0;
  int      rd_fd =0;

  size_t nfds=redirectedFds.size();
  // set default redirect to be master
  rd_fd=redirect_file;
  std::vector<epoll_event> events(nfds);
  int rfds = epoll_wait(epfd, events.data(), nfds, 10);

  if ( rfds > 0 ) {
      for ( int i=0; i< rfds; i++ ) { 
	  size = 0;
	  result=0;
	  proc_redirect *fd=(proc_redirect*)events[i].data.ptr;
	  if (ioctl (fd->cout, FIONREAD, &size) == -1) {
	      RH_ERROR(this->_baseLog, "(redirected IO) Error requesting how much to read,  PID: " << fd->pid << " FD:" << fd->cout );       
	      close(fd->cout);
	      fd->cout = -1;
	      fcloses++;
	  }
	  if ( fd->cout != -1 && rd_fd != -1 )  {
	      result = splice( fd->cout, NULL, rd_fd, NULL, size,0 );
	      RH_TRACE(this->_baseLog, " SPLICE DATA From Child to Output RES:" << result << "...  PID: " << fd->pid << " FD:" << fd->cout );        
	  }
	  if ( (int64_t)result == -1 )  {
              RH_ERROR(this->_baseLog, "(redirected IO) Error during transfer to redirected file,   PID: " << fd->pid << " FD:" << fd->cout );        
	      close(fd->cout);
	      fd->cout = -1;
	      fcloses++;
	  }
	  else {
	      nbytes += result;
	      cnt++;
	  }
      }
  }
  
  // close file while we wait
  if ( redirect_file ) close(redirect_file);
  RH_DEBUG(this->_baseLog, " IO REDIRECT,  NPROCS: "<< redirectedFds.size() << " CLOSED: " << fcloses <<" PROCESSED PROCS/Bytes " << cnt << "/" << nbytes );
  return NOOP;
}



std::vector<int> GPP_i::getPids()
{
    ReadLock lock(pidLock);
    std::vector<int> keys;
    for (ProcessList::iterator it=pids.begin();it!=pids.end();it++) {
        if ((not it->terminated) and (not it->reaped)) {
            keys.push_back(it->pid);
        }
    }
    return keys;
}

void GPP_i::addProcess(int pid, const std::string &appName, const std::string &identifier, const float req_reservation=1.0)
{
  WriteLock lock(pidLock);
  ProcessList:: iterator result = std::find_if( pids.begin(), pids.end(), std::bind2nd( FindPid(), pid ) );
  if ( result != pids.end() ) return;

  RH_DEBUG(this->_baseLog, "START Adding Process/RES: "  <<  pid << "/" << req_reservation << "  APP:" << appName );
  component_description tmp;
  tmp.appName = appName;
  tmp.pid  = pid;
  tmp.identifier = identifier;
  tmp.reservation = req_reservation;
  tmp.core_usage = 0;
  tmp.parent = this;
  pids.push_front( tmp );
  if (applicationReservations.find(appName) != applicationReservations.end()) {
      applicationReservations[appName].component_pids.push_back(pid);
  }
  RH_DEBUG(this->_baseLog, "END Adding Process/RES: "  <<  pid << "/" << req_reservation << "  APP:" << appName );
}

GPP_i::component_description GPP_i::getComponentDescription(int pid)
{
  ReadLock lock(pidLock);
  ProcessList:: iterator it = std::find_if( pids.begin(), pids.end(), std::bind2nd( FindPid(), pid ) );
  if (it == pids.end()) {
        throw std::invalid_argument("pid not found");
  }
  return *it;
}

GPP_i::component_description  GPP_i::markPidTerminated(const int pid)
{
    ReadLock lock(pidLock);
    ProcessList:: iterator it = std::find_if( pids.begin(), pids.end(), std::bind2nd( FindPid(), pid ) );
    if (it == pids.end()){
        throw std::invalid_argument("pid not found");
    }
    RH_DEBUG(this->_baseLog, " Mark For Termination: "  <<  it->pid << "  APP:" << it->appName );
    it->app_started= false;
    it->terminated = true;
    return *it;
}

GPP_i::component_description GPP_i::markPidReaped(const int pid)
{
    ReadLock lock(pidLock);
    ProcessList:: iterator it = std::find_if( pids.begin(), pids.end(), std::bind2nd( FindPid(), pid ) );
    if (it == pids.end()) {
        throw std::invalid_argument("pid not found");
    }
    RH_DEBUG(this->_baseLog, " Mark For Reaping: "  <<  it->pid << "  APP:" << it->appName );
    it->reaped = true;
    return *it;
}

void GPP_i::removeProcess(int pid)
{
  {
    WriteLock wlock(pidLock);
    ProcessList:: iterator result = std::find_if( pids.begin(), pids.end(), std::bind2nd( FindPid(), pid ) );
    if ( result != pids.end() ) {
      RH_DEBUG(this->_baseLog, "Monitor Process: REMOVE Process: " << result->pid << " app: " << result->appName );
      pids.erase(result);
    }
  }

  {
    WriteLock  wlock(fdsLock);
    ProcessFds::iterator i=std::find_if( redirectedFds.begin(), redirectedFds.end(), std::bind2nd( FindRedirect(), pid ) );
    if ( i != redirectedFds.end() )  {
	ProcRedirectPtr rdp=*i;
	rdp->close();
	rdp.reset();
        RH_DEBUG(this->_baseLog, "Redirectio IO ..REMOVE Redirected pid:" << pid  );
	redirectedFds.erase(i);
    }
  }
    
}

int GPP_i::_apply_affinity( const affinity_struct &nv,
                            const pid_t rsc_pid, 
                            const char *rsc_name  ) {

  return _apply_affinity( rsc_pid, rsc_name, nv.exec_directive_class, nv.exec_directive_value, bl_cpus );
}


int GPP_i::_apply_affinity(  const pid_t rsc_pid, 
                             const char *rsc_name,
                             const std::string &dir_class,
                             const std::string &dir_value,
                             const CpuList &bl_cpus) {

  int retval=0;
  // change affinity struct to affinity spec..
  redhawk::affinity::AffinityDirectives pol;
  redhawk::affinity::AffinityDirective value;
  value.first = dir_class;
  value.second = dir_value;
  pol.push_back(value);

  redhawk::affinity::CpuList blcpus;
  try {
    //bl_cpus = gpp::affinity::get_cpu_list( "cpu", nv.blacklist_cpus );
    blcpus.resize(bl_cpus.size());
    std::copy( bl_cpus.begin(), bl_cpus.end(), blcpus.begin() );
  }
  catch( redhawk::affinity::AffinityFailed &ex) {
    RH_NL_ERROR("GPP", "Unable to process blacklist cpu specification reason:"  << ex.what() );
    throw;
  }

  RH_NL_DEBUG("GPP", "Affinity Override Requested (processor socket),  pid/name: " << rsc_pid << "/" << rsc_name <<" pol/value "  << value.first << "/" << value.second );
  
  // apply affinity changes to the process
  try {
    if ( gpp::affinity::set_affinity( pol, rsc_pid, blcpus) != 0 )  {
      RH_NL_WARN("GPP", "Unable to set affinity for process, pid/name: " << rsc_pid << "/" << rsc_name );
    }
  }
  catch( redhawk::affinity::AffinityFailed &ex) {
    RH_NL_ERROR("GPP", "Unable to set affinity, reason:"  << ex.what() );
    throw;
  }

  return retval;
}


void GPP_i::_set_processor_monitor_list( const CpuList &cpus ) {
  std::ostringstream os;  
  if ( cpus.size() == 0 ) {
    os << "all";
  }
  else {
    CpuList::const_iterator iter = cpus.begin();
    for ( ; iter != cpus.end(); iter++ ) {
      os << *iter;
      if ( iter +1 != cpus.end() ) os << ",";
    }
  }

  RH_NL_DEBUG("GPP", "Processor white list " << wl_cpus.size() << " str == " << os.str() );
  processor_monitor_list = os.str();
}


bool GPP_i::_check_exec_partition( const std::string &iface ){

  bool retval=false;
  if ( execPartitions.size() == 0 ) {
    RH_NL_INFO("GPP", " _check_exec_partition: no exec partitions available.. iface:" << iface );
    retval=true;
  }
  else {
    int  soc=gpp::affinity::find_socket_for_interface( iface );
    RH_NL_INFO("GPP", " _check_exec_partition: iface:" << iface << "/" << soc );
    if  ( soc > -1  ) {
    
      if ( (uint32_t)soc < execPartitions.size() ) {
        const exec_socket &ep = execPartitions[soc];
        // get modified idle threshold value
        double m_idle_thresh = ep.idle_threshold + ( ep.idle_cap_mod * n_reservations) + 
          (float)loadCapacity/(float)ep.cpus.size();
        RH_NL_DEBUG("GPP", " Checking Execution Partition for an NIC interface iface/socket " << iface << "/" << soc << ") IDLE: actual/avg/threshold limit/modified " <<  
                   ep.get_idle_percent() << "/" << ep.get_idle_average() << "/" << ep.idle_threshold << "/" << m_idle_thresh );
        if ( ep.get_idle_percent() > m_idle_thresh ) {
          retval = true;
        }
        if ( ep.get_idle_average() > m_idle_thresh ) {
          retval = true;
        }
      }
    }
  }
  
  return retval;
}

int  GPP_i::_get_deploy_on_partition() {

  int psoc=-1;
  ExecPartitionList::iterator  iter = execPartitions.begin();
  for( ;  iter != execPartitions.end(); iter++ ) {
    // get modified idle threshold value
    double m_idle_thresh = iter->idle_threshold + ( iter->idle_cap_mod * n_reservations) + 
      (float)loadCapacity/(float)iter->cpus.size();
    RH_NL_DEBUG("GPP", " Looking for execute partition (processor socket:" << iter->id << ") IDLE: actual/avg/threshold limit/modified " <<  
		iter->get_idle_percent() << "/" << iter->get_idle_average() << "/" << iter->idle_threshold << "/" << m_idle_thresh );
    if ( iter->get_idle_percent() > m_idle_thresh ) {
      psoc=iter->id;
      break;
    }
    if ( iter->get_idle_average() > m_idle_thresh ) {
      psoc=iter->id;
      break;
    }
  }

  if ( psoc > -1 ) { RH_NL_INFO("GPP", " Deploy resource on selected SOCKET PARTITON, socket:" << psoc ); }
  return psoc;
}

void GPP_i::_cleanupProcessShm(pid_t pid)
{
    const std::string heap_name = redhawk::shm::getProcessHeapName(pid);
    redhawk::shm::SuperblockFile heap(heap_name);
    try {
        heap.open(false);
    } catch (const std::exception&) {
        // Ignore error, it probably doesn't exist
        return;
    }
    RH_DEBUG(_baseLog, "Removing shared memory heap '" << heap_name << "'");
    try {
        heap.file().unlink();
    } catch (const std::exception&) {
        // Someone else removed it in the meantime
    }
}
