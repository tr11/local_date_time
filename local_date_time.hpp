#ifndef LOCAL_DATE_TIME_LOCAL_DATE_TIME_HPP
#define LOCAL_DATE_TIME_LOCAL_DATE_TIME_HPP

#include "timezone.hpp"

namespace local_time {  

class local_date_time { 
  
public:
  local_date_time(const ptime& utc, time_zone_const_ptr tz) : _utc(utc), _tz(tz) { }
  
  local_date_time(const boost::gregorian::date& d, const time_duration& td, time_zone_const_ptr tz, time_zone::automatic_conversion dst = time_zone::automatic_conversion::THROW_ON_AMBIGUOUS) : _tz(tz) { 
    if(tz)
      _utc = tz->local_to_utc(ptime(d, td), dst);
    else
      _utc = ptime(d, td);
  }

  local_date_time(const local_date_time& other) : _utc(other._utc), _tz(other._tz) { }

  local_date_time(local_date_time&& other) : _utc(std::move(other._utc)), _tz(std::move(other._tz)) { }
  
  local_date_time(boost::posix_time::special_values sv, time_zone_const_ptr tz) : _utc(sv), _tz(tz) { }
  
  const time_zone_const_ptr zone() const { return _tz; }
  
  bool is_dst() {
    if(!_tz) return false;
    const time_zone_entry_info* z = _tz->zone_info_from_utc(_utc);
    return z && z->dst;
  }
  
  ptime utc_time() const { return _utc; }
  
  ptime local_time() const { 
    if(_tz)
      return _tz->utc_to_local(_utc);
    else 
      return _utc;
  }
  
  local_date_time local_time_in(time_zone_const_ptr tz, time_duration td=time_duration(0,0,0)) { 
    return local_date_time(_utc + td, tz); 
  }
  
  bool is_infinity() const { return _utc.is_infinity(); }

  bool is_neg_infinity() const { return _utc.is_neg_infinity(); }
  
  bool is_pos_infinity() const { return _utc.is_pos_infinity(); }
  
  bool is_not_a_date_time() const { return _utc.is_not_a_date_time(); }
  
  bool is_special() const { return _utc.is_special(); }

  bool operator== (const local_date_time& rhs) const { return _utc == rhs._utc;}
  
  bool operator!= (const local_date_time& rhs) const { return _utc != rhs._utc;}
  
  bool operator> (const local_date_time& rhs) const { return _utc > rhs._utc;}
  
  bool operator< (const local_date_time& rhs) const { return _utc < rhs._utc;}
  
  bool operator>= (const local_date_time& rhs) const { return _utc >= rhs._utc;}
  
  bool operator<= (const local_date_time& rhs) const { return _utc <= rhs._utc;}

  bool operator> (const ptime& rhs) const { return _utc > rhs;}
  
  bool operator< (const ptime& rhs) const { return _utc < rhs;}
  
  bool operator>= (const ptime& rhs) const { return _utc >= rhs;}
  
  bool operator<= (const ptime& rhs) const { return _utc <= rhs;}

  local_date_time operator+ (const boost::gregorian::days& d) const { return local_date_time(_utc + d, _tz); }
  
  local_date_time& operator+= (const boost::gregorian::days& d) { _utc += d; return *this; }
  
  local_date_time operator- (const boost::gregorian::days& d) const { return local_date_time(_utc - d, _tz); }
  
  local_date_time& operator-= (const boost::gregorian::days& d) { _utc -= d; return *this; }

  local_date_time operator+ (const boost::gregorian::months& m) const { return local_date_time(_utc + m, _tz); }
  
  local_date_time& operator+= (const boost::gregorian::months& m) { _utc += m; return *this; }
  
  local_date_time operator- (const boost::gregorian::months& m) const { return local_date_time(_utc - m, _tz); }
  
  local_date_time& operator-= (const boost::gregorian::months& m) { _utc -= m; return *this; }

  local_date_time operator+ (const boost::gregorian::years& y) const { return local_date_time(_utc + y, _tz); }
  
  local_date_time& operator+= (const boost::gregorian::years& y) { _utc += y; return *this; }
  
  local_date_time operator- (const boost::gregorian::years& y) const { return local_date_time(_utc - y, _tz); }
  
  local_date_time& operator-= (const boost::gregorian::years& y) { _utc -= y; return *this; }

  local_date_time operator+ (const boost::posix_time::time_duration& t) const { return local_date_time(_utc + t, _tz); }
  
  local_date_time& operator+= (const boost::posix_time::time_duration& t) { _utc += t; return *this; }
  
  local_date_time operator- (const boost::posix_time::time_duration& t) const { return local_date_time(_utc - t, _tz); }
  
  local_date_time& operator-= (const boost::posix_time::time_duration& t) { _utc -= t; return *this; }

  time_duration operator- (const boost::posix_time::ptime& p) {  return _utc - p; }
  
  time_duration operator- (const local_date_time& ldt) {  return _utc - ldt._utc; }
  
  friend std::ostream& operator<< (std::ostream& out, const local_date_time& ldt) {
    out << ldt.to_string();
    return out;
  }
  
  std::string to_string() const { 
    if(_tz)
      return _tz->utc_to_local_string(_utc);
    else
      return boost::posix_time::to_iso_string(_utc);
  }
  
  std::string to_iso_string() const {
    if(_tz)
      return _tz->utc_to_local_iso_string(_utc);
    else
      return boost::posix_time::to_iso_string(_utc);
  }
  
private:
  ptime                 _utc;
  time_zone_const_ptr   _tz;
};

}

#endif
