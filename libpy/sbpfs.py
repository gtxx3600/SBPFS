#!/usr/bin/python

import sys, os
import sbpfs
import time

CNode = '59.78.15.46'



def ls():
	path = '/'
	detail = False
	import sys 
	if len(sys.argv[1:]) > 1:
		if sys.argv[2].startswith('/'):
			path = sys.argv[2]
			if path.endswith('/'):
				path = path[:-1]
		if "-l" in sys.argv[1:]:
			detail = True
	
	fd = sbpfs.opendir(path)
	if(fd < 0):
		print 'Opendir Failed \nMake sure your request path exist and file-type must be directory'
		return
	if detail:
		print '%s %s %s %s %s' % ('name'.ljust(15), 'length'.ljust(15), 'mode'.ljust(10), 'owner'.ljust(8), 'last modify time'.ljust(33))
	try:
		while True:
			r = sbpfs.readdir(fd)
			if r:
				offset, type, name = r
				disp = ''
				def getmode(type, mod):
					ret = ''
					if(type == 'directory'):
					    ret += 'd'
					else:
						ret += '-'
					if mod & 0x04:
						ret += 'w'
					else :
						ret += '-'
					if mod & 0x08:
						ret += 'r'
					else:
						ret += '-'
					ret += ' '
					if mod & 0x01:
						ret += 'w'
					else:
						ret += '-'
					if mod & 0x02:
						ret += 'r'
					else:
						ret += '-'
					return ret
				if detail:
					import time
					dict = sbpfs.stat(path + '/' + name)
#					disp += '%.15' % name
#					disp += '%.10s ' % dict['size']
#					disp += '%.10s ' % getmode(type,int(dict['mode']))
#					disp += '%.8s' % dict['owner']
#					disp += '%.33s' % time.strftime("%D %H:%M:%S", time.gmtime(int(dict['modify_time'])))
					print name.ljust(15),str(dict['size']).ljust(15),getmode(type,int(dict['mode'])).ljust(10),str(dict['owner']).ljust(8),time.strftime("%Y-%m-%d %X", time.gmtime(int(dict['modify_time']))).ljust(33)

				else:
					print name
				
			else:
				break;
	except:
		import traceback
		import sys
		traceback.print_exc(file=sys.stdout)
	sbpfs.closedir(fd)
		
def get():
	try:
		path = '/'
		import sys
		if len(sys.argv[1:]) > 1:
			if sys.argv[2].startswith('/'):
				path = sys.argv[2]
				if path.endswith('/'):
					path = path[:-1]
		
		else:
			print 'arg err get failed'
			return
		filename = path.split("/")[-1]
		if not filename:
			print 'Invalid filename'
			return
		fd = sbpfs.open(path, 1, 0)
		if fd < 0: 
			print 'Make sure your request path exist and file-type must be file'
			return
		dict = sbpfs.stat(path)
		length = int(dict['size'])
		if os.path.exists(filename):
			print 'File : %s already exists' % filename
			sbpfs.close(fd)
			return
		f = file(filename, 'wb')
		bufsize = 1024*1024
		for i in range(0, length / bufsize):
			r = sbpfs.read(fd, bufsize)
			if r and len(r) == bufsize:
				f.write(r)
				print "received : %.2f%% of %d bytes" % (((float(i*bufsize)/length)*100) ,length)
			else:
				print 'Error occur'
				sbpfs.perror('')
				sbpfs.close(fd)
				return
		if length % bufsize:
			r = sbpfs.read(fd, length % bufsize)
			if r and len(r) == length % bufsize:
				f.write(r)
			print "received : %.2f%%" % float(100.0)
		f.close()
		sbpfs.close(fd)
				
					
	except:
		print 'Unknow error occur'
		import traceback
		import sys
		traceback.print_exc(file=sys.stdout)
		#pass

def put():
	try:
		path = '/'
		dst = '/'
		import sys
		if len(sys.argv[1:]) > 2:
			path = sys.argv[2]
			if path.endswith('/'):
				path = path[:-1]
	
			if sys.argv[3].startswith('/'):
				dst = sys.argv[3]
				if dst.endswith('/'):
					dst = dst[:-1]
		elif len(sys.argv[1:]) == 2:
			if sys.argv[2].startswith('/'):
				path = sys.argv[2]
				if path.endswith('/'):
					path = path[:-1]
			dst = '/' + path.split('/')[-1]

		else:
			print 'arg err get failed'
			return
		filename = path.split("/")[-1]
		if not filename:
			print 'Invalid filename'
			return
		
		
		if not os.path.exists(path):
			print 'File : %s not exists' % path
			return
		if not os.path.isfile(path):
			print '%s is not a file' % path
			return
		length = os.path.getsize(path)
		f = file(path, 'rb')
		fd = sbpfs.open(dst, 10, 14)
		if fd < 0: 
			print 'Make sure your request path exist and file-type must be file'
			return
		bufsize = 1024*1024
		for i in range(0, length / bufsize):
			r = f.read(bufsize)
			if r and len(r) == bufsize:
				rb = buffer(r)
				sbpfs.write(fd,rb,bufsize)
				print "send : %.2f%% of %d bytes" % (((float(i*bufsize)/length)*100) ,length)
			else:
				print 'Error occur'
				sbpfs.perror('')
				sbpfs.close(fd)
				return
		if length % bufsize:
			r = f.read(length % bufsize)
			
			rb=buffer(r)
			if r and len(r) == length % bufsize:
				sbpfs.write(fd,rb,length%bufsize)
			
			print "send : %.2f%%" % (100.0)
		f.close()
		sbpfs.close(fd)
				
					
	except:
		print 'Unknow error occur'
		import traceback
		import sys
		traceback.print_exc(file=sys.stdout)
		#pass
	
def mkdir():
	path = '/'
	import sys
	if len(sys.argv[1:]) > 1:
		path = sys.argv[2]
		if not path.startswith('/'):
			path = '/'+path
		if path.endswith('/'):
			path = path[:-1]
	if sbpfs.mkdir(path) < 0:
		print 'mkdir %s failed' % path
		sbpfs.perror('')
		
def rmdir():
	path = '/'
	import sys
	if len(sys.argv[1:]) > 1:
		path = sys.argv[2]
		if not path.startswith('/'):
			path = '/'+path
		if path.endswith('/'):
			path = path[:-1]
	if sbpfs.rmdir(path) < 0:
		print 'rmdir %s failed' % path
		sbpfs.perror('')
	
def remove():
	path = '/'
	import sys
	if len(sys.argv[1:]) > 1:
		path = sys.argv[2]
		if not path.startswith('/'):
			path = '/'+path
		if path.endswith('/'):
			path = path[:-1]
	if sbpfs.remove(path) < 0:
		print 'remove %s failed' % path
		sbpfs.perror('')
	
def move():
	path = '/'
	dst = '/'
	import sys
	if len(sys.argv[1:]) > 2:
		path = sys.argv[2]
		if path.endswith('/'):
			path = path[:-1]
		if not path.startswith('/'):
			path = '/' + path
		dst = sys.argv[3]
		if dst.endswith('/'):
			dst = dst[:-1]
		if not dst.startswith('/'):
			dst = '/' + dst
	else:
		print 'arg err get failed'
		return
	if sbpfs.move(path,dst) < 0:
		print 'rename %s to %s failed' % (path,dst)
		sbpfs.perror('')
		
		
methods = {'ls':ls,
		   'get':get,
		   'put':put,
		   'mkdir':mkdir,
		   'rmdir':rmdir,
		   'rm':remove,
		   'remove':remove,
		   'mv':move,
		   'move':move
		   }
if __name__ == "__main__":
	if sbpfs.login('root', 'root') < 0: print 'login err'
	if sbpfs.sethost(CNode) < 0:print 'sethost err'
	try :
		methods[sys.argv[1]]()
	except:
		pass
