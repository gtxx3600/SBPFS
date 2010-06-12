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

import socket
import sys
import threading

from c_node.lib import *
from c_node.user import *
from c_node.dtree import *

from util.common import *
from util.proc import *
from util.parse import *
from util.sbpfs import *

HOST = ''
CLIENT_PORT = 9000
PASSWD = 'config/passwd'
DTREE = 'vdisk/dtree.vd'
INODE = 'vdisk/inode.vd'

class CtrlNode:
    def __init__(self):
        try:
            PROC_NAME = 'SBPFSCtrlNode'
            is_conflict = proc_is_exist(PROC_NAME)
            
            if is_conflict:
                warning('CtrlNode is already running')
                return
            else:
                proc_rename(PROC_NAME)
            
            self.__initailize()

            self.__main()
        except:
            import traceback
            traceback.print_exc(file=sys.stderr)
        self.__quit()
    
    def __main(self):
        while True:
            work = self.__getWork()
            Connection(work)
    
    def __initailize(self):
        self.__init_passwd(PASSWD)
        self.__init_dtree(DTREE, INODE)
        self.__init_socket()
        
    def __init_passwd(self, filename):
        try:
            with open(filename, 'r') as f:
                lines = f.readlines()
                self.users_info = UsersInfo.load(lines)
                print self.users_info
        except IOError:
            self.users_info = UsersInfo()
            warning('Can not open configure file')
    
    def __save_passwd(self, filename):
        try:
            with open(filename, 'w') as f:
                buf = str(self.users_info)
                f.write(buf)
        except IOError:
            perror('Can not write to configure file')
    
    def __init_dtree(self, dtree_vd, inode_vd):
        self.dtree = DTree(dtree_vd, inode_vd)
    
    def __save_dtree(self):
        self.dtree.close()
    
    def __init_socket(self):
        try:
            self.listen_to_client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.listen_to_client.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.listen_to_client.bind((HOST, CLIENT_PORT))
            self.listen_to_client.listen(10)
        except:
            perror('Can not open server socket')
            self.__exit(-1)
    
    def __getWork(self):
        return self.listen_to_client.accept()
    
    def __quit(self):
        try:
            self.listen_to_client.close()
        except:
            pass
        self.__save_passwd(PASSWD)
        self.__save_dtree()
    
    def __exit(self, ret=0):
        self.__quit()
        sys.exit(ret)

class Connection(threading.Thread):
    def __init__(self, work):
        assert type(work) == tuple and len(work) == 2
        
        threading.Thread.__init__(self)
        
        self.conn = work[0]
        self.addr = work[1]
        debug('Connect by %s' % self.addr[0])
        self.start()
    
    def __quit(self):
        self.conn.close()
    
    def run(self):
        head, data = self.__readmsg()
#        print 'addr: ', self.addr
#        print head
#        print data
        if head == None:
            self.__quit()
            return
        if not head.has_key('User'):
            self.__senderr('NoUserError', 'No User Name')
            self.conn.close()
            return
        if head['User'].startswith('Client_'):
            self.__handle_client(head, data)
        elif head['User'].startswith('DNode_'):
            self.__handle_dnode(head, data)
        else:
            self.__handle_unknown(head, data)
        self.__quit()
    
    def __readmsg(self):
        buffer = ''
        while not '\r\n\r\n' in buffer:
            s = self.conn.recv(1024)
            debug(s, 1)
            if not s:
                return None, ''
            buffer += s
        spl = buffer.split('\r\n\r\n', 1)
        head_s = spl[0]
        if len(spl) == 1:
            data = ''
        else:
            data = spl[1]
        d = parse_head(head_s)
        if d == None:
            self.__senderr('SyntaxError', 'Head error')
            return None, ''
        if d['Protocol'] != VERSION:
            self.__senderr('ProtocolError', 'Unknown Protocal: %s' % d['Protocol'])
            return None, ''
        if d.has_key('Content-Length'):
            try:
                conlen = int(d['Content-Length'])
            except ValueError:
                self.__senderr('TypeError', 'Expect Content-Length to be an integer')
                return None, ''
            while len(data) < conlen:
                s = self.conn.recv(1024)
                debug(s, 1)
                if not s:
                    return None, ''
                if len(data) + len(s) > conlen:
                    offset = conlen - len(data)
                    s = s[:offset]
                data += s
            debug('DATA: %s' % data, 1)
        else:
            assert data == ''
        return d, data
    
    def __senderr(self, type, detail):
        statusline = 'SBPFS/1.0 ERROR'
        d = {}
        d['User'] = 'CNode_0'
        d['Content-Length'] = 0
        d['Error-Type'] = type
        d['Error-Detail'] = detail
        head_s = gen_head(statusline, d)
        try:
            debug(head_s, 1)
            self.conn.send(head_s)
        except:
            pass
    
    def __handle_client(self, head, data):
        def check_missing_key(key, type, detail):
            if not head.has_key(key):
                self.__senderr(type, detail)
                return True
            return False
        
        if check_missing_key('Method', 'NoMethodError', 'No Method'):
            return
        method = head['Method']
        
        if check_missing_key('Argc', 'NoArgcError', 'No Argc'):
            return
        try:
            argc = int(head['Argc'])
        except ValueError:
            self.__senderr('TypeError', 'Expect Argc to be an integer')
        
        args = []
        for i in range(0, argc):
            argi = 'Arg%d' % i
            if check_missing_key(argi, 'No%sError'%argi, 'Missing arguement %d'%i):
                return
            args.append(head[argi])
        do_method = {
                     'READ': self.__read,
                     'READDIR': self.__readdir,
                     'WRITE': self.__write,
                     'REMOVE': self.__remove,
                     'MOVE': self.__move,
                     'MKDIR': self.__mkdir,
                     'RMDIR': self.__rmdir,
                     'OPEN': self.__open,
                     'OPENDIR': self.__opendir,
                     'CLOSE': self.__close,
                     'CLOSEDIR': self.__closedir,
                     'CHOWN': self.__chown,
                     'CHMOD': self.__chmod,
                     'STAT': self.__stat,
                     }
        if not do_method.has_key(method):
            self.__senderr('MethodError', 'No Such Method: %s' % method)
        try:
            do_method[method](*args)
        except TypeError as e:
            self.__senderr('ArguementError', '%s' % e)
    
    def __handle_dnode(self, head, data):
        pass
    
    def __handle_unknown(self, head, data):
        self.__senderr('UserNameError', 'Unknown user')
    
    def __read(self, fd, offset, length):
        debug('__reand(%s, %s, %s)' % (fd, offset, length))
    def __readdir(self, dd, en):
        debug('__readdir(%s, %s)' % (dd, en))
    def __write(self, fd, offset, length):
        debug('__write(%s, %s, %s)' % (fd, offset, length))
    def __remove(self, filename):
        debug('__remove(%s)' % filename)
    def __move(self, dst, src):
        debug('__move(%s, %s)' % (dst, src))
    def __mkdir(self, dirname):
        debug('__mkdir(%s)' % dirname)
    def __rmdir(self, dirname):
        debug('__rmdir(%s)' % dirname)
    def __open(self, filename, oflags, mode):
        debug('__open(%s, %s, %s)' % (filename, oflags, mode))
    def __opendir(self, dirname):
        debug('__opendir(%s)' % dirname)
    def __close(self, fd):
        debug('__close(%s)' % fd)
    def __closedir(self, fd):
        debug('__closedir(%s)' % dd)
    def __chown(self, filename, username):
        debug('__chown(%s, %s)' % (filename, username))
#        self.conn.send('SBPFS/1.0 OK\r\n\r\n')
    def __chmod(self, filename, mode):
        debug('__chmod(%s, %s)' % (filename, mode))
    def __stat(self, filename):
        debug('__stat(%s)' % filename)