#!/usr/bin/python3

import re
import sys

def replace(matchobj):
  prefix = matchobj.group(1)
  s = matchobj.group(2)
  orig_s = s
  # Replace '\"' with '"'.
  s = re.sub(r'\\"', r'"', s)
  if s == orig_s:
    # No changes.
    return prefix + orig_s
  if re.search(r'\\', s):
    # Skip strings still containing '\'.
    return prefix + orig_s
  # Use '{}' instead of '""'.
  s = re.sub(r'^"', "{", s, flags=re.MULTILINE)
  s = re.sub(r'"$', "}", s, flags=re.MULTILINE)
  return prefix + s

def process_line(line):
    if re.search(r'[{}]', line):
      # Avoid transforming '"{"' into '{{}'.
      return line
    # Handle double-quoted strings in line.  Properly handle escaped
    # double-quotes.  Avoid strings containing '$', but allow any escaped
    # char.
    re_string=r'([^\\])("(?:[^"\\$]|(?:\\.))*")'
    return re.sub(re_string, replace, line)

# Process all lines of file passed as argument.
fn = sys.argv[1]
with open(fn) as fh:
  for line in fh:
    line = line.rstrip('\n')
    line = process_line(line)
    print(line)
