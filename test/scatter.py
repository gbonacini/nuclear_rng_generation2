#!/usr/bin/env python3

# -----------------------------------------------------------------
# GeigerRng - A Random Number Generator using an Arduino Geiger Shield
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
from matplotlib import pyplot

if len(sys.argv) != 2:
   print("Syntax: {} <input_file> \n".format(''.join(sys.argv[0])))
   sys.exit(1)

xs    = list()
ys    = list()
item  = 0

with open(''.join(sys.argv[1]), "rb") as inputFile:
       for binValues in iter(lambda: inputFile.read(4), ''):
                if len(binValues) != 4:
                    break

                xs.append(int.from_bytes(binValues[:2], byteorder='little'))
                ys.append(int.from_bytes(binValues[2:], byteorder='little'))

pyplot.rcParams["figure.figsize"] = (20,20)
pyplot.scatter(xs, ys, color='black')
pyplot.show()
