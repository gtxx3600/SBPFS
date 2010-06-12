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
# along with SBPFS; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA

from c_node.fs.libcfs import *

class DTree:
    def __init__(self, dt_file, in_file):
        self.dt_file = dt_file
        self.in_file = in_file
        self.diskp = open_disk(dt_file, in_file)
        c_mkdir(self.diskp, '/abc', 0)
    
    def __lookup(self, path):
        '''return a file entry of the selected path'''
    
    def close(self):
        ret = save_disk(self.diskp, self.dt_file, self.in_file)
        close_disk(self.diskp)
        if ret:
            raise SaveError('')
        
class SaveError(Exception):
    def __init__(self, msg=''):
        Exception.__init__(self, msg)