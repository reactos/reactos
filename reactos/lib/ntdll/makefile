all: ntdll.a

OBJECTS = napi.o

ntdll.a: $(OBJECTS)
	$(AR) vcsr ntdll.a $(OBJECTS)
	
dummy:

include ../../rules.mak
