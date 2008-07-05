#!/bin/sh

es=1
if [ $# -eq 0 ] ; then
	exec tr -d '\015\032'
elif [ ! -f "$1" ] ; then
	echo "Not found: $1" 1>&2
else
	for f in "$@" ; do
		if tr -d '\015\032' < "$f" > "$f.tmp" ; then
			if cmp "$f" "$f.tmp" > /dev/null ; then
				rm -f "$f.tmp"
			else
				touch -r "$f" "$f.tmp"
				if mv "$f" "$f.bak" ; then
					if mv "$f.tmp" "$f" ; then
						rm -f "$f.bak"
						es=$?
						echo "  converted $f"
					else
						rm -f "$f.tmp"
					fi
				else
					rm -f "$f.tmp"
				fi
			fi
		else
			rm -f "$f.tmp"
		fi
	done
fi

exit $es
