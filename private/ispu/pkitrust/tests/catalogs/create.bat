
makecat -v test.cdf

signcode -spc publish.spc -v publish.pvk -n "Peter's Catalog Test" -i "www.microsoft.com" -t "http://timestamp.verisign.com/scripts/timstamp.dll" test.p7s
