#! /usr/bin/awk -f
# This is an awk script which does dependencies. We do NOT want it to
# recursively follow #include directives.
# We only add to dependencies those files which are inside of the rootdir 
# tree :)

#
# Surely there is a more elegant way to see if a file exists.  Anyone know
# what it is?
#
function fileExists(f,    TMP, dummy, result) {
	if(result=FILEHASH[f]) {
		if(result=="Yes") {
			return "Yes"
		} else {return ""}
	}
	ERRNO = getline dummy < f
	if(ERRNO >= 0) {
		close(f)
		return FILEHASH[f]="Yes"
	} else {
		FILEHASH[f]="No"
		return ""
	}
}

function Canonic(path) {
	while (path ~ "/[^/]*/\\.\\./")
	    gsub("/[^/]*/\\.\\./","/",path)
	return path
}

BEGIN{
	hasdep=0
	objprefix=""
	USEDC=0
	if(dolib) {
	    # dolib = "libdirectory libname"
	    split(dolib, dlib)
	    I=0
	    rootdir=srcdir
	    sub("/$","",rootdir)
	    sub("/[^/]*$","",rootdir)
	    while (getline > 0) {
		if ($0 ~ "OBJS") {
		    objs=$0
		} else if ($0 ~ "^/.*\\.h:  \\\\$") {
		    sub(":  \\\\$","",$0)
		    USED[USEDC]=$0
		    ++USEDC
		}
	    }
	    sub("^OBJS=[ ]*\"[ ]*","",objs)
	    sub("\"[ ]*","",objs)
	    split(objs, obj)
	    printf "%s: ", dlib[2]
	    sub("/$","", dlib[1])
	    objprefix=dlib[1]"/"
	    for (fname in obj) {
		fullname=dlib[1]"/"obj[fname]
		printf " \\\n   %s", fullname
		sub("\\.o$",".c",obj[fname])
		ARGV[ARGC]=obj[fname]
		++ARGC
	    }
	    printf "\n"
	}
	if(!hpath) {
	    print "hpath is not set"
	    exit 1
	}
	if(!srcdir) {
	    print "srcdir is not set"
	    exit 1
	}
	sub("[/ ]*$","",srcdir)
	srcdir=srcdir"/"
	sub("^\./$","",srcdir)
	split(hpath, parray)
	for(path in parray) {
	    sub("^-I","",parray[path])
	    sub("[/ ]*$","",parray[path])
	    parray[path]=Canonic(parray[path])
	}
	for(path in ARGV) {
	    USED[USEDC]=Canonic(srcdir""ARGV[path])
	    ++USEDC
	}
}

/^#[ 	]*include[ 	]*[<"][^ 	]*[>"]/{
	found=0
	if(LASTFILE!=FILENAME) {
		if (hasdep) {
			print cmd
			hasdep=0
		}
		cmd=""
		LASTFILE=FILENAME
		depname=FILENAME
		relpath=FILENAME
		sub("\\.c$",".o: ",depname)
		if (depname==FILENAME) {
			depname=srcdir""depname
			depname=Canonic(depname)
			cmd="\n\t@touch "depname
		} else
			depname=objprefix""depname
		sub("\\.h$",".h: ",depname)
		if(relpath ~ "^\\." ) {
			sub("[^/]*$","",  relpath)
			relpath=relpath"/"
			sub("//","/",  relpath)
		} else {
			relpath=""
		}
	}
	fname=$0
	sub("^#[ 	]*include[ 	]*[<\"]","",fname)
	sub("[>\"].*","",fname)
	if(fileExists(relpath""fname)) {
		found=1
		if (!hasdep) {
			printf "%s", depname
			hasdep=1
		}
		fullname=Canonic(srcdir""relpath""fname)
		printf " \\\n   %s", fullname
		if(fname ~ "^\\." ) {
		    partname=relpath""fname
		    afound=0
		    for(name in USED) {
			if (USED[name] == fullname) {
			    afound=1
			    break
			}
		    }
		    if (!afound) {
			ARGV[ARGC]=partname
			++ARGC
			USED[USEDC]=fullname
			++USEDC
		    }
		}
	} else {
		for(path in  parray) {
			if(fileExists(parray[path]"/"fname)) {
				found=1
				if (!hasdep) {
					printf "%s", depname
					hasdep=1
				}
				printf " \\\n   %s", parray[path]"/"fname
			}
		}
	}
}

END{
	if (hasdep) {
		print cmd
	}
}
