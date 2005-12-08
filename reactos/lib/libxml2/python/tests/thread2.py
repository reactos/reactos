#!/usr/bin/python -u
import string, sys, time
import thread
from threading import Thread, Lock

import libxml2

THREADS_COUNT = 15

failed = 0

class ErrorHandler:

    def __init__(self):
        self.errors = []
        self.lock = Lock()

    def handler(self,ctx,str):
        self.lock.acquire()
        self.errors.append(str)
        self.lock.release()

def getLineNumbersDefault():
    old = libxml2.lineNumbersDefault(0)
    libxml2.lineNumbersDefault(old)
    return old

def test(expectedLineNumbersDefault):
    time.sleep(1)
    global failed
    # check a per thread-global
    if expectedLineNumbersDefault != getLineNumbersDefault():
        failed = 1
        print "FAILED to obtain correct value for " \
              "lineNumbersDefault in thread %d" % thread.get_ident()
    # check ther global error handler 
    # (which is NOT per-thread in the python bindings)
    try:
        doc = libxml2.parseFile("bad.xml")
    except:
        pass
    else:
        assert "failed"

# global error handler
eh = ErrorHandler()
libxml2.registerErrorHandler(eh.handler,"")

# set on the main thread only
libxml2.lineNumbersDefault(1) 
test(1)
ec = len(eh.errors)
if ec == 0:
    print "FAILED: should have obtained errors"
    sys.exit(1)

ts = []
for i in range(THREADS_COUNT):
    # expect 0 for lineNumbersDefault because
    # the new value has been set on the main thread only
    ts.append(Thread(target=test,args=(0,)))
for t in ts:
    t.start()
for t in ts:
    t.join()

if len(eh.errors) != ec+THREADS_COUNT*ec:
    print "FAILED: did not obtain the correct number of errors"
    sys.exit(1)

# set lineNumbersDefault for future new threads
libxml2.thrDefLineNumbersDefaultValue(1)
ts = []
for i in range(THREADS_COUNT):
    # expect 1 for lineNumbersDefault
    ts.append(Thread(target=test,args=(1,)))
for t in ts:
    t.start()
for t in ts:
    t.join()

if len(eh.errors) != ec+THREADS_COUNT*ec*2:
    print "FAILED: did not obtain the correct number of errors"
    sys.exit(1)

if failed:
    print "FAILED"
    sys.exit(1)

# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()
