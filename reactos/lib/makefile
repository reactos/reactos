LIBRARIES = ntdll kernel32

BASE_CFLAGS = -I../include

all: $(LIBRARIES)

ntdll: dummy
	$(MAKE) -C ntdll
       
kernel32: dummy
	$(MAKE) -C kernel32

dummy:

