#!/usr/bin/env python
#!/bin/env python
#
#    Counts the number of cites of PETSc in different categories in petscapp.bib
import os
import re
from exceptions import *
import sys
from string import *
import commands

def main():
  tcount = 0
  f     = open(os.path.join('src','docs','tex','petscapp.bib'))
  line  = f.readline()
  while (line):  
    if '@' in line:
      tcount = tcount + 1
    line  = f.readline()
  f.close()
  print("<li> Publications that have used PETSc "+str(tcount)+" (incomplete)")
  print("<ul>")

  f     = open(os.path.join('src','docs','tex','petscapp.bib'))
  line  = f.readline()
  topic = ''
  mat   = re.compile('<center>[a-zA-Z0-9_ -]*</center>')
  nam   = re.compile('<a name=\"[a-z]*\">')
  while line:
    if '<center>' in line and '</center>' in line:
      if topic and count > 0:
        print('<li><a href="petscapps.html#'+name+'">'+topic+' '+str(count)+'</a></li>')
      count = 0
      topic = mat.findall(line)[0]
      topic = topic[8:-9]
      name  = nam.findall(line)
      if name:
        name = name[0][9:-2]
     
    elif '@' in line:
      count = count + 1
    line = f.readline()
  f.close()
  print('<li><a href="petscapps.html#'+name+'">'+topic+' '+str(count)+'</a></li>')
  print("</ul></li>")
#
# The classes in this file can also be used in other python-programs by using 'import'
#
if __name__ ==  '__main__': 
    main()

