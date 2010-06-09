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

def proc_is_exist(name):
    import os
    plist = os.popen('ps -e').readlines()[1:]
    for p in plist:
        if name == p.split()[-1]:
            return True
    return False

def proc_rename(name):
    import ctypes
    libc = ctypes.CDLL('libc.so.6')
    libc.prctl(15, name, 0, 0, 0)