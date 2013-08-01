#ifndef LOCAL_DATE_TIME_TIMEZONE_HPP
#define LOCAL_DATE_TIME_TIMEZONE_HPP

#include <map>
#include <memory>
#include <vector>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <fstream>
#include <set>
#include <iterator>

namespace local_time {

using boost::posix_time::time_duration;
using boost::posix_time::ptime;

  
  
namespace detail {

//! Convert an integer representing the number of microseconds since the epoch to a ptime
static boost::posix_time::ptime microseconds_to_ptime(int64_t microsecs) {
  static boost::gregorian::date epoch(1970, 1, 1);
  long long dt = microsecs / 86400000000ULL;
  long long tm = microsecs % 86400000000ULL;
  long h = tm / 3600000000;
  long m = (tm % 3600000000) / 60000000;
  long s = (tm % 60000000) / 1000000;
  long msecs = tm % 1000000;
  boost::posix_time::ptime pt(epoch + boost::gregorian::days(dt), boost::posix_time::time_duration(h, m, s, msecs));
  return pt;
}

//! Convert a ptime to an integer representing the number of microseconds since the epoch
static int64_t ptime_to_microseconds(const boost::posix_time::ptime& p) {
  static boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
  return (p - epoch).total_microseconds();
}

//!
inline static time_duration seconds_to_time_duration(long seconds) {
  if(seconds / 3600 > std::numeric_limits<time_duration::hour_type>::max() || seconds / 3600 < std::numeric_limits<time_duration::hour_type>::min())
    throw std::out_of_range("Value is too large");
  return time_duration(seconds / 3600, (seconds / 3600) / 60, seconds % 60, 0);
}

}
  
  
struct local_time_exception : public std::logic_error
{
public:
  //! Constructor
  explicit local_time_exception(const std::string& what) : std::logic_error(what) {}
};  
  
struct ambiguous_result : public std::logic_error
{
public:
  //! Constructor
  explicit ambiguous_result(const std::string& timezone_name, const std::string& local_time_str) : std::logic_error(local_time_str + " is ambiguous for the " + timezone_name + " timezone.") {}
};  

struct time_label_invalid : public std::logic_error
{
public:
  //! Constructor
  explicit time_label_invalid(const std::string& timezone_name, const std::string& local_time_str) : std::logic_error(local_time_str + " is invalid for the " + timezone_name + " timezone.") {}
};  



struct time_zone_entry_info {
  time_zone_entry_info(long seconds, const std::string& abbr, bool is_dst) : offset(detail::seconds_to_time_duration(seconds)), tz(abbr), dst(is_dst) {  }
  
  time_duration         offset;     //!< time_duration offset
  std::string           tz;         //!< timezone abbr
  bool                  dst;        //!< dst or not
};


class time_zone;
typedef std::shared_ptr<time_zone>       time_zone_ptr;
typedef std::shared_ptr<const time_zone> time_zone_const_ptr  ;

class time_zone {
  
public:

  enum automatic_conversion { ASSUME_DST, ASSUME_NON_DST, THROW_ON_AMBIGUOUS };
  
  const std::string& name() const { return _name; }

  time_zone(const std::string& name) : _name(name) { }
  
  void add_entry(int64_t microsecs, time_zone_entry_info&& tze) {
    if(!_data.insert(std::make_pair(detail::microseconds_to_ptime(microsecs), std::move(tze))).second)
      throw local_time_exception("Failed adding entry to the time zone.");
  }

  void remove_entry(int64_t microsecs) {
    if(!_data.erase(detail::microseconds_to_ptime(microsecs)))
      throw local_time_exception("Failed erasing the time zone entry.");
  }
  
  static time_zone_ptr duplicate(time_zone_const_ptr p) {
    time_zone_ptr ptr(new time_zone(p->name()));
    ptr->_data = p->_data;
    return ptr;
  }
  
private:

  std::string                            _name;         //!< time zone name
  std::map<ptime, time_zone_entry_info>  _data;         //!< contains the discontinuity points for the time zone
  
  ptime utc_to_local(const ptime& p) const {
    const time_zone_entry_info* z = zone_info_from_utc(p);
    if(z)
      return p - z->offset;
    else 
      return p;
  }

  ptime local_to_utc(const ptime& p, automatic_conversion dst = THROW_ON_AMBIGUOUS) const {
    const time_zone_entry_info* z = zone_info_from_local(p, dst);
    if(z)
      return p + z->offset;
    else 
      return p;
  }
  
  const time_zone_entry_info* zone_info_from_utc(const ptime& p) const {
    if(!_data.size())
      return nullptr;
    auto prev = --(_data.lower_bound(p));
    return &prev->second;
  }
  
  const time_zone_entry_info* zone_info_from_local(const ptime& loc, automatic_conversion dst = THROW_ON_AMBIGUOUS) const {
    switch(_data.size()){
      case 0:
        return nullptr;
      case 1:
        return &(_data.begin()->second);
    }

    auto segment = std::upper_bound(_data.begin(), _data.end(), loc, [](const ptime& p, const std::pair<ptime, time_zone_entry_info>& r){ return p < r.first - r.second.offset; });
    if(segment == _data.begin())
      return &segment->second;
    auto next_segment(segment); --segment; 
    // segment is now the first element such that: time - offset <= loc

    // check the left side
    if(segment != _data.begin()) {
      auto prev_segment(segment); --prev_segment;
      if(segment->first - prev_segment->second.offset > loc) { // in previous segment too
        switch(dst) {
          case ASSUME_DST:
            if(segment->second.dst && !prev_segment->second.dst)
              return &segment->second;
            if (!segment->second.dst && prev_segment->second.dst)
              return &prev_segment->second;
            break;
          case ASSUME_NON_DST:
            if(segment->second.dst && !prev_segment->second.dst)
              return &prev_segment->second;
            if (!segment->second.dst && prev_segment->second.dst)
              return &segment->second;
            break;
          case THROW_ON_AMBIGUOUS:
            break;
        }
        throw ambiguous_result(_name, boost::posix_time::to_iso_string(loc));
      }
    }
    
    // check the right side
    if( next_segment != _data.end()) {
      if(next_segment->first - segment->second.offset <= loc) { // also in the next segment
        switch(dst) {
          case ASSUME_DST:
            if(segment->second.dst && !next_segment->second.dst)
              return &segment->second;
            if (!segment->second.dst && next_segment->second.dst)
              return &next_segment->second;
            break;
          case ASSUME_NON_DST:
            if(segment->second.dst && !next_segment->second.dst)
              return &next_segment->second;
            if (!segment->second.dst && next_segment->second.dst)
              return &segment->second;
            break;
          case THROW_ON_AMBIGUOUS:
            break;
        }
        throw time_label_invalid(_name, boost::posix_time::to_iso_string(loc));
      }
    }

    return &segment->second;
  }
  
  std::string utc_to_local_string(const ptime& p) const {
    const time_zone_entry_info* z = zone_info_from_utc(p);
    if(z != nullptr)
      return boost::posix_time::to_iso_string(p - z->offset) + " " + z->tz;
    else
      return boost::posix_time::to_iso_string(p);       // LCOV_EXCL_LINE
  }
  
  std::string utc_to_local_iso_string(const ptime& p) const {
    const time_zone_entry_info* z = zone_info_from_utc(p);
    if(!z)
      return boost::posix_time::to_iso_string(p);
    std::ostringstream ss;
    ss << boost::posix_time::to_iso_string(p - z->offset);
    if(z && z->offset.total_seconds()) {
      auto h = z->offset.hours();
      auto m = z->offset.minutes();
      auto s = z->offset.seconds();
      
      ss << (z->offset.is_negative() ? '-' : '+');
      ss << std::setfill('0') << std::setw(2) << h << std::setw(2) << m;
      if(s)
        ss << std::setw(2) << s;
    }
    return ss.str();
  }
  
  friend class time_zone_database;
  friend class local_date_time;
}; 
  

class time_zone_database {
public:
  
  time_zone_database() { }

  bool save_to_file(const std::string& filename) {
    std::ofstream f(filename);
    if(!f.is_open())
      return false;
    
    for(auto tzit=_timezones.begin(); tzit!=_timezones.end(); ++tzit) {
      for(auto it=tzit->second->_data.begin(); it!=tzit->second->_data.end(); ++it) {
        f << tzit->first << ","
          << detail::ptime_to_microseconds(it->first) << ","
          << (it->second.offset.hours()*3600 + it->second.offset.minutes()*60 + it->second.offset.seconds()) << ","
          << it->second.tz << ","
          << (it->second.dst ? 1 : 0)
          << std::endl;
      }
    }
    
    return true; 
  }
  
  
  bool load_from_file(const std::string& filename) {
    enum db_fields { NAME, ISOTIME, OFFSET, ABBR, DSTADJUST, FIELD_COUNT };  
    
    std::ifstream f(filename);
    if(!f.is_open())
      return false;
    std::string line;
    map_type _timezones_new;
    while(getline(f, line)) {
      auto result = parse_string(line);
      // make sure we got the right number of fields
      if(result.size() != FIELD_COUNT) {
        std::ostringstream msg;
        msg << "Expecting " << FIELD_COUNT << " fields, got " 
              << result.size() << " fields in line: " << line;      
        throw std::runtime_error(msg.str());
      }
      // read the information, cast to appropriate values
      int64_t msecs = atoll(result[1].c_str());
      ptime pt = detail::microseconds_to_ptime(msecs);
      time_zone_entry_info tze(std::atol(result[2].c_str()), result[3], (result[4] == "1"));

      auto tz_it = _timezones_new.find(result[0]);
      if(tz_it == _timezones_new.end()) {
        time_zone_ptr tz = time_zone_ptr(new time_zone(result[0]));
        tz_it = _timezones_new.insert(std::make_pair(result[0], tz)).first;
      }
      tz_it->second->_data.insert(std::make_pair(pt, tze));
    }

    // copy other timezones from existing variable
    _timezones_new.insert(_timezones.begin(), _timezones.end());
    
    // assign to member data
    _timezones.swap(_timezones_new);
    
    return true;
  }


  bool load_from_struct(const std::map<std::string, std::vector<std::tuple<int64_t, long, std::string, bool> > >& data) {

    try {
      map_type _timezones_new;
      for(auto zone_it=data.begin(); zone_it!=data.end(); ++zone_it) {
        auto tz_it = _timezones_new.find(zone_it->first);
        if(tz_it == _timezones_new.end()) {
          time_zone_ptr tz = time_zone_ptr(new time_zone(zone_it->first));
          tz_it = _timezones_new.insert(std::make_pair(zone_it->first, tz)).first;
        }
        for(auto it=zone_it->second.begin(); it!=zone_it->second.end(); ++it) {
          // read the information, cast to appropriate values
          ptime pt = detail::microseconds_to_ptime(std::get<0>(*it));
          time_zone_entry_info tze(std::get<1>(*it), std::get<2>(*it), std::get<3>(*it));
    
          tz_it->second->_data.insert(std::make_pair(pt, std::move(tze)));
        }
      }

      // copy other timezones from existing variable
      _timezones_new.insert(_timezones.begin(), _timezones.end());
      
      // assign to member data
      _timezones.swap(_timezones_new);
    }
    catch(...) {
      return false;
    }

    return true;  
  }

  static time_zone_database from_file(const std::string& filename) {
    time_zone_database tzdb;
    if(!tzdb.load_from_file(filename))
      throw std::runtime_error("Error loading time zone database file");
    return tzdb;
  }
  
  static time_zone_database from_struct(const std::map<std::string, std::vector<std::tuple<int64_t, long, std::string, bool> > >& data) {
    time_zone_database tzdb;
    if(!tzdb.load_from_struct(data))
      throw std::runtime_error("Error loading time zone database struct");
    return tzdb;
  }
  
  bool add_record(std::string id, time_zone_ptr tz) {
    _timezones[id] = tz;
    return true;
  }

  bool delete_record(std::string id){
    _timezones.erase(id);
    return true;
  }
  
  time_zone_const_ptr time_zone_from_region(const std::string& id) const {
    try {
      return _timezones.at(id);
    }
    catch(const std::out_of_range&) {
      return time_zone_ptr();
    }
  }
  
  std::set<std::string> region_list() const {
    std::set<std::string> v;
    std::transform(_timezones.begin(), _timezones.end(), std::inserter(v, v.end()), [](const map_type::value_type& p){return p.first;});
    return v;
  }
  
private:
  typedef std::map<std::string, std::shared_ptr<time_zone> > map_type;
  
  map_type                                              _timezones;
    
  static std::vector<std::string> parse_string(const std::string& s) {
    std::vector<std::string> v;
    boost::tokenizer<boost::escaped_list_separator<char> > tok(s);
    std::transform(tok.begin(), tok.end(), std::back_inserter(v), [](const std::string& p){return p;}); 
    return v;
  }
};





}


#endif
