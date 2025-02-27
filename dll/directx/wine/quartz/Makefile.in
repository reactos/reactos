MODULE       = quartz.dll
IMPORTLIB    = quartz
IMPORTS      = strmiids dxguid strmbase uuid dsound msacm32 msvfw32 ole32 oleaut32 rpcrt4 user32 gdi32 advapi32 winmm msvcrt
DELAYIMPORTS = ddraw

SOURCES = \
	acmwrapper.c \
	avidec.c \
	control_tlb.idl \
	decoder.c \
	dsoundrender.c \
	filesource.c \
	filtergraph.c \
	filtermapper.c \
	main.c \
	memallocator.c \
	parser.c \
	passthrough.c \
	quartz.rc \
	quartz_strmif.idl \
	systemclock.c \
	videorenderer.c \
	vmr7.c \
	vmr7_presenter.c \
	vmr9.c \
	window.c

dlldata_EXTRADEFS = -DENTRY_PREFIX=QUARTZ_ -DPROXY_DELEGATION -DWINE_REGISTER_DLL
