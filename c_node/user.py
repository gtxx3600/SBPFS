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
    def __init__(self, uid, username, password, groupname):
        self.uid = uid
        self.username = username
        self.password = password
        self.groupname = groupname

class GroupInfo:
    def __init__(self, gid, groupname):
        self.gid = gid
        self.groupname = groupname

USER_EXIST_EXCEPTION = 'UserExistException'
GROUP_EXIST_EXCEPTION = 'GroupExistException'

class UsersInfo:
    def __init__(self, *init_users):
        self.__users = {}
        self.__groups = {}
        self.__ucount = 0
        self.__gcount = 0
        for user in init_users:
            self.adduser(user)
    
    @classmethod
    def load(cls, lines):
        users_info = UsersInfo()
        for line in lines:
            if line.startswith('#'):
                continue
            try:
                un, gn, uid, gid, passwd = line.split(':')
                users_info.__groups[gn] = GroupInfo(gid, gn)
                users_info.__users[un] = UserInfo(uid, un, passwd, gn)
                if users_info.__ucount < uid:
                    users_info.__ucount = uid
                if users_info.__gcount < gid:
                    users_info.__gcount = gid
            except:
                Warning('Invalid line: ', line)
        return users_info
    
    def find_uid(self, username):
        if self.has_user(username):
            return self.__users[username].uid
        else:
            return None
    
    def adduser(self, user):
        assert type(user) == tuple and len(user) == 3
        
        if self.has_user(user[0]):
            raise UserException(USER_EXIST_EXCEPTION)
        if not self.has_group(user[3]):
            self.addgroup(user[3])
        self.__users[user[0]] = UserInfo(self.__ucount, *user)
        self.__ucount += 1
    
    def addgroup(self, groupname):
        if self.has_group(groupname):
            raise UserException(GROUP_EXIST_EXCEPTION)
        self.__groups[groupname] = GroupInfo(self.__gcount, groupname)
        self.__gcount += 1
    
    def has_user(self, username):
        return self.__users.has_key(username)
    
    def has_group(self, groupname):
        return self.__groups.has_key(groupname)

class UserException(Exception):
    def __init__(self, type):
        Exception.__init__(self, type)
        self.type = type