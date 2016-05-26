#/bin/sh
cd ../../../media
mkdir rapps/utf16
for i in $(find -type f); do
  ../../host-tools/utf16le $i utf16/$i
done
cd ..
../../host-tools/cabman -M mszip -S rapps/rappmgr.cab rapps/utf16/*.txt
rm -r rapps/uft16