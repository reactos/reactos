# /bin/sh

nasm >/dev/null

if [[ "$?" == "0" || "$?" == "1" ]]; then
	echo "all: samples_all"         >samples/Makefile.nasm
	echo "NASM=nasm"               >>samples/Makefile.nasm
else
	echo "#nothing to do here"      >samples/Makefile.nasm
	echo "all:"                    >>samples/Makefile.nasm
	echo -n -e "\t"                >>samples/Makefile.nasm
	echo "echo You do not have NASM... skipping sample build" >>samples/Makefile.nasm
fi

