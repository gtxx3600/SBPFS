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

from util.common import *
from util.proc import *
from util.parse import *

HOST = ''
CLIENT_PORT = 9000

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
            
            try:
                self.__initailize()
            except:
                error('Can not open server socket')
                return

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
        self.listen_to_client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.listen_to_client.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.listen_to_client.bind((HOST, CLIENT_PORT))
        self.listen_to_client.listen(10)
    
    def __getWork(self):
        return self.listen_to_client.accept()
    
    def __quit(self):
        try:
            self.listen_to_client.close()
        except:
            pass

class Connection(threading.Thread):
    def __init__(self, work):
        assert type(work) == tuple and len(work) == 2
        
        threading.Thread.__init__(self)
        
        self.conn = work[0]
        self.addr = work[1]
        print 'Connect by %s' % self.addr[0]
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
        if d.has_key('Content-Length'):
            try:
                conlen = int(d['Content-Length'])
            except ValueError:
                self.__senderr('TypeError', 'Expect Content-Length to be an integer')
                return None, ''
            while len(data) < conlen:
                s = self.conn.recv(1024)
                if not s:
                    return None, ''
                data += s
        else:
            assert data == ''
        return d, data
    
    def __senderr(self, type, detail):
        statusline = 'SBPFS/1.0 ERROR'
        d = {}
        d['User'] = 'CNode_0'
        d['Error-Type'] = type
        d['Error-Detail'] = detail
        head_s = gen_head(statusline, d)
        try:
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
                     'READ': ctrl_read,
                     'WRITE': ctrl_write,
                     'REMOVE': ctrl_remove,
                     'MOVE': ctrl_move,
                     'MKDIR': ctrl_mkdir,
                     'RMDIR': ctrl_rmdir,
                     'OPEN': ctrl_open,
                     'OPENDIR': ctrl_opendir,
                     'CHOWN': ctrl_chown,
                     'CHMOD': ctrl_chmod,
                     }
        if not do_method.has_key(method):
            self.__senderr('MethodError', 'No Such Method: %s' % method)
        try:
            do_method[method](self, *args)
        except TypeError as e:
            self.__senderr('ArguementError', '%s' % e)
    
    def __handle_dnode(self, head, data):
        pass
    
    def __handle_unknown(self, head, data):
        self.__senderr('UserNameError', 'Unknown user')