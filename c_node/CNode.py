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

import socket
import sys
import threading

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
        self.start()
    
    def run(self):
        head, data = self.__readmsg()
#        print 'addr: ', self.addr
#        print head
#        print data
        if head['User'].startswith('Client_'):
            self.__handle_client(head, data)
        elif head['User'].startswith('DNode_'):
            self.__handle_dnode(head, data)
        else:
            self.__handle_unknown(head, data)
        self.conn.close()
    
    def __readmsg(self):
        buffer = ''
        while not '\r\n\r\n' in buffer:
            buffer += self.conn.recv(1024)
        spl = buffer.split('\r\n\r\n', 1)
        head = spl[0]
        if len(spl) == 1:
            data = ''
        else:
            data = spl[1]
        d = parse_head(head)
        if d.has_key('Content-Length'):
            conlen = int(d['Content-Length'])
            while len(data) < conlen:
                data += self.conn.recv(1024)
        else:
            assert data == ''
        return d, data
    
    def __handle_client(self, head, data):
        pass
    
    def __handle_dnode(self, head, data):
        pass
    
    def __handle_unknown(self, head, data):
        pass