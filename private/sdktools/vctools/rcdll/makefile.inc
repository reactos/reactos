msg00001.bin rcmsgs.h rcmsgs.rc: rcmsgs.mc
	mc -v rcmsgs.mc

$(NTTARGETFILE0): msg00001.bin rcdll.rc rcmsgs.h
