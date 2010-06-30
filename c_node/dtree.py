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
from util.common import *

S_DIR = 1
S_FIL = 0

SBP_O_RDONLY = 0x01
SBP_O_WRONLY = 0x02
SBP_O_RDWR = 0x04
SBP_O_CREAT = 0x08
SBP_O_APPEND = 0x10

def load_dnodes():
    with open('config/dnodes', 'r') as f:
        lines = f.readlines()
    ips = []
    for line in lines:
        name, ip = line.rsplit(':', 1)
        ips.append(ip)
    return ','.join(ips)

class FileDes:
    def __init__(self, fb, mode, type):
        self.fb = fb
        self.mode = mode
        self.type = type

class FdPool:
    def __init__(self):
        self.__usingmap = {}
        self.__usedlist = []
        self.__count = 0
    
    def get_file(self, fd):
        try:
            return self.__usingmap[fd]
        except:
            return None
    
    def gen(self, file):
        try:
            fd = self.__usedlist.pop()
            self.__usingmap[fd] = file
        except:
            fd = self.__count
            self.__count += 1
            self.__usingmap[fd] = file
        return fd
    
    def des(self, fd):
        try:
            del self.__usingmap[fd]
            if len(self.__usingmap) == 0:
                self.__usedlist = []
                self.__count = 0
            else:
                self.__usedlist.append(fd)
        except:
            raise DTreeError('CloseError', 'Not such fd: %d' % fd)
    
    def isEmpty(self):
        return len(self.__usingmap) == 0

class DTree:
    def __init__(self, dt_file, in_file):
        self.dt_file = dt_file
        self.in_file = in_file
        self.fds = {}
        self.ips = load_dnodes();
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

    def __gen_fd(self, file, uid):
        if self.fds.has_key(uid):
            fdp = self.fds.get(uid)
        else:
            fdp = FdPool()
            self.fds[uid] = fdp
        return fdp.gen(file)
    
    def __des_fd(self, fd, uid):
        self.fds[uid].des(fd)
        if self.fds[uid].isEmpty():
            del self.fds[uid]
    
    def dt_close(self):
        ret = save_disk(self.diskp, self.dt_file, self.in_file)
        if ret:
            raise SaveError()
        print 'close disk'
        close_disk(self.diskp)
        print 'shutdown'
    
    def open(self, path, oflags, mode, uid):
        oflags = int(oflags)
        mode = int(mode)
        fb = c_open(self.diskp, path, oflags, mode, uid)
        if fb == -1:
            raise DTreeError('OpenError', 'No such file')
        if fb == -2:
            raise DTreeError('OpenError', 'Cannot open a directory')
        elif fb == -4:
            raise DTreeError('PermissionError', 'Permission denied')
        elif fb < 0:
            raise DTreeError('OpenError', 'Unknown error')
        file = FileDes(fb, oflags, S_FIL)
        return self.__gen_fd(file, uid)
    
    def read(self, fd, offset, length, uid):
        fd = int(fd)
        offset = int(offset)
        length = int(length)
        try:
            file = self.fds[uid].get_file(fd)
        except:
            raise DTreeError('ReadError', 'Not such fd: %d' % fd)
        if file == None or file.type != S_FIL:
            raise DTreeError('ReadError', 'Not such fd: %d' % fd)
        if not (file.mode & SBP_O_RDONLY or file.mode & SBP_O_RDWR):
            raise DTreeError('ModeError', 
                             'Can not read. mode=%d' % file.mode)
        data = c_read(self.diskp, file.fb, offset, length, uid)
        if data == None:
            raise DTreeError('ReadError', 'ReadError')
        return data
    
    def write(self, fd, offset, length, uid):
        fd = int(fd)
        offset = int(offset)
        length = int(length)
        try:
            file = self.fds[uid].get_file(fd)
        except:
            raise DTreeError('WriteError', 'Not such fd: %d' % fd)
        if file == None or file.type != S_FIL:
            raise DTreeError('WriteError', 'Not such fd: %d' % fd)
        if not (file.mode & SBP_O_WRONLY or file.mode & SBP_O_RDWR):
            raise DTreeError('ModeError', 
                             'Can not write. mode=%d' % file.mode)
        data = c_write(self.diskp, file.fb, offset, length, self.ips, uid)
        if data == None:
            raise DTreeError('WriteError', 'WriteError')
        return data
    
    def stat(self, path, uid):
        data = c_stat(self.diskp, path, uid)
        if data:
            raise DTreeError("StatError", 'StatError');
        return data
    
    def close(self, fd, uid):
        fd = int(fd)
        if self.fds.has_key(uid):
            file = self.fds[uid].get_file(fd)
            if file == None or file.type != S_FIL:
                raise DTreeError('CloseError', 'Not such fd: %d' % fd)
            self.__des_fd(fd, uid)
        else:
            raise DTreeError('CloseError', 'Not such fd: %d' % fd)
    
    def opendir(self, path, uid):
        ret = c_opendir(self.diskp, path, uid)
        if ret == None:
            raise DTreeError('FatalError', 'Server Error')
        fb, entnum = ret
        if fb == -1:
            raise DTreeError('OpenDirError', 'No such directory')
        elif fb == -4:
            raise DTreeError('PermissionError', 'Permission denied')
        elif fb < 0:
            raise DTreeError('OpenDirError', 'Unknown error')
        file = FileDes(fb, 0, S_DIR)
        return self.__gen_fd(file, uid), entnum
    
    def readdir(self, dd, entnum, uid):
        dd = int(dd)
        entnum = int(entnum)
        try:
            file = self.fds[uid].get_file(dd)
        except:
            raise DTreeError('ReadDirError', 'Not such dirfd: %d' % dd)
        if file == None or file.type != S_DIR:
            raise DTreeError('ReadDirError', 'Not such dirfd: %d' % dd)
        ret = c_readdir(self.diskp, file.fb, entnum, uid)
        if ret == None:
            raise DTreeError('ReadDirError', 'ReadDirError')
        return ret
    
    def closedir(self, dd, uid):
        dd = int(dd)
        if self.fds.has_key(uid):
            file = self.fds[uid].get_file(dd)
            if file == None or file.type != S_DIR:
                raise DTreeError('CloseDirError', 'Not such dirfd: %d' % dd)
            self.__des_fd(dd, uid)
        else:
            raise DTreeError('CloseDirError', 'Not such dirfd: %d' % dd)
    
    def move(self, dst, src, uid):
        ret = c_move(self.diskp, dst, src, uid)
        if ret == 1:
            raise DTreeError('MoveError', 'No such file or directory')
        elif ret == 2:
            raise DTreeError('MoveError', 'destination exists')
        elif ret == 4:
            raise DTreeError('PermissionError', 'Permission denied')
        elif ret < 0:
            raise DTreeError('FatalError', 'Server Error')
    
    def remove(self, path, uid):
        ret = c_remove(self.diskp, path, uid)
        if ret == 1:
            raise DTreeError('RemoveError', 'No such file or directory')
        elif ret == 2:
            raise DTreeError('RemoveError', 'Is a directory')
        elif ret == 4:
            raise DTreeError('PermissionError', 'Permission denied')
        elif ret < 0:
            raise DTreeError('FatalError', 'Server Error')
    
    def chown(self, path, newuid, uid):
        newuid = int(newuid)
        ret = c_chown(self.diskp, path, newuid, uid)
        if ret == 1:
            raise DTreeError('ChownError', 'No such file or directory')
        elif ret == 4:
            raise DTreeError('PermissionError', 'Permission denied')
        elif ret < 0:
            raise DTreeError('FatalError', 'Server Error')
    
    def chmod(self, path, mode, uid):
        mode = int(mode)
        ret = c_chmod(self.diskp, path, mode, uid)
        if ret == 1:
            raise DTreeError('ChmodError', 'No such file or directory')
        elif ret == 4:
            raise DTreeError('PermissionError', 'Permission denied')
        elif ret < 0:
            raise DTreeError('FatalError', 'Server Error')
    
    def mkdir(self, path, uid):
        ret = c_mkdir(self.diskp, path, uid)
        if ret == 1:
            raise DTreeError('MkdirError', 'No such file or directory')
        elif ret == 2:
            raise DTreeError('MkdirError', 'No such file or directory')
        elif ret == 3:
            raise DTreeError('MkdirError', 'File exists')
        elif ret == 4:
            raise DTreeError('PermissionError', 'Permission denied')
        elif ret < 0:
            raise DTreeError('FatalError', 'Server Error')
    
    def rmdir(self, path, uid):
        ret = c_rmdir(self.diskp, path, uid)
        if ret == 1:
            raise DTreeError('RmdirError', 'No such file or directory')
        elif ret == 2:
            raise DTreeError('RmdirError', 'Not a directory')
        elif ret == 3:
            raise DTreeError('RmdirError', 'Directory not empty')
        elif ret == 4:
            raise DTreeError('PermissionError', 'Permission denied')
        elif ret < 0:
            raise DTreeError('FatalError', 'Server Error')

class DTreeError(Exception):
    def __init__(self, type='', msg=''):
        Exception.__init__(self, msg)
        self.type = type
        self.msg = msg
        
class SaveError(DTreeError):
    def __init__(self, msg=''):
        Exception.__init__(self, msg)