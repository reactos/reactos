LIBRARIES = ntdll kernel32

BASE_CFLAGS = -I../include

all: $(LIBRARIES)

ntdll: dummy
	make -C ntdll
       
kernel32: dummy
	make -C kernel32

dummy:

