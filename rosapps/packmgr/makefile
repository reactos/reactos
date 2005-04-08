
all: lib gui cmd-line

lib: dummy
	$(MAKE) -C lib
       
gui: dummy
	$(MAKE) -C gui

cmd-line: dummy
	$(MAKE) -C cmd-line

dummy:

clean:
	$(MAKE) -C gui clean
	$(MAKE) -C lib clean
