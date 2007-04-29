all: compdvr

%.o: %.cpp
	g++ -g -o $@ $<

compdvr: compdvr.cpp fork_unix.cpp header.cpp objectfile.cpp util.cpp
	g++ -g -o $@ $^ -lelf
