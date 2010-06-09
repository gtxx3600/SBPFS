#!/usr/bin/env python

# SBPFS -  DFS
#
# Copyright (C) 2009-2010, SBPFS Developers Team
#
# SBPFS is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# SBPFS is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Ailurus; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA

def parse_head(s):
    d = {}
    lines = s.split('\r\n')
    spl = lines[0].split()
    d['Protocol'] = spl[0]
    if len(spl) >= 2:
        d['Response'] = spl[1]
    for line in lines[1:]:
        spl = line.split(': ', 1)
        if not d.has_key(spl[0]):
            d[spl[0]] = spl[1]
    return d