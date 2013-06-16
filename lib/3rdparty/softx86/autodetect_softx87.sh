# /bin/sh

if [ -d ./softx87 ]; then
	echo "INCLUDZ += -DSOFT87FPU"   >softx86dbg/Makefile.softx87
	echo "LIBINCLUDZ += -lsoftx87" >>softx86dbg/Makefile.softx87
else
	echo "#nothing to do here"      >softx86dbg/Makefile.softx87
fi

