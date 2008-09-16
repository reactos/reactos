#!/bin/sh
# Copyright (C) 2001-2007, International Business Machines
#   Corporation and others.  All Rights Reserved.
#
# Authors:
# Ami Fixler
# Steven R. Loomis
# George Rhoten
#
# Shell script to unpax ICU and convert the files to an EBCDIC codepage.
# After extracting to EBCDIC, binary files are re-extracted without the
# EBCDIC conversion, thus restoring them to original codepage.
#
# Set the following variable to the list of binary file suffixes (extensions)

#binary_suffixes='ico ICO bmp BMP jpg JPG gif GIF brk BRK'
#ICU specific binary files
binary_suffixes='brk BRK bin BIN res RES cnv CNV dat DAT icu ICU spp SPP xml XML'

usage()
{
    echo "Enter archive filename as a parameter: $0 icu-archive.tar"
}
# first make sure we at least one arg and it's a file we can read
if [ $# -eq 0 ]; then
    usage
    exit
fi
tar_file=$1
if [ ! -r $tar_file ]; then
    echo "$tar_file does not exist or cannot be read."
    usage
    exit
fi

echo ""
echo "Extracting from $tar_file ..."
echo ""
# extract files while converting them to EBCDIC
pax -rvf $tar_file -o to=IBM-1047,from=ISO8859-1 -o setfiletag

echo ""
echo "Determining binary files ..."
echo ""

# When building in ASCII mode, text files are converted as ASCII
if [ "${ICU_ENABLE_ASCII_STRINGS}" -eq 1 ]; then
    binary_suffixes="$binary_suffixes txt TXT ucm UCM"
else
	for file in `find ./icu \( -name \*.txt -print \) | sed -e 's/^\.\///'`; do
		bom8=`head -c 3 $file|\
			od -t x1|\
			head -n 1|\
			sed 's/  */ /g'|\
			cut -f2-4 -d ' '|\
			tr 'A-Z' 'a-z'`;
		#Find a converted UTF-8 BOM
		if [ "$bom8" = "57 8b ab" ]
		then
			binary_files="$binary_files $file";
		fi
	done
fi

for i in $(pax -f $tar_file 2>/dev/null)
do
	case $i in
	*/) ;;		# then this entry is a directory
	*.*)		# then this entry has a dot in the filename
		for j in $binary_suffixes
		do
			# We substitute the suffix more than once
			# to handle files like NormalizationTest-3.2.0.txt
			suf=${i#*.*}
			suf=${suf#*.*}
			suf=${suf#*.*}
			if [ "$suf" = "$j" ]
			then
				binary_files="$binary_files $i"
				break
			fi
		done
		;;
	*) ;;		# then this entry does not have a dot in it
    esac
done

# now see if a re-extract of binary files is necessary
if [ ${#binary_files} -eq 0 ]; then
    echo ""
    echo "There are no binary files to restore."
else
    echo "Restoring binary files ..."
    echo ""
    rm $binary_files
    pax -rvf $tar_file $binary_files
    # Tag the files as binary for proper interaction with the _BPXK_AUTOCVT
    # environment setting
    chtag -b $binary_files
fi
echo ""
echo "$0 has completed extracting ICU from $tar_file."
