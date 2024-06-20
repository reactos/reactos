
        CHARGEN
        
This file implements a nice example of handling multiple tcp sockets in a
server environment. Just call chargen_init() from your application after
you have initialized lwip and added your network interfaces. Change the
MAX_SERV option to increase or decrease the number of sessions supported.

chargen will jam as much data as possible into the output socket, so it
will take up a lot of CPU time. Therefore it will be a good idea to run it
as the lowest possible priority (just ahead of any idle task).
 
This is also a good example of how to support multiple sessions in an
embedded system where you might not have fork(). The multiple sessions are
all handled by the same thread and select() is used for demultiplexing.
 
No makefile is provided, just add chargen to the makefile for your
application. It is OS and HW independent.

Once the chargen server is running in your application, go to another system
and open a telnet session to your lwip platform at port 19. You should see an
ASCII pattern start to stream on you screen.

As an example, lets say that your system running lwip is at IP address
192.168.10.244 and you have a linux system connected to it at IP address
192.168.10.59. Issue the following command at a terminal prompt on the linux system:

telnet 192.168.10.244 19

You will see a pattern similar to the following on streaming by on your
screen:

ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{
BCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|
CDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}
DEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~
EFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~!
FGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~!"
GHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~!"#
HIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~!"#$
IJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~!"#$%
JKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~!"#$%&
KLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~!"#$%&'
LMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~!"#$%&'(

It even works from windows: At a dos prompt you can also issue the same
telnet command and you will get a similar (but much slower, at least on W98)
data stream.

David Haas


