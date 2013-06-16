# bin/bash
#---------------------------
# install script for Softx86
# (C) 2003 Jonathan Campbell
#   <jcampbell@mdjk.com>
#---------------------------
# this script automatically generated
 
exit_if_failed() {
	if [[ !( $? -eq 0 ) ]]; then exit 1; fi
	return 0;
}
 
installcopy2() {
	echo "Installing Softx86 libraries+headers"
	exit_if_failed
	cp lib/libsoftx86.a /usr/lib/libsoftx86.a
	exit_if_failed
	cp lib/libsoftx86.so /usr/lib/libsoftx86.so
	exit_if_failed
	cp include/softx86.h /usr/include/softx86.h
	exit_if_failed
	cp include/softx86cfg.h /usr/include/softx86cfg.h
	exit_if_failed
	chmod 644 /usr/lib/libsoftx86.a
	exit_if_failed
	chmod 644 /usr/lib/libsoftx86.so
	exit 0;
}

install87copy2() {
	echo "Installing Softx87 libraries+headers"
	exit_if_failed
	cp lib/libsoftx87.a /usr/lib/libsoftx87.a
	exit_if_failed
	cp lib/libsoftx87.so /usr/lib/libsoftx87.so
	exit_if_failed
	cp include/softx87.h /usr/include/softx87.h
	exit_if_failed
	chmod 644 /usr/lib/libsoftx87.a
	exit_if_failed
	chmod 644 /usr/lib/libsoftx87.so
	exit 0;
}
 
installcopy() {
	if (installcopy2);
	then echo "Installation done";
	else echo "Installation failed";
	fi
 
	return 0;
}

install87copy() {
	if (install87copy2);
	then echo "Installation done";
	else echo "Installation failed";
	fi
}
 
if [[ -f lib/libsoftx86.a && -f lib/libsoftx86.so ]];
then installcopy;
else echo "You must build this project first";
exit 0;
fi

if [ -d softx87 ]; then
	if [[ -f lib/libsoftx87.a && -f lib/libsoftx87.so ]];
	then install87copy;
	else echo "You must build softx87 first";
	exit 0;
	fi
fi

#end
