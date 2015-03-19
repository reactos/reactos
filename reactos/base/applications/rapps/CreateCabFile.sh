#/bin/sh
cd rapps
mkdir utf16
for i in $(find -type f); do
  ../../../../output-MinGW-i386/host-tools/tools/utf16le.exe $i utf16/$i
done
cd ..
../../../output-MinGW-i386/host-tools/tools/cabman/cabman -M mszip -S rappmgr.cab rapps/utf16/*.txt
rm -r rapps/uft16