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
#include <boost/filesystem.hpp>


#ifdef USE_ZONEINFO
#include <tuple>
extern "C" {
#include "tzfile.h"
#include "stdint.h"
}
#endif //USE_ZONEINFO


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
  
  #ifdef USE_ZONEINFO
  static time_zone from_zoneinfo(const std::string& name, const std::string& path=TZDIR) {
    boost::filesystem::path file_path(path);
    file_path /= name;

    // read tzhead
    std::ifstream ifs(file_path.string(), std::ios::binary);
    if(!ifs.good())
      throw std::runtime_error("Error opening zone file '" + file_path.string() + "'"); // LCOV_EXCL_LINE
    ifs.seekg(0, std::ios::beg);

    union th_union_t {
      tzhead                head;
      const char*           ptr;
    };

    #ifndef TYPE_SIGNED
    #define TYPE_SIGNED(type) (((type) -1) < 0)
    #endif /* !defined TYPE_SIGNED */

    /* The minimum and maximum finite time values.  */
    static time_t const time_t_min =
      (TYPE_SIGNED(time_t)
      ? (time_t) -1 << (CHAR_BIT * sizeof (time_t) - 1)
      : 0);
    static time_t const time_t_max =
      (TYPE_SIGNED(time_t)
      ? - (~ 0 < 0) - ((time_t) -1 << (CHAR_BIT * sizeof (time_t) - 1))
      : -1);

      std::string contents;
    ifs.seekg(0, std::ios::end);
    contents.resize(ifs.tellg());
    ifs.seekg(0, std::ios::beg);
    ifs.read(&contents[0], contents.size());
    ifs.close();

    const char* data = contents.c_str();
    const th_union_t *th_union = reinterpret_cast<const th_union_t*>(data);
    const tzhead* th = &th_union->head;

    if(std::string(th->tzh_magic, 4) != TZ_MAGIC)
      throw std::runtime_error("Invalid zone file '" + file_path.string() + "'"); // LCOV_EXCL_LINE

    std::vector<time_t> transitions;
    std::vector<std::tuple<int, bool, short>> types;
    std::vector<unsigned char> transition_types;
    const char* abbr;

    for (int stored = 4; stored <= 8; stored *= 2) {
      int ttisstdcnt = (int) detzcode(th->tzh_ttisstdcnt);
      int ttisgmtcnt = (int) detzcode(th->tzh_ttisgmtcnt);
      int leapcnt = (int) detzcode(th->tzh_leapcnt);
      int timecnt = (int) detzcode(th->tzh_timecnt);
      int typecnt = (int) detzcode(th->tzh_typecnt);
      int charcnt = (int) detzcode(th->tzh_charcnt);

      const char* ptr = th->tzh_charcnt + sizeof(th->tzh_charcnt);
      if (leapcnt < 0 || leapcnt > TZ_MAX_LEAPS || typecnt <= 0 || typecnt > TZ_MAX_TYPES || timecnt < 0 || timecnt > TZ_MAX_TIMES || charcnt < 0 || charcnt > TZ_MAX_CHARS || (ttisstdcnt != typecnt && ttisstdcnt != 0) || (ttisgmtcnt != typecnt && ttisgmtcnt != 0))
        throw std::runtime_error("Error reading zone file '" + file_path.string() + "' struct"); // LCOV_EXCL_LINE
      if (contents.size() < (sizeof(tzhead)       /* struct tzhead */ + timecnt * stored   /* ats */ + timecnt        /* types */ + typecnt * 6        /* ttinfos */ + charcnt        /* chars */ + leapcnt * (stored + 4) /* lsinfos */ + ttisstdcnt     /* ttisstds */ + ttisgmtcnt))       /* ttisgmts */
        throw std::runtime_error("Error reading zone file '" + file_path.string() + "' struct"); // LCOV_EXCL_LINE

      transitions.reserve(timecnt);
      transitions.resize(0);
      for (int i = 0; i < timecnt; ++i) {
          int_fast64_t at = stored == 4 ? detzcode(ptr) : detzcode64(ptr);
          if(at <= time_t_max) {
            transitions.push_back(((TYPE_SIGNED(time_t) ? at < time_t_min : at < 0) ? time_t_min : at));
          }
          ptr += stored;
      }

      transition_types.reserve(timecnt);
      transition_types.resize(0);
      for(int i=0; i<timecnt; ++i) {
          unsigned char typ = *ptr++;
          if (typecnt <= typ)
            throw std::runtime_error("Error reading zone file '" + file_path.string() + "' struct"); // LCOV_EXCL_LINE
          transition_types.push_back(typ);
      }

      types.reserve(typecnt);
      for(int i = 0; i < typecnt; ++i) {
          /* gmt offset */
          int offset = detzcode(ptr);
          ptr += 4;
          /* dst */
          if(!(*ptr < 2))
            throw std::runtime_error("Error reading zone file '" + file_path.string() + "' struct"); // LCOV_EXCL_LINE
          bool dst = *ptr;
          ptr++;
          /* abbr */
          short abbrind = *ptr++;
          if (! (abbrind < charcnt))
            throw std::runtime_error("Error reading zone file '" + file_path.string() + "' struct"); // LCOV_EXCL_LINE
          types.push_back(std::make_tuple(-offset, dst, abbrind));
      }

      abbr = ptr;
      ptr += charcnt;
      ptr += leapcnt * 2 * stored;
      ptr += typecnt; // tt_ttisstd
      ptr += typecnt; // tt_ttisgmt

      if (th->tzh_version[0] == '\0')
          break; // LCOV_EXCL_LINE

      // get ready for 8 bytes
      th_union = reinterpret_cast<const th_union_t*>(ptr);
      th = &th_union->head;
    }

    time_zone this_tz(name);
    for(std::size_t i=0; i<transitions.size(); ++i) {
      this_tz.add_entry(transitions[i] * 1000000, time_zone_entry_info(std::get<0>(types[transition_types[i]]), std::string(abbr + std::get<2>(types[transition_types[i]])), std::get<1>(types[transition_types[i]])));
    }

    return this_tz;
    #undef TYPE_SIGNED
  }
  #endif //USE_ZONEINFO

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
    auto match = _data.lower_bound(p);
    if(match->first != p)
    match--;
    return &match->second;
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
      
      if(z->offset.is_negative())
        ss << '+' << std::setfill('0') << std::setw(2) << (-h) << std::setw(2) << m;
      else
        ss << '-' << std::setfill('0') << std::setw(2) << h << std::setw(2) << m;
      if(s)
        ss << std::setw(2) << s;
    }
    return ss.str();
  }
  
  #ifdef USE_ZONEINFO
  static int_fast32_t detzcode(const char *const codep) {
      int_fast32_t  result;
      int           i;

      result = (codep[0] & 0x80) ? -1 : 0;
      for (i = 0; i < 4; ++i)
          result = (result << 8) | (codep[i] & 0xff);
      return result;
  }

  static int_fast64_t detzcode64(const char *const codep) {
      register int_fast64_t result;
      register int    i;

      result = (codep[0] & 0x80) ? -1 : 0;
      for (i = 0; i < 8; ++i)
          result = (result << 8) | (codep[i] & 0xff);
      return result;
  }
  #endif //USE_ZONEINFO

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
