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


def ctrl_read(conn, filename, offset, length):
    print 'ctrl_reand(%s, %s, %s)' % filename, offset, length
def ctrl_write(conn, filename, offset, length):
    print 'ctrl_write(%s, %s, %s)' % filename, offset, length
def ctrl_remove(conn, filename):
    print 'ctrl_remove(%s)' % filename
def ctrl_move(conn, dst, src):
    print 'ctrl_move(%s, %s)' % dst, src
def ctrl_mkdir(conn, dirname):
    print 'ctrl_mkdir(%s)' % dirname
def ctrl_rmdir(conn, dirname):
    print 'ctrl_rmdir(%s)' % dirname
def ctrl_open(conn, filename, oflags, mode):
    print 'ctrl_open(%s, %s, %s)' % filename, oflags, mode
def ctrl_opendir(conn, dirname):
    print 'ctrl_opendir(%s)' % dirname
def ctrl_chown(conn, filename, username, groupname):
    print 'ctrl_chown(%s, %s, %s)' % filename, username, groupname
def ctrl_chmod(conn, filename, mode):
    print 'ctrl_chmod(%s, %s)' % filename, mode
