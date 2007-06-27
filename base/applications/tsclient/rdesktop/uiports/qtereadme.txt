This is the Qt/Emb ui port
qt should be installed in /usr/local/qt
you may need to have LD_LIBRARY_PATH and QTDIR defined to run qtrdesktop
tested with versions 2.3, 3.1

makefile_qte can be edited to change file localtions
run make -f makefile_qte in this directory to compile it

qtereadme.txt - notes, this file
makefile_qte - makefile
qtewin.cpp - ui lib
qtewin.h - header
