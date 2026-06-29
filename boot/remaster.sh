#!/bin/sh
# shellcheck disable=SC3003,SC3045

##
## PROJECT:     ReactOS ISO Remastering Script
## LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
## PURPOSE:     Allows easy remastering of customized ReactOS ISO images.
##              Based on the boot/boot_images.cmake script in the ReactOS
##              source tree. Requires a MKISOFS-compatible utility.
## COPYRIGHT:   Copyright 2025 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
##
## POSIX shell compliance checker: https://www.shellcheck.net
##

## echo -ne "\033]0;ReactOS ISO Remastering Script\007"

##
## Customizable settings
##
## ISO image identifier names
ISO_MANUFACTURER="ReactOS Project"  # For both the publisher and the preparer
ISO_VOLNAME="ReactOS"               # For both the Volume ID and the Volume set ID

## Image names of the MKISOFS and ISOHYBRID tools
MKISOFS=mkisofs
ISOHYBRID=isohybrid


##
## Main script
##
clear
echo "*******************************************************************************"
echo "*                                                                             *"
echo "*                       ReactOS ISO Remastering Script                        *"
echo "*                                                                             *"
echo "*******************************************************************************"
echo

exit_script()
{
    ERRLVL=${1:-$?} # Use parameter, otherwise default to $?
    [ "$ERRLVL" -eq 0 ] && echo "Success!"
    # shellcheck disable=SC3045
    read -n 1 -r -s -p $'Press any key to quit...\n'
    exit "$ERRLVL"
}

##
## Prompt the user for a choice.
## Usage example:
##   choice YN "Yes or No [Y,N]? "
##
## Return in $REPLY the index of the reply in the choices list.
##
## See also:
## https://stackoverflow.com/q/226703
##
choice()
{
    ## Normalize the choices list to lowercase, and prepare
    ## the pattern to match for only one single character.
    CHOICE_LIST="$(echo "$1" | tr '[:upper:]' '[:lower:]')"
    CHOICE_PATTERN="^[$CHOICE_LIST]$"

    ## Echo prompt and wait for keypress
    shift 1
    # shellcheck disable=SC3037
    echo -n "$@"
    REPLY_LOWER=
    until # Adapted from https://unix.stackexchange.com/a/249036
        # shellcheck disable=SC3045
        read -N 1 -r -s
        REPLY_LOWER="$(echo "$REPLY" | tr '[:upper:]' '[:lower:]')"
        expr "$REPLY_LOWER" : "$CHOICE_PATTERN" 1>/dev/null
    do :; done
    echo "$REPLY"

    ## Return the index of the reply in the choices list
    #REPLY=$(expr index "$CHOICE_LIST" "$REPLY_LOWER")
    REPLY=${CHOICE_LIST%%"$REPLY_LOWER"*}
    #REPLY=$(( $(expr "$REPLY" : ".*")+1 ))
    REPLY=$(( ${#REPLY}+1 ))
}


## Verify that we have access to a temporary directory.
for TMPDIR in "$TMPDIR" "$TEMP" "$TMP" "/tmp"; do
    [ -n "$TMPDIR" ] && [ -d "$TMPDIR" ] && break # Valid directory found
done
if [ -z "$TMPDIR" ] || [ ! -d "$TMPDIR" ]; then
    echo No temporary directory exists on your system.
    echo Please create one and assign it to the TMPDIR environment variable.
    echo
    exit_script 1
fi


## Try to auto-locate MKISOFS and if not, prompt the user for a directory.
TOOL_DIR=
TOOL_PATH=$(type -p $MKISOFS)
if [ -z "$TOOL_PATH" ]; then
    read -e -r -p $'Please enter the directory path where '$MKISOFS$' can be found:\n' TOOL_DIR
    echo
    # Non-POSIX-compatible test: [ "${TOOL_DIR:(-1)}" = "/" ]
    [ "${TOOL_DIR#"${TOOL_DIR%?}"}" = "/" ] && TOOL_DIR=${TOOL_DIR%?}
    TOOL_PATH=$TOOL_DIR/$MKISOFS
else
    # Get the directory without the '/filename' part (and doesn't include trailing /)
    TOOL_DIR=${TOOL_PATH%/*}
fi
MKISOFS=$TOOL_PATH


read -e -r -p $'Please enter the path of the directory tree to image into the ISO:\n' INPUT_DIR
echo
read -e -r -p $'Please enter the file path of the ISO image that will be created:\n' OUTPUT_ISO
echo


## Retrieve the full paths to the 'isombr', 'isoboot', 'isobtrt' and 'efisys' files
isombr_file=loader/isombr.bin
isoboot_file=loader/isoboot.bin
isobtrt_file=loader/isobtrt.bin
efisys_file=loader/efisys.bin

#choice 12 $'Please choose the ISO boot file: 1) isoboot.bin ; 2) isobtrt.bin\n[default: 1]: '
choice YN $'Do you want the ReactOS media to wait for a key-press before booting [Y,N]? '
echo
ISOBOOT_PATH=$isoboot_file
if [ "$REPLY" -eq 1 ]; then
    ISOBOOT_PATH=$isoboot_file
elif [ "$REPLY" -eq 2 ]; then
    ISOBOOT_PATH=$isobtrt_file
fi

## Enable (U)EFI boot support if possible: check the
## presence of '$efisys_file' in the ISO directory tree.
#ISO_EFI_BOOT_PARAMS
ISO_BOOT_EFI_OPTIONS=$([ -f "$INPUT_DIR/$efisys_file" ] && \
    echo "-eltorito-alt-boot -eltorito-platform efi -eltorito-boot $efisys_file -no-emul-boot")

## Summary of the boot files.
echo "ISO boot file: '$ISOBOOT_PATH'"
[ -n "$ISO_BOOT_EFI_OPTIONS" ] && echo "EFI boot file: '$efisys_file'"
echo


choice YN $'Do you want to store duplicated files only once (reduces the size\nof the ISO image) [Y,N]? '
echo
DUPLICATES_ONCE=$([ "$REPLY" -eq 1 ] && echo "-duplicates-once")


echo "Creating the ISO image..."
echo

## Create a mkisofs sort file to specify an explicit ordering for the boot files
## to place them at the beginning of the image (makes ISO image analysis easier).
## See mkisofs/schilytools/mkisofs/README.sort and boot/boot_images.cmake script
## in the ReactOS source tree for more details.

## echo ${CMAKE_CURRENT_BINARY_DIR}/empty/boot.catalog 4
cat > "$TMPDIR/bootfiles.sort" << EOF
boot.catalog 4
$INPUT_DIR/$isoboot_file 3
$INPUT_DIR/$isobtrt_file 2
$INPUT_DIR/$efisys_file 1
EOF

## Finally, create the ISO image proper.
#echo "Running command:
#$MKISOFS \
#    -o \"$OUTPUT_ISO\" -iso-level 4 \
#    -publisher \"$ISO_MANUFACTURER\" -preparer \"$ISO_MANUFACTURER\" \
#    -volid \"$ISO_VOLNAME\" -volset \"$ISO_VOLNAME\" \
#    -eltorito-boot $ISOBOOT_PATH -no-emul-boot -boot-load-size 4 \
#    $ISO_BOOT_EFI_OPTIONS \
#    -hide boot.catalog -sort \"$TMPDIR/bootfiles.sort\" \
#    $DUPLICATES_ONCE -no-cache-inodes \"$INPUT_DIR\"
#"
$MKISOFS \
    -o "$OUTPUT_ISO" -iso-level 4 \
    -publisher "$ISO_MANUFACTURER" -preparer "$ISO_MANUFACTURER" \
    -volid "$ISO_VOLNAME" -volset "$ISO_VOLNAME" \
    -eltorito-boot $ISOBOOT_PATH -no-emul-boot -boot-load-size 4 \
    $ISO_BOOT_EFI_OPTIONS \
    -hide boot.catalog -sort "$TMPDIR/bootfiles.sort" \
    $DUPLICATES_ONCE -no-cache-inodes "$INPUT_DIR"; ERRLVL=$?
## -graft-points -path-list "some/directory/iso_image.lst"
echo
rm "$TMPDIR/bootfiles.sort"

if [ "$ERRLVL" -ne 0 ]; then
    echo "An error $ERRLVL happened while creating the ISO image \"$OUTPUT_ISO\"."
    exit_script $ERRLVL
fi
echo "The ISO image \"$OUTPUT_ISO\" has been successfully created."
echo


## Check whether ISOHYBRID is also available and if so, propose to post-process
## the generated ISO image to allow hybrid booting as a CD-ROM or as a hard disk.
TOOL_PATH=$TOOL_DIR/$ISOHYBRID
if [ -z "$TOOL_PATH" ] || [ ! -f "$TOOL_PATH" ]; then
    TOOL_PATH=$(type -p $ISOHYBRID)
    if [ -z "$TOOL_PATH" ]; then
        echo "$ISOHYBRID patching skipped."
        exit_script 0
    fi
fi
ISOHYBRID=$TOOL_PATH


choice YN $'Do you want to post-process the ISO image to allow hybrid booting\nas a CD-ROM or as a hard disk [Y,N]? '
echo
[ "$REPLY" -ne 1 ] && exit_script 0

echo "Patching the ISO image..."

"$ISOHYBRID" -b "$INPUT_DIR/$isombr_file" -t 0x96 "$OUTPUT_ISO"; ERRLVL=$?
echo

if [ "$ERRLVL" -ne 0 ]; then
    echo "An error $ERRLVL happened while patching the ISO image \"$OUTPUT_ISO\"."
    exit_script $ERRLVL
## else
##     echo "The ISO image \"$OUTPUT_ISO\" has been successfully patched."
fi
echo

exit_script 0
