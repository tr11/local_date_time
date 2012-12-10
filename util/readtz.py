#! /usr/bin/env python

from __future__ import print_function

COMMAND = 'zdump'

DEFAULT_FOLDER = '/usr/share/zoneinfo'

import argparse
parser = argparse.ArgumentParser(epilog="example: %(prog)s -c America/New_York Europe/London")
parser.add_argument('-i', help='output info', action='store_true')
parser.add_argument('--path', '-p', help='zones path (default: %s)' % DEFAULT_FOLDER, type=str)
parser.add_argument('-c', help='output c++ code', action='store_true')
parser.add_argument('--namespace', '-n', help='c++ namespace (automatically selects c++ mode)', type=str)
parser.add_argument('zones', help='time zone names', nargs='*')
args = parser.parse_args()

import sys
from subprocess import Popen, STDOUT, PIPE
import re
import datetime
EPOCH = datetime.datetime(1970,1,1)

c_version = args.c or args.namespace
zones = args.zones
if zones and zones[0].lower() == '-c':
  c_version = True
  zones = zones[1:]
info = args.i
include_namespace = args.namespace
if args.path:
  folder = args.path
else:
  folder = DEFAULT_FOLDER

if not zones:
  zones = [x.split()[2] for x in open(folder + '/zone.tab', 'r') if x[0] != "#"]

zones = sorted(zones)  
  
null_row = re.compile(r'^$|^.*NULL$')
info_row = re.compile(r'^([A-Za-z_0-9-+\/]+)\s+(Sun|Mon|Tue|Wed|Thu|Fri|Sat)\s+(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s+([ 123]\d)\s+(\d\d:\d\d:\d\d)\s+(\d+)\s+([A-Z]+)\s=\s+(Sun|Mon|Tue|Wed|Thu|Fri|Sat)\s+(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s+([ 123]\d)\s+(\d\d:\d\d:\d\d)\s+(\d+)\s+([A-Za-z]+)\s+isdst=(\d) gmtoff=(\-?\d+)\s*$')

month_map = dict((x,y+1) for y,x in enumerate("Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec".split("|")))

def sign(a):
  return (a > 0) - (a < 0)

def info_from_row(row):
  splits = info_row.match(row).groups()
  utc = int('%04d%02d%02d%6s' % (int(splits[5]), month_map[splits[2]], int(splits[3]), splits[4].replace(':', '')))
  offset = (utc - int('%04d%02d%02d%6s' % (int(splits[11]), month_map[splits[8]], int(splits[9]), splits[10].replace(':', ''))))
  offset = (sign((offset + 240000) // 1000000) - 1) * 240000 + (offset + 240000) % 1000000
  return (splits[0], 
          '%04d%02d%02dT%6s' % (int(splits[5]), month_map[splits[2]], int(splits[3]), splits[4].replace(':', '')),
          3600 * (offset // 10000) + 60 * ((offset // 100) % 100) + offset % 100,
          splits[12],
          splits[13]
         )

def c_info_from_info(info):
  as_dt = datetime.datetime.strptime(info[1], '%Y%m%dT%H%M%S')
  return ('%d' % ((as_dt - EPOCH).total_seconds() * 1000000), info[2], '"%s"' % info[3], info[4])

  
if c_version:
  print("""
#include <tuple>
#include <map>
#include <cstdint>
#include <vector>
#include <string>
""")
  if include_namespace:
    print("namespace %s {" % include_namespace)
    print()
  print("std::map<std::string, std::vector<std::tuple<int64_t, long, std::string, bool> > > static_zones_info  {\n")
  
count = len(zones)
  
for i, zone in enumerate(x for x in zones if not x.startswith('-')):
  if info:
    print('(%03d/%03d) %s' % (i+1, count,zone), file=sys.stderr)
  proc = Popen([COMMAND, "-v", zone], stdout=PIPE)
  result = proc.communicate()[0].decode().split('\n')

  if c_version:
    print('  {"%s",' % zone)
    print('    {')

  for row in result: #include first row
    if null_row.match(row):
      continue
    if c_version:
      print("      std::make_tuple(%s)," % ", ".join(str(x) for x in c_info_from_info(info_from_row(row))))
    else:
      print(",".join(str(x) for x in info_from_row(row)))
    break
    
  for row in result[1::2]: # include every other row
    if null_row.match(row):
      continue
    if c_version:
      print("      std::make_tuple(%s)," % ", ".join(str(x) for x in c_info_from_info(info_from_row(row))))
    else:
      print(",".join(str(x) for x in info_from_row(row)))
  
  if c_version:
    print('    }')
    print('  },')
      
if c_version:
  print("};")
  print()
  
if include_namespace:
  print("}")
  print()

