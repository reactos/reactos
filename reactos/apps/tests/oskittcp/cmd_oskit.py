from socket import *
import sys

s = socket(AF_INET,SOCK_DGRAM,0)
s.connect(('localhost',5001))

while 1:
	sys.stdout.write('>> ')
	line = sys.stdin.readline()
	s.send('CMD ' + line)