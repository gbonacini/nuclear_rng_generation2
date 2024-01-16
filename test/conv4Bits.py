#!/usr/bin/env python3

# -----------------------------------------------------------------
# GeigerRng - A Random Number Generator using Common Geiger Counters
# Copyright (C) 2022,2023,2024  Gabriele Bonacini
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
# -----------------------------------------------------------------

import sys
from ctypes import c_ubyte

if len(sys.argv) != 3:
   print("Syntax: {} <input_file> <output_file>\n".format(sys.argv[0]))
   sys.exit(1)

INVALID=-1
highBits=INVALID
with open(sys.argv[2], "wb") as outputFile:
   with open(sys.argv[1]) as inputFile:
        for line in inputFile:        
                binValue = int(line.rstrip('\n'))
                if highBits == INVALID:
                    highBits = binValue << 4
                else:
                    rnd = highBits | binValue
                    outputFile.write(c_ubyte(rnd))
                    highBits=INVALID
