TARGET := ArchBlackmann.exe

.PHONY: all

all: $(TARGET)

CPP=mingw32-g++
CFLAGS := -DWIN32 -D_DEBUG -D_CONSOLE -D_MBCS
LFLAGS :=
LIBS := -lstdc++ -lws2_32

SRCS := ArchBlackmann.cpp \
	base64.cpp \
	chomp.cpp \
	cram_md5.cpp \
	File.cpp \
	IRCClient.cpp \
	MD5.cpp \
	panic.cpp \
	ReliMT.cpp \
	SockUtils.cpp \
	SplitJoin.cpp \
	ssprintf.cpp \
	ThreadPool.cpp \
	trim.cpp

OBJS := $(SRCS:.cpp=.o)

$(TARGET): $(OBJS)
	$(CPP) $(LFLAGS) -o $@ $(OBJS) $(LIBS)

.cpp.o: $<
	$(CPP) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	-@del $(TARGET)
	-@del $(OBJS)
