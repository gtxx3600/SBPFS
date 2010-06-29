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

from util.common import *

class UserInfo:
    def __init__(self, uid, username, password):
        self.uid = uid
        self.username = username
        self.password = password
    
    def __str__(self):
        buf = '%s:%s:%s' % (self.username, self.uid, self.password)
        return buf

USER_EXIST_EXCEPTION = 'UserExistException'

class UsersInfo:
    def __init__(self, *init_users):
        self.__users = {}
        self.__ucount = 0
        for user in init_users:
            self.adduser(user)
    
    def __str__(self):
        lines = []
        for key in self.__users.keys():
            lines.append(str(self.__users[key]))
        return '\n'.join(lines)
    
    @classmethod
    def load(cls, lines):
        users_info = UsersInfo()
        for line in lines:
            if line.startswith('#'):
                continue
            try:
                line = line.strip()
                un, uid, passwd = line.split(':')
                uid = int(uid)
                users_info.__users[un] = UserInfo(uid, un, passwd)
                if users_info.__ucount < uid:
                    users_info.__ucount = uid
            except:
                Warning('Invalid line: ', line)
        return users_info
    
    def check(self, user_info):
        if not self.has_user(user_info.username):
            return False
        if user_info.username == 'anonymous':
            return True
        if self.__users[user_info.username].password != user_info.password:
            return False
        return True
    
    def find_uid(self, username):
        if self.has_user(username):
            return self.__users[username].uid
        else:
            return None
    
    def adduser(self, user):
        assert type(user) == tuple and len(user) == 3
        
        if self.has_user(user[0]):
            raise UserException(USER_EXIST_EXCEPTION)
        self.__users[user[0]] = UserInfo(self.__ucount, *user)
        self.__ucount += 1
    
    def has_user(self, username):
        return self.__users.has_key(username)

class UserException(Exception):
    def __init__(self, type):
        Exception.__init__(self, type)
        self.type = type