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
#        print self.diskp
#        print c_mkdir(self.diskp, '/haha1/', 0)
#        print c_mkdir(self.diskp, '/haha2', 0)
#        print c_mkdir(self.diskp, '/haha3/', 0)
#        print c_mkdir(self.diskp, '/haha4', 0)
#        print c_mkdir(self.diskp, '/haha5', 0)
#        print c_mkdir(self.diskp, '/haha1/gaga1', 0)
#        print c_mkdir(self.diskp, '/haha1/gaga2', 0)
#        print c_mkdir(self.diskp, '/haha1/gaga3', 0)
#        for i in range(0, 32):
#            print 'sk%d: %d\n' % (i, c_mkdir(self.diskp, '/sk%d'%i, 0))
#        print c_rmdir(self.diskp, '/abcb', 0)
#        print c_chmod(self.diskp, '/abc', 15, 0)
    
    def __lookup(self, path):
        '''return a file entry of the selected path'''
    
    def close(self):
        ret = save_disk(self.diskp, self.dt_file, self.in_file)
        if ret:
            raise SaveError()
        print 'close disk'
        close_disk(self.diskp)
        print 'shutdown'
    
    def mkdir(self, path, uid):
        ret = c_mkdir(self.diskp, path, uid)
        if (ret == 1):
            raise DTreeError('MkdirError', 'No such file or directory')
        elif (ret == 2):
            raise DTreeError('MkdirError', 'No such file or directory')
        elif (ret == 3):
            raise DTreeError('MkdirError', 'File exists')
        elif (ret == 4):
            raise DTreeError('PermissionError', 'Permission denied')
        elif (ret < 0):
            raise DTreeError('FatalError', 'Fatal Error');
    
    def rmdir(self, path, uid):
        ret = c_rmdir(self.diskp, path, uid)
        if (ret == 1):
            raise DTreeError('RmdirError', 'No such file or directory')
        elif (ret == 2):
            raise DTreeError('RmdirError', 'Not a directory')
        elif (ret == 3):
            raise DTreeError('RmdirError', 'Directory not empty')
        elif (ret == 4):
            raise DTreeError('PermissionError', 'Permission denied')
        elif (ret < 0):
            raise DTreeError('FatalError', 'Server Error');

class DTreeError(Exception):
    def __init__(self, type='', msg=''):
        Exception.__init__(self, msg)
        self.type = type
        self.msg = msg
        
class SaveError(DTreeError):
    def __init__(self, msg=''):
        Exception.__init__(self, msg)