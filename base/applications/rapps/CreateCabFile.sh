#/bin/sh

if [ "$#" != "3" ]; then
    echo "Usage: CreateCabFile.sh path/to/cabman path/to/utf16le path/to/rapps-db"
    exit -1
fi

CABMAN_CMD="$1"
UTF16LE_CMD="$2"
RAPPSDB_PATH="$3"

mkdir "$RAPPSDB_PATH/utf16"

echo Converting txt files to utf16
for filename in $RAPPSDB_PATH/*.txt; do
    just_filename=$(basename -- "$filename")
    $UTF16LE_CMD "$filename" "$RAPPSDB_PATH/utf16/$just_filename"
done

echo Building rappmgr.cab
$CABMAN_CMD -M mszip -S "$RAPPSDB_PATH/rappmgr.cab" "$RAPPSDB_PATH/utf16/*.txt"

echo Building rappmgr2.cab
$CABMAN_CMD -M mszip -S "$RAPPSDB_PATH/rappmgr2.cab" "$RAPPSDB_PATH/utf16/*.txt" -F icons "$RAPPSDB_PATH/icons/*.ico"

echo Cleaning up
rm -r "$RAPPSDB_PATH/utf16"

echo Done
