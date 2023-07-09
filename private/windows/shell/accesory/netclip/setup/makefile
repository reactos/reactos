OUT_DIR=ship

all: iexpress

iexpress: $(OUT_DIR)\nc$(CPU).exe

clean:
	if exist *.cab del *.cab
	if exist $(OUT_DIR)\nc$(CPU).exe del $(OUT_DIR)\nc$(CPU).exe
    if exist netclip.exe del netclip.exe
    if exist nclipps.dll del nclipps.dll
    if exist *.dbg del *.dbg

############## IExperss for netclip proper #######################

$(OUT_DIR)\nc$(CPU).exe: nc$(CPU).cdf netclip.inf netclipfiles
	@-if not exist $(OUT_DIR) md $(OUT_DIR)
    splitsym -a netclip.exe
    splitsym -a nclipps.dll
	iexpress /n nc$(CPU).cdf
    @copy nc$(CPU).exe $(OUT_DIR)
    @del nc$(CPU).exe

netclipfiles: 
    @copy ..\netclip\obj\$(CPU)\netclip.exe .
    @copy ..\proxy\obj\$(CPU)\nclipps.dll .
