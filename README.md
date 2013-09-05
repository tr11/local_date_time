local_date_time
===============

[![Build Status](https://api.travis-ci.org/tr11/local_date_time.png?branch=master)](https://travis-ci.org/tr11/local_date_time)

The local_date_time C++ library is intended to be a replacement to the Local Time classes in the excellent Boost Date Time library (http://www.boost.org/libs/date_time/).

An issue with the time_zone construct in the Boost date time library that this attempts to overcome concerns time zones for which rules change over time.  For example, in the United States, DST began on the first Sunday in April through 2006, but since 2007 it begins on the second Sunday of March. This change is not directly modeled with the ``custom_time_zones`` class in the Boost date time library. This library solves the issue above by using a lookup map to determine the correct segment to use.  

There is also a Python utility script to read zoneinfo files in a Linux environment (relying on the zdump program) which tie to the Olson tz database (http://www.twinsun.com/tz/tz-link.htm). The Python utility can output comma separated values or a C++ file with a map that can be passed directly to the time_zone_database construct.

This library is released under the Boost Software License, Version 1.0. (see http://www.boost.org/LICENSE_1_0.txt).
