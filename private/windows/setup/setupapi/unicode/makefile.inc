!include ..\makefile.inc

msg.h msg00001.bin msg.rc: ..\msg.mc
        mc -v ..\msg.mc

