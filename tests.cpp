
#define BOOST_TEST_MODULE local_date_time_tests
#include <boost/test/unit_test.hpp>

#include "local_date_time.hpp"
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

using namespace local_time;

const std::map<std::string, std::vector<std::tuple<int64_t, long, std::string, bool> > > zones_struct_simple  {
  { "TZ_1", { std::tuple<int64_t, long, std::string, bool> {0, 0, "EST", 0},                      // 1970/01/01 00:00:00
              std::tuple<int64_t, long, std::string, bool> {3600LL*24*1000000, 3600, "DST", 1}    // 1970/01/02 00:00:00
            } 
  },
  { "TZ_2", { std::tuple<int64_t, long, std::string, bool> {0, 0, "EST", 0},                      // 1970/01/01 00:00:00
              std::tuple<int64_t, long, std::string, bool> {3600LL*24*1000000, -3600, "DST", 1}   // 1970/01/02 00:00:00
            } 
  },
  { "TZ_3", { std::tuple<int64_t, long, std::string, bool> {0, 0, "DST", 1},                      // 1970/01/01 00:00:00
              std::tuple<int64_t, long, std::string, bool> {3600LL*24*1000000, 3600, "EST", 0}    // 1970/01/02 00:00:00
            } 
  },
  { "TZ_4", { std::tuple<int64_t, long, std::string, bool> {0, 0, "EST1", 0},                      // 1970/01/01 00:00:00
              std::tuple<int64_t, long, std::string, bool> {3600LL*24*1000000, 3600, "EST2", 0}    // 1970/01/02 00:00:00
            } 
  },
  { "TZ_5", { std::tuple<int64_t, long, std::string, bool> {0, 0, "EST", 1},                      // 1970/01/01 00:00:00
              std::tuple<int64_t, long, std::string, bool> {3600LL*24*1000000, -3600, "DST", 0}   // 1970/01/02 00:00:00
            } 
  },
  { "TZ_6", { std::tuple<int64_t, long, std::string, bool> {0, 0, "EST", 0},                      // 1970/01/01 00:00:00
              std::tuple<int64_t, long, std::string, bool> {3600LL*24*1000000, -3600, "DST", 0}   // 1970/01/02 00:00:00
            } 
  },
  
};


BOOST_AUTO_TEST_CASE(test_simple_init) {
  { // empty database
    auto db = time_zone_database();
    std::set<std::string> region_list = db.region_list();
    std::set<std::string> expected_region_list;
    BOOST_CHECK_EQUAL_COLLECTIONS(region_list.begin(), region_list.end(), expected_region_list.begin(), expected_region_list.end());
  }
  { // a few zones
    auto db = time_zone_database::from_struct(zones_struct_simple);
    std::set<std::string> region_list = db.region_list();
    std::set<std::string> expected_region_list = { "TZ_1", "TZ_2", "TZ_3", "TZ_4", "TZ_5", "TZ_6" };
    BOOST_CHECK_EQUAL_COLLECTIONS(region_list.begin(), region_list.end(), expected_region_list.begin(), expected_region_list.end());
  }
  
  // create an empty timezone
  time_zone_ptr tz(new time_zone(""));
  BOOST_CHECK_EQUAL(tz->name(), "");
  
  // check that the timezone has no info
  ptime p(boost::gregorian::date(2000, 1, 1));
  local_date_time ldt(p, tz);
  BOOST_CHECK_EQUAL(ldt.utc_time(), p);
  BOOST_CHECK_EQUAL(ldt.local_time(), p);

  // test basic operators
  BOOST_CHECK_EQUAL(ldt - p, time_duration());

  BOOST_CHECK( !(ldt < p) );
  BOOST_CHECK( !(ldt > p) );
  BOOST_CHECK( ldt <= p );
  BOOST_CHECK( ldt >= p );
}


BOOST_AUTO_TEST_CASE(test_constructors) {
  { // from struct
    time_zone_database tzdb( time_zone_database::from_struct(zones_struct_simple) );
    time_zone_const_ptr tz = tzdb.time_zone_from_region("TZ_1");
    BOOST_CHECK_EQUAL(tz->name(), "TZ_1");
  }
}

BOOST_AUTO_TEST_CASE(test_logic_nullptr) {
  time_zone_ptr tz;
  boost::gregorian::date d(2000, 1, 1);
  local_date_time ldt1(d, time_duration(0,0,0), tz);
  BOOST_CHECK_EQUAL(ldt1.utc_time().date(), d);
  BOOST_CHECK_EQUAL(ldt1.utc_time().time_of_day(), time_duration(0,0,0));  
}

BOOST_AUTO_TEST_CASE(test_logic_nullptr_2) {
  time_zone_database tzdb;
  BOOST_CHECK_EQUAL(tzdb.time_zone_from_region("ABCDEF"), time_zone_const_ptr());
}

BOOST_AUTO_TEST_CASE(test_logic_0) {
  time_zone_ptr tz(new time_zone("test"));
  boost::gregorian::date d(2000, 1, 1);
  local_date_time ldt1(d, time_duration(0,0,0), tz);
  
  BOOST_CHECK_EQUAL(ldt1.utc_time().date(), d);
  BOOST_CHECK_EQUAL(ldt1.utc_time().time_of_day(), time_duration(0,0,0));

  tz->add_entry(0, time_zone_entry_info(7200, "ABC", false));
  local_date_time ldt2(d, time_duration(0,0,0), tz);
  
  BOOST_CHECK_EQUAL(ldt2.utc_time().date(), d);
  BOOST_CHECK_EQUAL(ldt2.utc_time().time_of_day(), time_duration(2,0,0));
  
}

BOOST_AUTO_TEST_CASE(test_logic_1) {
  {
    time_zone_database tzdb( time_zone_database::from_struct(zones_struct_simple) );
    time_zone_const_ptr tz = tzdb.time_zone_from_region("TZ_1");
    
    { // check before the first entry
      ptime p(boost::gregorian::date(1900, 1, 1));
      local_date_time ldt(p, tz);
      BOOST_CHECK_EQUAL(ldt.utc_time(), p);
      BOOST_CHECK_EQUAL(ldt.local_time().date(), p.date());
      BOOST_CHECK_EQUAL(ldt.local_time().time_of_day(), p.time_of_day());
    }  
    { // check before the first change
      ptime p(boost::gregorian::date(1970, 1, 1));
      local_date_time ldt(p, tz);
      BOOST_CHECK_EQUAL(ldt.utc_time(), p);
      BOOST_CHECK_EQUAL(ldt.local_time().date(), p.date());
      BOOST_CHECK_EQUAL(ldt.local_time().time_of_day(), p.time_of_day());
    }  
    { // check after the first change
      ptime p(boost::gregorian::date(1970, 2, 1));
      local_date_time ldt(p, tz);
      BOOST_CHECK_EQUAL(ldt.utc_time(), p);
      BOOST_CHECK_EQUAL(ldt.local_time().date(), p.date() - boost::gregorian::days(1));
      BOOST_CHECK_EQUAL(ldt.local_time().time_of_day(), boost::posix_time::time_duration(23,0,0));
    }  
  }  
}


BOOST_AUTO_TEST_CASE(test_logic_3) {
  time_zone_database tzdb( time_zone_database::from_struct(zones_struct_simple) );
  time_zone_const_ptr tz1 = tzdb.time_zone_from_region("TZ_1");
  time_zone_const_ptr tz2 = tzdb.time_zone_from_region("TZ_2");
  ptime p1(boost::gregorian::date(2000, 1, 1));
  local_date_time ldt(p1, tz1);
  
  BOOST_CHECK_EQUAL(ldt.local_time_in(tz2).local_time(), p1 + boost::posix_time::hours(1));
  
}  


BOOST_AUTO_TEST_CASE(test_local_date_time_constructors) {
  time_zone_database tzdb( time_zone_database::from_struct(zones_struct_simple) );
  
  {
    time_zone_const_ptr tz = tzdb.time_zone_from_region("TZ_1");
    ptime p1(boost::gregorian::date(2000, 1, 1));
    local_date_time ldt1(p1, tz);
    local_date_time ldt2(ldt1);
    
    BOOST_CHECK_EQUAL(ldt1, ldt2);
    BOOST_CHECK_EQUAL(ldt1.zone(), ldt2.zone());
    BOOST_CHECK_EQUAL(ldt1.zone(), tz);
    
    local_date_time ldt3(std::move(ldt2));
    BOOST_CHECK_EQUAL(ldt1, ldt3);
    BOOST_CHECK_EQUAL(ldt1.zone(), ldt3.zone());
  }
  
  {  
    time_zone_const_ptr tz = tzdb.time_zone_from_region("TZ_1");
    ptime p1(boost::gregorian::date(1970, 1, 1));
    local_date_time ldt1(p1, tz);
    BOOST_CHECK_EQUAL(ldt1.local_time().date(), p1.date());
    BOOST_CHECK_EQUAL(ldt1.local_time().time_of_day(), time_duration(0,0,0));

    ptime p2(boost::gregorian::date(1970, 2, 1));
    local_date_time ldt2(p2, tz);
    BOOST_CHECK_EQUAL(ldt2.local_time().date(), p2.date() - boost::gregorian::days(1));
    BOOST_CHECK_EQUAL(ldt2.local_time().time_of_day(), time_duration(23,0,0));
  }
  { // segments intersect
    time_zone_const_ptr tz = tzdb.time_zone_from_region("TZ_1");
    auto d1 = boost::gregorian::date(1970, 1, 1);
    auto d2 = boost::gregorian::date(1970, 1, 2);
    { // before the first element
      local_date_time ldt1(boost::gregorian::date(1960, 1, 1), time_duration(5,0,0), tz);
      BOOST_CHECK_EQUAL(ldt1.utc_time().date(), boost::gregorian::date(1960, 1, 1));
      BOOST_CHECK_EQUAL(ldt1.utc_time().time_of_day(), time_duration(5,0,0));
    }
    { // before the change
      local_date_time ldt1(d1, time_duration(5,0,0), tz);
      BOOST_CHECK_EQUAL(ldt1.utc_time().date(), d1);
      BOOST_CHECK_EQUAL(ldt1.utc_time().time_of_day(), time_duration(5,0,0));
    }
    { // right before the change
      time_duration td(23,0,0);
      td -= boost::posix_time::microseconds(1);
      local_date_time ldt1(d1, td , tz);
      BOOST_CHECK_EQUAL(ldt1.utc_time().date(), d1);
      BOOST_CHECK_EQUAL(ldt1.utc_time().time_of_day(), td);
    }
    { // during an ambiguous period 
      time_duration td(23,0,0);
      BOOST_CHECK_THROW( local_date_time(d1, td , tz), local_time::ambiguous_result);
      BOOST_CHECK_THROW( local_date_time(d1, td + boost::posix_time::minutes(30), tz), local_time::ambiguous_result);
      BOOST_CHECK_THROW( local_date_time(d1, time_duration(24,0,0) - boost::posix_time::microseconds(1), tz), local_time::ambiguous_result);
    }
    { // right after the change
      local_date_time ldt2(d2, time_duration(0,0,0), tz);
      BOOST_CHECK_EQUAL(ldt2.utc_time().date(), d2);
      BOOST_CHECK_EQUAL(ldt2.utc_time().time_of_day(), time_duration(1,0,0));
    }
    { // after the change
      local_date_time ldt2(d2, time_duration(0,0,0) + boost::posix_time::microseconds(1), tz);
      BOOST_CHECK_EQUAL(ldt2.utc_time().date(), d2);
      BOOST_CHECK_EQUAL(ldt2.utc_time().time_of_day(), time_duration(1,0,0, 1));
    }
  }
  { // segments are disjoint
    time_zone_const_ptr tz = tzdb.time_zone_from_region("TZ_2");
    auto d1 = boost::gregorian::date(1970, 1, 1);
    auto d2 = boost::gregorian::date(1970, 1, 2);
    { // before the first element
      local_date_time ldt1(boost::gregorian::date(1960, 1, 1), time_duration(5,0,0), tz);
      BOOST_CHECK_EQUAL(ldt1.utc_time().date(), boost::gregorian::date(1960, 1, 1));
      BOOST_CHECK_EQUAL(ldt1.utc_time().time_of_day(), time_duration(5,0,0));
    }
    { // before the change
      local_date_time ldt1(d1, time_duration(5,0,0), tz);
      BOOST_CHECK_EQUAL(ldt1.utc_time().date(), d1);
      BOOST_CHECK_EQUAL(ldt1.utc_time().time_of_day(), time_duration(5,0,0));
    }
    { // right before the change
      time_duration td(24,0,0);
      td -= boost::posix_time::microseconds(1);
      local_date_time ldt1(d1, td , tz);
      BOOST_CHECK_EQUAL(ldt1.utc_time().date(), d1);
      BOOST_CHECK_EQUAL(ldt1.utc_time().time_of_day(), td);
    }
    { // during an invalid segment
      time_duration td(0,0,0);
      BOOST_CHECK_THROW( local_date_time(d2, td , tz), local_time::time_label_invalid);
      BOOST_CHECK_THROW( local_date_time(d2, td + boost::posix_time::minutes(30), tz), local_time::time_label_invalid);
      BOOST_CHECK_THROW( local_date_time(d2, time_duration(1,0,0) - boost::posix_time::microseconds(1), tz), local_time::time_label_invalid);
    }
    { // right after the change
      local_date_time ldt2(d2, time_duration(1,0,0), tz);
      BOOST_CHECK_EQUAL(ldt2.utc_time().date(), d2);
      BOOST_CHECK_EQUAL(ldt2.utc_time().time_of_day(), time_duration(0,0,0));
    }
    { // after the change
      local_date_time ldt2(d2, time_duration(1,0,0) + boost::posix_time::microseconds(1), tz);
      BOOST_CHECK_EQUAL(ldt2.utc_time().date(), d2);
      BOOST_CHECK_EQUAL(ldt2.utc_time().time_of_day(), time_duration(0,0,0, 1));
    }
  }

  { // segments intersect -- no throw, assume dst
    time_zone_const_ptr tz = tzdb.time_zone_from_region("TZ_1");
    time_zone_const_ptr tz3 = tzdb.time_zone_from_region("TZ_3");
    time_zone_const_ptr tz4 = tzdb.time_zone_from_region("TZ_4");
    auto d1 = boost::gregorian::date(1970, 1, 1);
    auto d2 = boost::gregorian::date(1970, 1, 2);
    { // before the first element
      local_date_time ldt1(boost::gregorian::date(1960, 1, 1), time_duration(5,0,0), tz, time_zone::automatic_conversion::ASSUME_DST);
      BOOST_CHECK_EQUAL(ldt1.utc_time().date(), boost::gregorian::date(1960, 1, 1));
      BOOST_CHECK_EQUAL(ldt1.utc_time().time_of_day(), time_duration(5,0,0));
    }
    { // before the change
      local_date_time ldt1(d1, time_duration(5,0,0), tz, time_zone::automatic_conversion::ASSUME_DST);
      BOOST_CHECK_EQUAL(ldt1.utc_time().date(), d1);
      BOOST_CHECK_EQUAL(ldt1.utc_time().time_of_day(), time_duration(5,0,0));
    }
    { // right before the change
      time_duration td(23,0,0);
      td -= boost::posix_time::microseconds(1);
      local_date_time ldt1(d1, td , tz, time_zone::automatic_conversion::ASSUME_DST);
      BOOST_CHECK_EQUAL(ldt1.utc_time().date(), d1);
      BOOST_CHECK_EQUAL(ldt1.utc_time().time_of_day(), td);
    }
    { // during an ambiguous period 
      time_duration td(23,0,0);
      BOOST_CHECK_EQUAL( local_date_time(d1, td , tz, time_zone::automatic_conversion::ASSUME_DST).utc_time(), 
                         boost::posix_time::ptime(d2) );
      BOOST_CHECK_EQUAL( local_date_time(d1, td + boost::posix_time::minutes(30), tz, time_zone::automatic_conversion::ASSUME_DST).utc_time(), 
                         boost::posix_time::ptime(d2) + boost::posix_time::minutes(30));
      BOOST_CHECK_EQUAL( local_date_time(d1, time_duration(24,0,0) - boost::posix_time::microseconds(1), tz, time_zone::automatic_conversion::ASSUME_DST).utc_time(), 
                         boost::posix_time::ptime(d2) + boost::posix_time::hours(1) - boost::posix_time::microseconds(1));

      BOOST_CHECK_EQUAL( local_date_time(d1, td , tz3, time_zone::automatic_conversion::ASSUME_DST).utc_time(), 
                         boost::posix_time::ptime(d1) + td );
      BOOST_CHECK_EQUAL( local_date_time(d1, td + boost::posix_time::minutes(30), tz3, time_zone::automatic_conversion::ASSUME_DST).utc_time(), 
                         boost::posix_time::ptime(d1) + td + boost::posix_time::minutes(30));
      BOOST_CHECK_EQUAL( local_date_time(d1, time_duration(24,0,0) - boost::posix_time::microseconds(1), tz3, time_zone::automatic_conversion::ASSUME_DST).utc_time(), 
                         boost::posix_time::ptime(d2) - boost::posix_time::microseconds(1));

      BOOST_CHECK_THROW( local_date_time(d1, td , tz4, time_zone::automatic_conversion::ASSUME_DST).utc_time(), 
                         local_time::ambiguous_result );
      BOOST_CHECK_THROW( local_date_time(d1, td + boost::posix_time::minutes(30), tz4, time_zone::automatic_conversion::ASSUME_DST).utc_time(), 
                         local_time::ambiguous_result );
      BOOST_CHECK_THROW( local_date_time(d1, time_duration(24,0,0) - boost::posix_time::microseconds(1), tz4, time_zone::automatic_conversion::ASSUME_DST).utc_time(), 
                         local_time::ambiguous_result );
    }
    { // right after the change
      local_date_time ldt2(d2, time_duration(0,0,0), tz, time_zone::automatic_conversion::ASSUME_DST);
      BOOST_CHECK_EQUAL(ldt2.utc_time().date(), d2);
      BOOST_CHECK_EQUAL(ldt2.utc_time().time_of_day(), time_duration(1,0,0));
    }
    { // after the change
      local_date_time ldt2(d2, time_duration(0,0,0) + boost::posix_time::microseconds(1), tz, time_zone::automatic_conversion::ASSUME_DST);
      BOOST_CHECK_EQUAL(ldt2.utc_time().date(), d2);
      BOOST_CHECK_EQUAL(ldt2.utc_time().time_of_day(), time_duration(1,0,0, 1));
    }
  }
  { // segments are disjoint -- no throw, assume dst
    time_zone_const_ptr tz = tzdb.time_zone_from_region("TZ_2");
    time_zone_const_ptr tz3 = tzdb.time_zone_from_region("TZ_5");
    time_zone_const_ptr tz4 = tzdb.time_zone_from_region("TZ_6");
    auto d1 = boost::gregorian::date(1970, 1, 1);
    auto d2 = boost::gregorian::date(1970, 1, 2);
    { // before the first element
      local_date_time ldt1(boost::gregorian::date(1960, 1, 1), time_duration(5,0,0), tz, time_zone::automatic_conversion::ASSUME_DST);
      BOOST_CHECK_EQUAL(ldt1.utc_time().date(), boost::gregorian::date(1960, 1, 1));
      BOOST_CHECK_EQUAL(ldt1.utc_time().time_of_day(), time_duration(5,0,0));
    }
    { // before the change
      local_date_time ldt1(d1, time_duration(5,0,0), tz, time_zone::automatic_conversion::ASSUME_DST);
      BOOST_CHECK_EQUAL(ldt1.utc_time().date(), d1);
      BOOST_CHECK_EQUAL(ldt1.utc_time().time_of_day(), time_duration(5,0,0));
    }
    { // right before the change
      time_duration td(24,0,0);
      td -= boost::posix_time::microseconds(1);
      local_date_time ldt1(d1, td , tz, time_zone::automatic_conversion::ASSUME_DST);
      BOOST_CHECK_EQUAL(ldt1.utc_time().date(), d1);
      BOOST_CHECK_EQUAL(ldt1.utc_time().time_of_day(), td);
    }
    { // during an invalid segment
      time_duration td(0,0,0);
      BOOST_CHECK_EQUAL( local_date_time(d2, td , tz, time_zone::automatic_conversion::ASSUME_DST).utc_time(), 
                         boost::posix_time::ptime(d2) - boost::posix_time::hours(1) );
      BOOST_CHECK_EQUAL( local_date_time(d2, td + boost::posix_time::minutes(30), tz, time_zone::automatic_conversion::ASSUME_DST).utc_time(),
                         boost::posix_time::ptime(d2) - boost::posix_time::minutes(30) );
      BOOST_CHECK_EQUAL( local_date_time(d2, time_duration(1,0,0) - boost::posix_time::microseconds(1), tz, time_zone::automatic_conversion::ASSUME_DST).utc_time(),
                         boost::posix_time::ptime(d2) - boost::posix_time::microseconds(1) );

      BOOST_CHECK_EQUAL( local_date_time(d2, td , tz3, time_zone::automatic_conversion::ASSUME_DST).utc_time(), 
                         boost::posix_time::ptime(d2));
      BOOST_CHECK_EQUAL( local_date_time(d2, td + boost::posix_time::minutes(30), tz3, time_zone::automatic_conversion::ASSUME_DST).utc_time(),
                         boost::posix_time::ptime(d2) + boost::posix_time::hours(1) - boost::posix_time::minutes(30) );
      BOOST_CHECK_EQUAL( local_date_time(d2, time_duration(1,0,0) - boost::posix_time::microseconds(1), tz3, time_zone::automatic_conversion::ASSUME_DST).utc_time(),
                         boost::posix_time::ptime(d2) + boost::posix_time::hours(1) - boost::posix_time::microseconds(1) );

      BOOST_CHECK_THROW( local_date_time(d2, td , tz4, time_zone::automatic_conversion::ASSUME_DST), local_time::time_label_invalid);
      BOOST_CHECK_THROW( local_date_time(d2, td + boost::posix_time::minutes(30), tz4, time_zone::automatic_conversion::ASSUME_DST), local_time::time_label_invalid);
      BOOST_CHECK_THROW( local_date_time(d2, time_duration(1,0,0) - boost::posix_time::microseconds(1), tz4, time_zone::automatic_conversion::ASSUME_DST), local_time::time_label_invalid);      
    }
    { // right after the change
      local_date_time ldt2(d2, time_duration(1,0,0), tz, time_zone::automatic_conversion::ASSUME_DST);
      BOOST_CHECK_EQUAL(ldt2.utc_time().date(), d2);
      BOOST_CHECK_EQUAL(ldt2.utc_time().time_of_day(), time_duration(0,0,0));
    }
    { // after the change
      local_date_time ldt2(d2, time_duration(1,0,0) + boost::posix_time::microseconds(1), tz, time_zone::automatic_conversion::ASSUME_DST);
      BOOST_CHECK_EQUAL(ldt2.utc_time().date(), d2);
      BOOST_CHECK_EQUAL(ldt2.utc_time().time_of_day(), time_duration(0,0,0, 1));
    }
  }
  
  { // segments intersect -- no throw, assume non-dst
    time_zone_const_ptr tz = tzdb.time_zone_from_region("TZ_1");
    time_zone_const_ptr tz3 = tzdb.time_zone_from_region("TZ_3");
    time_zone_const_ptr tz4 = tzdb.time_zone_from_region("TZ_4");
    auto d1 = boost::gregorian::date(1970, 1, 1);
    auto d2 = boost::gregorian::date(1970, 1, 2);
    { // before the first element
      local_date_time ldt1(boost::gregorian::date(1960, 1, 1), time_duration(5,0,0), tz, time_zone::automatic_conversion::ASSUME_NON_DST);
      BOOST_CHECK_EQUAL(ldt1.utc_time().date(), boost::gregorian::date(1960, 1, 1));
      BOOST_CHECK_EQUAL(ldt1.utc_time().time_of_day(), time_duration(5,0,0));
    }
    { // before the change
      local_date_time ldt1(d1, time_duration(5,0,0), tz, time_zone::automatic_conversion::ASSUME_NON_DST);
      BOOST_CHECK_EQUAL(ldt1.utc_time().date(), d1);
      BOOST_CHECK_EQUAL(ldt1.utc_time().time_of_day(), time_duration(5,0,0));
    }
    { // right before the change
      time_duration td(23,0,0);
      td -= boost::posix_time::microseconds(1);
      local_date_time ldt1(d1, td , tz, time_zone::automatic_conversion::ASSUME_NON_DST);
      BOOST_CHECK_EQUAL(ldt1.utc_time().date(), d1);
      BOOST_CHECK_EQUAL(ldt1.utc_time().time_of_day(), td);
    }
    { // during an ambiguous period 
      time_duration td(23,0,0);
      BOOST_CHECK_EQUAL( local_date_time(d1, td , tz, time_zone::automatic_conversion::ASSUME_NON_DST).utc_time(), 
                         boost::posix_time::ptime(d2) - boost::posix_time::hours(1));
      BOOST_CHECK_EQUAL( local_date_time(d1, td + boost::posix_time::minutes(30), tz, time_zone::automatic_conversion::ASSUME_NON_DST).utc_time(), 
                         boost::posix_time::ptime(d2) - boost::posix_time::hours(1) + boost::posix_time::minutes(30));
      BOOST_CHECK_EQUAL( local_date_time(d1, time_duration(24,0,0) - boost::posix_time::microseconds(1), tz, time_zone::automatic_conversion::ASSUME_NON_DST).utc_time(), 
                         boost::posix_time::ptime(d2) - boost::posix_time::microseconds(1));

      BOOST_CHECK_EQUAL( local_date_time(d1, td , tz3, time_zone::automatic_conversion::ASSUME_NON_DST).utc_time(), 
                         boost::posix_time::ptime(d1) + boost::posix_time::hours(1) + td );
      BOOST_CHECK_EQUAL( local_date_time(d1, td + boost::posix_time::minutes(30), tz3, time_zone::automatic_conversion::ASSUME_NON_DST).utc_time(), 
                         boost::posix_time::ptime(d1) + td + boost::posix_time::hours(1) + boost::posix_time::minutes(30));
      BOOST_CHECK_EQUAL( local_date_time(d1, time_duration(24,0,0) - boost::posix_time::microseconds(1), tz3, time_zone::automatic_conversion::ASSUME_NON_DST).utc_time(), 
                         boost::posix_time::ptime(d2) + boost::posix_time::hours(1) - boost::posix_time::microseconds(1));

      BOOST_CHECK_THROW( local_date_time(d1, td , tz4, time_zone::automatic_conversion::ASSUME_NON_DST).utc_time(), 
                         local_time::ambiguous_result );
      BOOST_CHECK_THROW( local_date_time(d1, td + boost::posix_time::minutes(30), tz4, time_zone::automatic_conversion::ASSUME_NON_DST).utc_time(), 
                         local_time::ambiguous_result );
      BOOST_CHECK_THROW( local_date_time(d1, time_duration(24,0,0) - boost::posix_time::microseconds(1), tz4, time_zone::automatic_conversion::ASSUME_NON_DST).utc_time(), 
                         local_time::ambiguous_result );
    }
    { // right after the change
      local_date_time ldt2(d2, time_duration(0,0,0), tz, time_zone::automatic_conversion::ASSUME_NON_DST);
      BOOST_CHECK_EQUAL(ldt2.utc_time().date(), d2);
      BOOST_CHECK_EQUAL(ldt2.utc_time().time_of_day(), time_duration(1,0,0));
    }
    { // after the change
      local_date_time ldt2(d2, time_duration(0,0,0) + boost::posix_time::microseconds(1), tz, time_zone::automatic_conversion::ASSUME_NON_DST);
      BOOST_CHECK_EQUAL(ldt2.utc_time().date(), d2);
      BOOST_CHECK_EQUAL(ldt2.utc_time().time_of_day(), time_duration(1,0,0, 1));
    }
  }
  { // segments are disjoint -- no throw, assume non-dst
    time_zone_const_ptr tz = tzdb.time_zone_from_region("TZ_2");
    time_zone_const_ptr tz3 = tzdb.time_zone_from_region("TZ_5");
    time_zone_const_ptr tz4 = tzdb.time_zone_from_region("TZ_6");
    auto d1 = boost::gregorian::date(1970, 1, 1);
    auto d2 = boost::gregorian::date(1970, 1, 2);
    { // before the first element
      local_date_time ldt1(boost::gregorian::date(1960, 1, 1), time_duration(5,0,0), tz, time_zone::automatic_conversion::ASSUME_NON_DST);
      BOOST_CHECK_EQUAL(ldt1.utc_time().date(), boost::gregorian::date(1960, 1, 1));
      BOOST_CHECK_EQUAL(ldt1.utc_time().time_of_day(), time_duration(5,0,0));
    }
    { // before the change
      local_date_time ldt1(d1, time_duration(5,0,0), tz, time_zone::automatic_conversion::ASSUME_NON_DST);
      BOOST_CHECK_EQUAL(ldt1.utc_time().date(), d1);
      BOOST_CHECK_EQUAL(ldt1.utc_time().time_of_day(), time_duration(5,0,0));
    }
    { // right before the change
      time_duration td(24,0,0);
      td -= boost::posix_time::microseconds(1);
      local_date_time ldt1(d1, td , tz, time_zone::automatic_conversion::ASSUME_NON_DST);
      BOOST_CHECK_EQUAL(ldt1.utc_time().date(), d1);
      BOOST_CHECK_EQUAL(ldt1.utc_time().time_of_day(), td);
    }
    { // during an invalid segment
      time_duration td(0,0,0);
      BOOST_CHECK_EQUAL( local_date_time(d2, td , tz, time_zone::automatic_conversion::ASSUME_NON_DST).utc_time(), 
                         boost::posix_time::ptime(d2) + boost::posix_time::hours(1) - boost::posix_time::hours(1) );
      BOOST_CHECK_EQUAL( local_date_time(d2, td + boost::posix_time::minutes(30), tz, time_zone::automatic_conversion::ASSUME_NON_DST).utc_time(),
                         boost::posix_time::ptime(d2) + boost::posix_time::hours(1) - boost::posix_time::minutes(30) );
      BOOST_CHECK_EQUAL( local_date_time(d2, time_duration(1,0,0) - boost::posix_time::microseconds(1), tz, time_zone::automatic_conversion::ASSUME_NON_DST).utc_time(),
                         boost::posix_time::ptime(d2) + boost::posix_time::hours(1) - boost::posix_time::microseconds(1) );

      BOOST_CHECK_EQUAL( local_date_time(d2, td , tz3, time_zone::automatic_conversion::ASSUME_NON_DST).utc_time(), 
                         boost::posix_time::ptime(d2) - boost::posix_time::hours(1));
      BOOST_CHECK_EQUAL( local_date_time(d2, td + boost::posix_time::minutes(30), tz3, time_zone::automatic_conversion::ASSUME_NON_DST).utc_time(),
                         boost::posix_time::ptime(d2) - boost::posix_time::minutes(30) );
      BOOST_CHECK_EQUAL( local_date_time(d2, time_duration(1,0,0) - boost::posix_time::hours(1) - boost::posix_time::microseconds(1), tz3, time_zone::automatic_conversion::ASSUME_NON_DST).utc_time(),
                         boost::posix_time::ptime(d2) - boost::posix_time::microseconds(1) );

      BOOST_CHECK_THROW( local_date_time(d2, td , tz4, time_zone::automatic_conversion::ASSUME_NON_DST), local_time::time_label_invalid);
      BOOST_CHECK_THROW( local_date_time(d2, td + boost::posix_time::minutes(30), tz4, time_zone::automatic_conversion::ASSUME_NON_DST), local_time::time_label_invalid);
      BOOST_CHECK_THROW( local_date_time(d2, time_duration(1,0,0) - boost::posix_time::microseconds(1), tz4, time_zone::automatic_conversion::ASSUME_NON_DST), local_time::time_label_invalid);      
    }
    { // right after the change
      local_date_time ldt2(d2, time_duration(1,0,0), tz, time_zone::automatic_conversion::ASSUME_NON_DST);
      BOOST_CHECK_EQUAL(ldt2.utc_time().date(), d2);
      BOOST_CHECK_EQUAL(ldt2.utc_time().time_of_day(), time_duration(0,0,0));
    }
    { // after the change
      local_date_time ldt2(d2, time_duration(1,0,0) + boost::posix_time::microseconds(1), tz, time_zone::automatic_conversion::ASSUME_NON_DST);
      BOOST_CHECK_EQUAL(ldt2.utc_time().date(), d2);
      BOOST_CHECK_EQUAL(ldt2.utc_time().time_of_day(), time_duration(0,0,0, 1));
    }
  }
  
}

BOOST_AUTO_TEST_CASE(test_local_date_time_special_values) {
  time_zone_database tzdb( time_zone_database::from_struct(zones_struct_simple) );
  time_zone_ptr tz = time_zone::duplicate(tzdb.time_zone_from_region("TZ_1"));

  ptime p(boost::gregorian::date(2000, 1, 1));
  local_date_time regular(p, tz);
  BOOST_CHECK(!regular.is_infinity());
  BOOST_CHECK(!regular.is_neg_infinity());
  BOOST_CHECK(!regular.is_pos_infinity());
  BOOST_CHECK(!regular.is_special());
  BOOST_CHECK(!regular.is_not_a_date_time());
  
  {
    local_date_time ldt(boost::posix_time::special_values::min_date_time, tz);
    BOOST_CHECK(regular > ldt);
    BOOST_CHECK(!ldt.is_dst());
    BOOST_CHECK_EQUAL(ldt.to_string(), "14000101T000000 EST");
    BOOST_CHECK(!ldt.is_infinity());
    BOOST_CHECK(!ldt.is_neg_infinity());
    BOOST_CHECK(!ldt.is_pos_infinity());
    BOOST_CHECK(!ldt.is_special());
    BOOST_CHECK(!ldt.is_not_a_date_time());
//     BOOST_CHECK_THROW(ldt - boost::gregorian::days(1) == ldt, std::out_of_range);
  }
  {
    local_date_time ldt(boost::posix_time::special_values::max_date_time, tz);
    BOOST_CHECK(regular < ldt);
    BOOST_CHECK(ldt.is_dst());
    BOOST_CHECK_EQUAL(ldt.to_string(), "99991231T225959.999999 DST");
    BOOST_CHECK(!ldt.is_infinity());
    BOOST_CHECK(!ldt.is_neg_infinity());
    BOOST_CHECK(!ldt.is_pos_infinity());
    BOOST_CHECK(!ldt.is_special());
    BOOST_CHECK(!ldt.is_not_a_date_time());
//     BOOST_CHECK_THROW(ldt + boost::gregorian::days(1) == ldt, std::out_of_range);
  }
  {
    local_date_time ldt(boost::posix_time::special_values::neg_infin, tz);
    BOOST_CHECK(ldt < regular);
    BOOST_CHECK(ldt < boost::posix_time::special_values::min_date_time);
    BOOST_CHECK(!ldt.is_dst());
    BOOST_CHECK_EQUAL(ldt.to_string(), "-infinity EST");
    BOOST_CHECK(ldt.is_infinity());
    BOOST_CHECK(ldt.is_neg_infinity());
    BOOST_CHECK(!ldt.is_pos_infinity());
    BOOST_CHECK(ldt.is_special());
    BOOST_CHECK(!ldt.is_not_a_date_time());
    BOOST_CHECK_EQUAL(ldt + boost::gregorian::days(10), ldt);
    BOOST_CHECK((regular - ldt).is_pos_infinity());
    BOOST_CHECK_EQUAL(ldt - boost::gregorian::days(1), ldt);
  }
  {
    local_date_time ldt(boost::posix_time::special_values::pos_infin, tz);
    BOOST_CHECK(ldt > regular);
    BOOST_CHECK(ldt > boost::posix_time::special_values::max_date_time);
    BOOST_CHECK(ldt.is_dst());
    BOOST_CHECK_EQUAL(ldt.to_string(), "+infinity DST");
    BOOST_CHECK(ldt.is_infinity());
    BOOST_CHECK(!ldt.is_neg_infinity());
    BOOST_CHECK(ldt.is_pos_infinity());
    BOOST_CHECK(ldt.is_special());
    BOOST_CHECK(!ldt.is_not_a_date_time());
    BOOST_CHECK_EQUAL(ldt + boost::gregorian::days(10), ldt);
    BOOST_CHECK((ldt - regular).is_pos_infinity());
    BOOST_CHECK_EQUAL(ldt + boost::gregorian::days(1), ldt);
  }
  {
    local_date_time ldt(boost::posix_time::special_values::not_a_date_time, tz);
    BOOST_CHECK(ldt > regular);
    BOOST_CHECK(ldt > boost::posix_time::special_values::max_date_time);
    BOOST_CHECK(ldt.is_dst());
    BOOST_CHECK_EQUAL(ldt.to_string(), "not-a-date-time DST");
    BOOST_CHECK(!ldt.is_infinity());
    BOOST_CHECK(!ldt.is_neg_infinity());
    BOOST_CHECK(!ldt.is_pos_infinity());
    BOOST_CHECK(ldt.is_special());
    BOOST_CHECK(ldt.is_not_a_date_time());
  }
  
}

BOOST_AUTO_TEST_CASE(test_local_date_time_operations) {
  time_zone_database tzdb( time_zone_database::from_struct(zones_struct_simple) );
  time_zone_ptr tz = time_zone::duplicate(tzdb.time_zone_from_region("TZ_1"));

  ptime p(boost::gregorian::date(1970, 1, 1), time_duration(12,7,1));
  {
    local_date_time ldt(p, tz);
    BOOST_CHECK(ldt <= ldt);
    BOOST_CHECK(ldt >= ldt);
    BOOST_CHECK(!(ldt != ldt));
  }
  {
    local_date_time ldt(p, tz);
    BOOST_CHECK_EQUAL((ldt + boost::gregorian::days(5)).utc_time(), p + boost::gregorian::days(5));
    ldt += boost::gregorian::days(5);
    BOOST_CHECK_EQUAL(ldt.utc_time(), p + boost::gregorian::days(5));
  }
  {
    local_date_time ldt(p, tz);
    BOOST_CHECK_EQUAL((ldt + boost::gregorian::months(5)).utc_time(), p + boost::gregorian::months(5));
    ldt += boost::gregorian::months(5);
    BOOST_CHECK_EQUAL(ldt.utc_time(), p + boost::gregorian::months(5));
  }
  {
    local_date_time ldt(p, tz);
    BOOST_CHECK_EQUAL((ldt + boost::gregorian::years(5)).utc_time(), p + boost::gregorian::years(5));
    ldt += boost::gregorian::years(5);
    BOOST_CHECK_EQUAL(ldt.utc_time(), p + boost::gregorian::years(5));
  }
  {
    local_date_time ldt(p, tz);
    BOOST_CHECK_EQUAL((ldt + time_duration(0,0,0,1)).utc_time(), p + time_duration(0,0,0,1));
    ldt += time_duration(0,0,0,1);
    BOOST_CHECK_EQUAL(ldt.utc_time(), p + time_duration(0,0,0,1));
  }
  {
    local_date_time ldt(p, tz);
    BOOST_CHECK_EQUAL((ldt - boost::gregorian::days(5)).utc_time(), p - boost::gregorian::days(5));
    ldt -= boost::gregorian::days(5);
    BOOST_CHECK_EQUAL(ldt.utc_time(), p - boost::gregorian::days(5));
  }  
  {
    local_date_time ldt(p, tz);
    BOOST_CHECK_EQUAL((ldt - boost::gregorian::months(5)).utc_time(), p - boost::gregorian::months(5));
    ldt -= boost::gregorian::months(5);
    BOOST_CHECK_EQUAL(ldt.utc_time(), p - boost::gregorian::months(5));
  }
  {
    local_date_time ldt(p, tz);
    BOOST_CHECK_EQUAL((ldt - boost::gregorian::years(5)).utc_time(), p - boost::gregorian::years(5));
    ldt -= boost::gregorian::years(5);
    BOOST_CHECK_EQUAL(ldt.utc_time(), p - boost::gregorian::years(5));
  }
  {
    local_date_time ldt(p, tz);
    BOOST_CHECK_EQUAL((ldt - time_duration(0,0,0,1)).utc_time(), p - time_duration(0,0,0,1));
    ldt -= time_duration(0,0,0,1);
    BOOST_CHECK_EQUAL(ldt.utc_time(), p - time_duration(0,0,0,1));
  }

  
  {
    local_date_time ldt(p, tz);
    BOOST_CHECK_EQUAL((ldt + boost::gregorian::days(5)).local_time(), p + boost::gregorian::days(5) - time_duration(1,0,0));
    ldt += boost::gregorian::days(5);
    BOOST_CHECK_EQUAL(ldt.local_time(), p + boost::gregorian::days(5) - time_duration(1,0,0));
  }
  {
    local_date_time ldt(p, tz);
    BOOST_CHECK_EQUAL((ldt + boost::gregorian::months(5)).local_time(), p + boost::gregorian::months(5) - time_duration(1,0,0));
    ldt += boost::gregorian::months(5);
    BOOST_CHECK_EQUAL(ldt.local_time(), p + boost::gregorian::months(5) - time_duration(1,0,0));
  }
  {
    local_date_time ldt(p, tz);
    BOOST_CHECK_EQUAL((ldt + boost::gregorian::years(5)).local_time(), p + boost::gregorian::years(5) - time_duration(1,0,0));
    ldt += boost::gregorian::years(5);
    BOOST_CHECK_EQUAL(ldt.local_time(), p + boost::gregorian::years(5) - time_duration(1,0,0));
  }
  {
    local_date_time ldt(p, tz);
    BOOST_CHECK_EQUAL((ldt + time_duration(0,0,0,1)).local_time(), p + time_duration(0,0,0,1));
    ldt += time_duration(0,0,0,1);
    BOOST_CHECK_EQUAL(ldt.local_time(), p + time_duration(0,0,0,1));
  }
  {
    local_date_time ldt(p, tz);
    BOOST_CHECK_EQUAL((ldt - boost::gregorian::days(5)).local_time(), p - boost::gregorian::days(5));
    ldt -= boost::gregorian::days(5);
    BOOST_CHECK_EQUAL(ldt.local_time(), p - boost::gregorian::days(5));
  }  
  {
    local_date_time ldt(p, tz);
    BOOST_CHECK_EQUAL((ldt - boost::gregorian::months(5)).local_time(), p - boost::gregorian::months(5));
    ldt -= boost::gregorian::months(5);
    BOOST_CHECK_EQUAL(ldt.local_time(), p - boost::gregorian::months(5));
  }
  {
    local_date_time ldt(p, tz);
    BOOST_CHECK_EQUAL((ldt - boost::gregorian::years(5)).local_time(), p - boost::gregorian::years(5));
    ldt -= boost::gregorian::years(5);
    BOOST_CHECK_EQUAL(ldt.local_time(), p - boost::gregorian::years(5));
  }
  {
    local_date_time ldt(p, tz);
    BOOST_CHECK_EQUAL((ldt - time_duration(0,0,0,1)).local_time(), p - time_duration(0,0,0,1));
    ldt -= time_duration(0,0,0,1);
    BOOST_CHECK_EQUAL(ldt.local_time(), p - time_duration(0,0,0,1));
  }
  
  
}


BOOST_AUTO_TEST_CASE(test_local_date_time_strings) {
  time_zone_database tzdb( time_zone_database::from_struct(zones_struct_simple) );
  time_zone_ptr tz = time_zone::duplicate(tzdb.time_zone_from_region("TZ_1"));
  
  time_zone_ptr newtz(new time_zone("A"));
  ptime p(boost::gregorian::date(2000, 1, 1));
  BOOST_CHECK_EQUAL(local_date_time(p, newtz).to_iso_string(), "20000101T000000");
  
  local_date_time ldt(p, tz);
  
  BOOST_CHECK_EQUAL(ldt.to_string(), "19991231T230000 DST");
  BOOST_CHECK_EQUAL(ldt.to_iso_string(), "19991231T230000+0100");
  
  tz->add_entry(3600LL*48*1000000, time_zone_entry_info(3601, "ABC", false));
  BOOST_CHECK_EQUAL(ldt.to_string(), "19991231T225959 ABC");
  BOOST_CHECK_EQUAL(ldt.to_iso_string(), "19991231T225959+010001");
  
  time_zone_ptr emptytz;
  local_date_time ldt2(p, emptytz);
  BOOST_CHECK_EQUAL(ldt2.to_string(), "20000101T000000");
  BOOST_CHECK_EQUAL(ldt2.to_iso_string(), "20000101T000000");
 
  std::stringstream out;
  out << ldt2;
  BOOST_CHECK_EQUAL(out.str(), "20000101T000000");
  
}


BOOST_AUTO_TEST_CASE(test_local_date_time_io) {
  // get temp path
  boost::filesystem::path path;
  while( path.empty() || boost::filesystem::exists(path) ) {
    path = boost::filesystem::temp_directory_path();
    path /= boost::filesystem::unique_path("%%%%-%%%%-%%%%-%%%%");
  }

  {
    BOOST_CHECK_THROW(time_zone_database::from_file(path.string()), std::runtime_error);
  }

  {
    time_zone_database tzdb( time_zone_database::from_struct(zones_struct_simple) );
    BOOST_CHECK(!tzdb.save_to_file(""));
  }

  {
    const std::map<std::string, std::vector<std::tuple<int64_t, long, std::string, bool> > > zones  {
      { "TZ_1", { std::tuple<int64_t, long, std::string, bool> {std::numeric_limits< int64_t >::min(), std::numeric_limits< long >::min(), "EST", 0} } },
    };
    BOOST_CHECK_THROW(time_zone_database tzdb( time_zone_database::from_struct(zones) ), std::runtime_error);
  }
  
  {
    time_zone_database tzdb( time_zone_database::from_struct(zones_struct_simple) );
    
    // save the file
    tzdb.save_to_file(path.string());
    
    // now read it
    boost::filesystem::ifstream f(path);
    std::string filestr = std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    std::string result("TZ_1,0,0,EST,0\nTZ_1,86400000000,3600,DST,1\nTZ_2,0,0,EST,0\nTZ_2,86400000000,-3600,DST,1\nTZ_3,0,0,DST,1\nTZ_3,86400000000,3600,EST,0\nTZ_4,0,0,EST1,0\nTZ_4,86400000000,3600,EST2,0\nTZ_5,0,0,EST,1\nTZ_5,86400000000,-3600,DST,0\nTZ_6,0,0,EST,0\nTZ_6,86400000000,-3600,DST,0\n");
    f.close();
    boost::filesystem::remove(path);
    
    BOOST_CHECK_EQUAL(filestr, result);
  }

  {
    boost::filesystem::ofstream fo(path);
    fo << "TZ_1,0,0,EST,0\nTZ_1,86400000000,3600,DST,1\nTZ_2,0,0,EST,0\nTZ_2,86400000000,-3600,DST,1\n";
    fo.close();
    time_zone_database tzdb = time_zone_database::from_file(path.string());
    boost::filesystem::remove(path);

    tzdb.save_to_file(path.string());
    boost::filesystem::ifstream fi(path);
    std::string filestr = std::string(std::istreambuf_iterator<char>(fi), std::istreambuf_iterator<char>());
    fi.close();
    boost::filesystem::remove(path);

    std::string result("TZ_1,0,0,EST,0\nTZ_1,86400000000,3600,DST,1\nTZ_2,0,0,EST,0\nTZ_2,86400000000,-3600,DST,1\n");
    BOOST_CHECK_EQUAL(filestr, result);
  }  

  {
    boost::filesystem::ofstream fo(path);
    fo << "TZ_1,0,0,EST,0\nTZ_1,86400000000,3600,DST,1\nTZ_2,0,0,EST,0\nTZ_2,86400000000,-3600,DST,1,10\n";
    fo.close();
    BOOST_CHECK_THROW(time_zone_database::from_file(path.string()), std::runtime_error);
    boost::filesystem::remove(path);
  }    
}


BOOST_AUTO_TEST_CASE(test_local_date_time_manual_entries) {
  // get temp path
  boost::filesystem::path path;
  while( path.empty() || boost::filesystem::exists(path) ) {
    path = boost::filesystem::temp_directory_path();
    path /= boost::filesystem::unique_path("%%%%-%%%%-%%%%-%%%%");
  }

  {
    time_zone_database tzdb;
    
    time_zone_ptr tz(new time_zone("TZ1"));
    
    tz->add_entry(0, time_zone_entry_info(3600, "ABC", false));
    tz->add_entry(10000, time_zone_entry_info(0, "CBA", true));
    tz->add_entry(100000, time_zone_entry_info(7200, "BAC", false));
    
    {
      BOOST_CHECK_THROW(tz->add_entry(100000, time_zone_entry_info(7200, "BAC", false)), local_time_exception);
      BOOST_CHECK_THROW(tz->remove_entry(9999), local_time_exception);
    }    
    {
      tzdb.add_record("NAME", tz);
      tzdb.save_to_file(path.string());
      boost::filesystem::ifstream fi(path);
      std::string filestr = std::string(std::istreambuf_iterator<char>(fi), std::istreambuf_iterator<char>());
      fi.close();
      boost::filesystem::remove(path);

      std::string result("NAME,0,3600,ABC,0\nNAME,10000,0,CBA,1\nNAME,100000,7200,BAC,0\n");
      BOOST_CHECK_EQUAL(filestr, result);
    }   
    {
      tz->remove_entry(10000);
      tzdb.save_to_file(path.string());
      boost::filesystem::ifstream fi(path);
      std::string filestr = std::string(std::istreambuf_iterator<char>(fi), std::istreambuf_iterator<char>());
      fi.close();
      boost::filesystem::remove(path);

      std::string result("NAME,0,3600,ABC,0\nNAME,100000,7200,BAC,0\n");
      BOOST_CHECK_EQUAL(filestr, result);
    }    
    {
      time_zone_database tzdb_z( time_zone_database::from_struct(zones_struct_simple) );
      tzdb.add_record("TZZ", time_zone::duplicate(tzdb_z.time_zone_from_region("TZ_1") ) );
      tzdb.save_to_file(path.string());
      boost::filesystem::ifstream fi(path);
      std::string filestr = std::string(std::istreambuf_iterator<char>(fi), std::istreambuf_iterator<char>());
      fi.close();
      boost::filesystem::remove(path);

      std::string result("NAME,0,3600,ABC,0\nNAME,100000,7200,BAC,0\nTZZ,0,0,EST,0\nTZZ,86400000000,3600,DST,1\n");
      BOOST_CHECK_EQUAL(filestr, result);
    }
    {
      time_zone_database tzdb( time_zone_database::from_struct(zones_struct_simple) );
      tzdb.delete_record("TZ_1");
      tzdb.save_to_file(path.string());
      boost::filesystem::ifstream fi(path);
      std::string filestr = std::string(std::istreambuf_iterator<char>(fi), std::istreambuf_iterator<char>());
      fi.close();
      boost::filesystem::remove(path);

      std::string result("TZ_2,0,0,EST,0\nTZ_2,86400000000,-3600,DST,1\nTZ_3,0,0,DST,1\nTZ_3,86400000000,3600,EST,0\nTZ_4,0,0,EST1,0\nTZ_4,86400000000,3600,EST2,0\nTZ_5,0,0,EST,1\nTZ_5,86400000000,-3600,DST,0\nTZ_6,0,0,EST,0\nTZ_6,86400000000,-3600,DST,0\n");
      BOOST_CHECK_EQUAL(filestr, result);
    }

    
  }  
  
}

BOOST_AUTO_TEST_CASE(make_gcov_happy) {
  std::unique_ptr<local_time_exception> a(new local_time_exception(""));
  std::unique_ptr<ambiguous_result> b(new ambiguous_result("", ""));
  std::unique_ptr<time_label_invalid> c(new time_label_invalid("", ""));
}



