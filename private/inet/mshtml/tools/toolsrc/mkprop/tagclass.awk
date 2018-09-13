BEGIN {

    etags = 0
    classes = 0

#    print " in element"
    while (getline < "..\\base\\element.hxx" > 0) {
        if ($1 ~ /ETAG_/) {
            split($1, t, ",")
            etagt[etags] = t[1]
#            print t[1]
            etags++
            tags[t[1]] = 1
         }
    }
    
    
#    print " in parser "
    while (getline > 0) {
        if ($2 == "CTagClass")  {
            class = $3
            classt[classes] = class
#            print class
            classes++
            linest[class, 0] = $0
            getline
            linest[class, 1] = $0
            getline
            linest[class, 2] = $0
            getline
        }
        else {
            if ($1 ~ /ETAG/) {
                cancontain[class, $1] = 1
                if (!($1 in tags)) {
                    print "******* unknown tag: " $1
                }
#                print $1
            }
        }
    }
}
END {
    for (i = 0;  i < classes; i++)
    {
#        print "//** " classt[i]
        print linest[classt[i], 0]
        print "{"
        print linest[classt[i], 1] ","
        print linest[classt[i], 2] ","
        print ""
        for (j = 0; j < etags; j++) {
            if (cancontain[classt[i], etagt[j]]) {
                print "\t1, //" etagt[j]
            }
            else {
                print "\t0, //" etagt[j]
            }
        }
        print "};\n"
    }
}
        