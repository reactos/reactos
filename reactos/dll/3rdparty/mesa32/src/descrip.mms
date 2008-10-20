# Makefile for Mesa for VMS
# contributed by Jouk Jansen  joukj@hrem.nano.tudelft.nl
# Date last revision : 7 March 2007

all :
	set default [.main]
	$(MMS)$(MMSQUALIFIERS)
	set default [-.glapi]
	$(MMS)$(MMSQUALIFIERS)
	set default [-.shader]
	$(MMS)$(MMSQUALIFIERS)
	set default [-.drivers.common]
	$(MMS)$(MMSQUALIFIERS)
	set default [-.x11]
	$(MMS)$(MMSQUALIFIERS)
	set default [-.osmesa]
	$(MMS)$(MMSQUALIFIERS)
	set default [--.math]
	$(MMS)$(MMSQUALIFIERS)
	set default [-.tnl]
	$(MMS)$(MMSQUALIFIERS)
	set default [-.swrast]
	$(MMS)$(MMSQUALIFIERS)
	set default [-.swrast_setup]
	$(MMS)$(MMSQUALIFIERS)
	set default [-.vbo]
	$(MMS)$(MMSQUALIFIERS)
