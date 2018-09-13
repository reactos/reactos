###############################################################################################
# This awk program is processing a file containing a list of HXX header file names.
#   In the header files it looks for the following 'special' names:
#
#   AWK_DECLARE_ENUM(enum type, [prefix], [guid])
#
#    should be inside DECLARE_ENUM
#   AWK_ENUM(C++ name, user name, [value])   
#       
#   AWK_DECLARE_CLASS(class, base class)
#
#    should be inside DECLARE_CLASS or DECLARE_INTERFACE
#   AWK_PROPERTY(property name(automation type, dispid, [attribute:value], [automation attribute], [get], [set], [bag] | [spf] | [scf] ))
#       where
#           [attribute:value] defines attributes for the internal property description
#               type:    the internal type of a member variable
#               member:  the name of the member variable used to store the property
#               method:  the name of a pair get/set helper methods prefixed by Get/Set getting and setting the value
#               help:    the help id for the typelib
#               szattribute:  when set, uses this as the html string instead of the automation name
#               dwExtra: 32 bits of extra information
#           [automation attribute]
#               any automation attribute which should go to the type library
#           [get]   
#               if the property is readable
#           [set]
#               if the property writable
#           [bag]
#               if the property is in our special attribute bag
#
#   AWK_DECLARE_INTERFACE(interface, base interface)
#
#    should be inside DECLARE_INTERFACE
#   AWK_REF_PROPERTY(class:property)
#
###############################################################################################

#function CheckFile(s) {
#    print "// " s > s
#    close(s)
#    if (getline < s <= 0 || $2 != s) {
#        print "*** AWK *** Couldn't write into \"" s "\""
#    }
#    close(s)
#}

function CalculateMask(enum, i, m)
{
    m = 0
    for (i = et[enum] - 1; i >= 0; i--)
    {
        if (ev[enum, i] < 0 || ev[enum, i] > 31)
        {
            return 0
        }
        m += 2 ^ ev[enum, i]
    }
    return m
}

function GeneratePropertyDesc(class, file, i, j, s, m, b) {

    print "In class: " class > fileLog

    for (i = 0; i < ct[class]; i++) {

        if (pt[class, i, "func"])
            continue

        print "Property: " pt[class, i, "name"] > fileLog

        for (j = pt[class, i] - 1; j > 0; j--) {
            print "\t" pa[class, i, j] " : " pt[class, i, pa[class, i, j]] > fileLog
        }

        s = pt[class, i, "type"]
        if (s in et) {
            m = CalculateMask(s)
            b = "ENUM"
            s = "Enum"
        }
        else {
            b = bt[s]
            s = dt[s]
        }
        print class ", " pt[class, i, "name"] ", " pt[class, i, "dispid"] ", " pt[class, i, "dwflags"] > fileLog

        print "" >> file

        propdescstruct = "PROPDESC" toupper(class) toupper(pt[class, i, "name"])

        if (pt[class, i, "szattribute"]) {
            htmldesc = pt[class, i, "szattribute"]
        }
        else {
            htmldesc = pt[class, i, "name"]
        }

        if (pt[class, i, "abstract"]) {
            propertydesc = "s_propdesc" class pt[class, i, "name"] " =\n{\n\tNULL, _T(\"" htmldesc "\"), (ULONG)" pt[class, i, "default"] ",\n\t{"
        }
        else {
            propertydesc = "s_propdesc" class pt[class, i, "name"] " =\n{\n\tPROPERTYDESC::Handle" s "Property, _T(\"" htmldesc "\"), (ULONG)" pt[class, i, "default"] ",\n\t{"
        }

        if (pt[class, i, "method"]) {
            memberdesc = ""
        }
        else if (pt[class, i, "bag"]) {
            memberdesc = class "::CLAttrBag, " pt[class, i, "member"]
        }
        else if (pt[class, i, "spf"]) {
            memberdesc = "CParaFormat, " pt[class, i, "member"]
        }
        else if (pt[class, i, "scf"]) {
            memberdesc = "CCharFormat, " pt[class, i, "member"]
        }
        else if (pt[class, i, "indirect"]) {
            memberdesc = class pt[class, i, "indirect"] ", " pt[class, i, "member"]
        }
        else {
            memberdesc = class ", " pt[class, i, "member"]
        }

        propparamdesc = (pt[class, i, "method"] ? "PROPPARAM_GET | PROPPARAM_SET" : "PROPPARAM_MEMBER") 
        if (pt[class, i, "ppflags"]) {
            propparamdesc = propparamdesc " | " pt[class, i, "ppflags"]
        }

        if (pt[class, i, "bag"]) {
            propparamdesc = propparamdesc " | PROPPARAM_BAG | PROPPARAM_INDIRECT"
        }
        else if (pt[class, i, "spf"]) {
            propparamdesc = propparamdesc " | PROPPARAM_INDIRECT | PROPPARAM_STYLE_PF"
        }
        else if (pt[class, i, "scf"]) {
            propparamdesc = propparamdesc " | PROPPARAM_INDIRECT | PROPPARAM_STYLE_CF"
        }
        else if (pt[class, i, "indirect"]) {
            propparamdesc = propparamdesc " | PROPPARAM_INDIRECT"
        }

        if (pt[class, i, "inherit"]) {
            propparamdesc = propparamdesc " | PROPPARAM_INHERITED"
        }
        if (s == "Num" || s == "Enum" || s == "UnitValue" ) {
            if (pt[class, i, "method"]) {
                print "EXTERN_C const struct " propdescstruct " { PROPERTYDESC a; NUMPROPPARAMS b; PFN_NUMPROPGET c; PFN_NUMPROPSET d; }" >> file
            }
            else if (pt[class, i, "abstract"]) {
                print "EXTERN_C const struct " propdescstruct " { PROPERTYDESC a; NUMPROPPARAMS b; }" >> file
            }
            else if (pt[class, i, "dwextra"])
            {
                print "EXTERN_C const struct " propdescstruct " { PROPERTYDESC a; NUMPROPPARAMS b; DWORD c; DWORD d; }" >> file
            }
            else {
               print "EXTERN_C const struct " propdescstruct " { PROPERTYDESC a; NUMPROPPARAMS b; DWORD c; }" >> file
            }

            print propertydesc "\n\t{" >> file

            if (s == "Enum") {
                propparamdesc = propparamdesc " | PROPPARAM_ENUM"
            }

            if (pt[class, i, "dispid"]) {
                print "\t\t" propparamdesc ", " pt[class, i, "dispid"] ", " pt[class, i, "dwflags"]  " \n\t}," >> file
            }
            else {
                print "\t\t" propparamdesc ", 0, " pt[class, i, "dwflags"] " \n\t}," >> file
            }

            if (pt[class, i, "abstract"]) {
                printf("\t\t0, 0") >> file
            }
            else {
                if (pt[class, i, "vt"] == "") {
                    pt[class, i, "vt"] = (pt[class, i, "atype"] == "short") ? "VT_I2" : "VT_I4"
                }
                if (pt[class, i, "method"]) {
                    printf("\t\t%s, 0", pt[class, i, "vt"]) >> file
                }
                else {
                    printf("\t\t%s, SIZE_OF(%s)", pt[class, i, "vt"], memberdesc) >> file
                }
            }
            if (s == "Enum") {
                if (pt[class, i, "abstract"]) {
                    printf(", 0, 0,\n") >> file
                }
                else {
                    printf(", %d, (long)&s_enumdesc%s,\n", m, pt[class, i, "type"]) >> file
                }
            }
            else {
                if (pt[class, i, "min"] == "") {
                    pt[class, i, "min"] = "LONG_MIN"
                }
                if (pt[class, i, "max"] == "") {
                    pt[class, i, "max"] = "LONG_MAX"
                }
                printf(", %s, %s,\n", pt[class, i, "min"], pt[class, i, "max"]) >> file 
            }

            print "\t}," >> file

            if (pt[class, i, "method"]) {
                print "\tPFN_NUMPROPGET(PFNB_NUMPROPGET(" class" ::Get" pt[class, i, "method"] "))," >> file
                print "\tPFN_NUMPROPSET(PFNB_NUMPROPSET(" class" ::Set" pt[class, i, "method"] "))" >> file
            }
            else if (pt[class, i, "abstract"]) {
            }
            else {
                print "\toffsetof(" memberdesc ")" >> file
            }
        }
        else
        {
            if (pt[class, i, "method"]) {
                print "EXTERN_C const struct " propdescstruct " { PROPERTYDESC a; BASICPROPPARAMS b; PFN_" b "PROPGET c; PFN_" b "PROPSET d; }" >> file
            }
            else if (pt[class, i, "abstract"]) {
                print "EXTERN_C const struct " propdescstruct " { PROPERTYDESC a; BASICPROPPARAMS b; }" >> file
            }
            else if (pt[class, i, "dwextra"])
            {
                print "EXTERN_C const struct " propdescstruct " { PROPERTYDESC a; BASICPROPPARAMS b; DWORD c; DWORD d; }" >> file
            }
            else {
                print "EXTERN_C const struct " propdescstruct " { PROPERTYDESC a; BASICPROPPARAMS b; DWORD c; }" >> file
            }

            print propertydesc >> file

            if (pt[class, i, "dispid"]) {
                print "\t\t" propparamdesc ", " pt[class, i, "dispid"] ", " pt[class, i, "dwflags"] " \n\t}," >> file
            }
            else {
                print "\t\t" propparamdesc ", 0, " pt[class, i, "dwflags"] " \n\t}," >> file
            }

            if (pt[class, i, "method"]) {
                print "\tPFN_" b "PROPGET(PFNB_" b "PROPGET(" class" ::Get" pt[class, i, "method"] "))," >> file
                print "\tPFN_" b "PROPSET(PFNB_" b "PROPSET(" class" ::Set" pt[class, i, "method"] "))" >> file
            }
            else if (pt[class, i, "abstract"]) {
            }
            else {
                print "\toffsetof(" memberdesc ")" >> file
            }
        }
        if (pt[class, i, "dwextra"])
        {
            print "\t, " pt[class, i, "dwextra"] "\n" >> file
        }
        print "};\n" >> file
    }
}

function GenerateExternProp(class, file, i) {

    if (ca[class, "super"] != "") {
        GenerateExternProp(ca[class, "super"], file)
    }

    print "In class: " class > fileLog

    for (i = 0; i < ct[class]; i++) {

        if (pt[class, i, "func"])
            continue

        print "Extern property: " pt[class, i, "name"] > fileLog

#        print "\nEXTERN_C struct PROPDESC" toupper(class) toupper(pt[class, i, "name"]) " s_propdesc" class pt[class, i, "name"] ";\n" >> file
        print "\nEXTERN_C struct PROPERTYDESC s_propdesc" class pt[class, i, "name"] ";\n" >> file
    }
}

function GenerateReferProp(class, file, i) {

    if (ca[class, "super"] != "") {
        GenerateReferProp(ca[class, "super"], file)
    }

    print "In class: " class > fileLog

    for (i = 0; i < ct[class]; i++) {

        if (pt[class, i, "func"])
            continue

        print "Property: " pt[class, i, "name"] > fileLog

        if (!ca[class, "imported"]) {
            print "\t&s_propdesc" class pt[class, i, "name"] ".a," >> file
        }
        else {
            print "\t&s_propdesc" class pt[class, i, "name"] "," >> file
        }
    }
}

function GenerateTearOffMethods(interface, file, i, j, a, ps, class) {

    if (ia[interface, "super"] != "") {
        GenerateTearOffMethods(ia[interface, "super"], file)
    }

    for (o = 0; o < it[interface]; o++) {

        class = ic[interface, o]
        i = pi[class, im[interface, o]]

        if (pt[class, i, "func"]) {
            print "\t(HRESULT (__stdcall CVoid::*)(void))" >> file
            printf("\t\t(%s (__stdcall CVoid::*)(", pt[class, i, "return"]) >> file

            a = 0
            for (j = 0; j < pt[class, i, "func"]; j++) {
                if (a) {
                    printf(", ") >> file
                }
                a++
                printf("%s", pt[class, i, j, "type"]) >> file
            }
            print "))" pt[class, i, "name"] "," >> file
        }
        else {
            if (pt[class, i, "index"]) {
                ps = pt[class, i, "indextype"] ", "
            }
            else {
                ps = ""
            }
            if (pt[class, i, "index1"]) {
                ps = ps pt[class, i, "indextype1"] ", "
            }

            if (pt[class, i, "set"]) {
                print "\t(HRESULT (__stdcall CVoid::*)(void))" >> file
                print "\t\t(HRESULT (__stdcall CVoid::*)(" ps pt[class, i, "atype"] "))Set" pt[class, i, "name"] "," >> file

                if (pt[class, i, "object"]) {
                    print "\t(HRESULT (__stdcall CVoid::*)(void))" >> file
                    print "\t\t(HRESULT (__stdcall CVoid::*)(" ps pt[class, i, "atype"] "))Set" pt[class, i, "name"] "ByRef," >> file
                }
            }
            if (pt[class, i, "get"]) {
                print "\t(HRESULT (__stdcall CVoid::*)(void))" >> file
                print "\t\t(HRESULT (__stdcall CVoid::*)(" ps pt[class, i, "atype"] "*))Get" pt[class, i, "name"] "," >> file
            }
        }
    }
}

function GeneratePropMethodDecl(class, file, i, j, a, aa, ps) {

    print "In class: " class > fileLog

    print "// Property get/set method declarations for class " class >> file

    for (i = 0; i < ct[class]; i++) {

        if (pt[class, i, "func"]) {

            print "Method: " pt[class, i, "name"] > fileLog

            printf("\tvirtual %s STDMETHODCALLTYPE %s(", pt[class, i, "return"], pt[class, i, "name"]) >> file

            a = 0
            for (j = 0; j < pt[class, i, "func"]; j++) {
                if (a) {
                    printf(", ") >> file
                }
                a++
                printf("%s %s", pt[class, i, j, "type"], pt[class, i, j, "arg"]) >> file
            }
            print ");" >> file

        }
        else {

            print "Property: " pt[class, i, "name"] > fileLog

            if (pt[class, i, "index"]) {
                ps = pt[class, i, "indextype"] " " pt[class, i, "index"] ", "
            }
            else {
                ps = ""
            }
            if (pt[class, i, "index1"]) {
                ps = ps pt[class, i, "indextype1"] " " pt[class, i, "index1"] ", "
            }

            if (pt[class, i, "set"]) {
                print "\tvirtual HRESULT STDMETHODCALLTYPE Set" pt[class, i, "name"] "(" ps pt[class, i, "atype"] " " pt[class, i, "name"] ");" >> file
                if (pt[class, i, "object"]) {
                    print "\tvirtual HRESULT STDMETHODCALLTYPE Set" pt[class, i, "name"] "ByRef(" ps pt[class, i, "atype"] " " pt[class, i, "name"] ");" >> file
                }
            }
            if (pt[class, i, "get"]) {
                print "\tvirtual HRESULT STDMETHODCALLTYPE Get" pt[class, i, "name"] "(" ps pt[class, i, "atype"] " * p" pt[class, i, "name"] ");" >> file
            }
        }
    }
}

function GenerateEventFireDecl(interface, file, i, j, a, s, ts) {

    print "In events: " interface > fileLog

    print "\n// Event fire method declarations for events " interface >> file

    for (i = 0; i < ct[interface]; i++) {

        if (pt[interface, i, "func"]) {

            print "Method: " pt[interface, i, "name"] > fileLog

            printf("\tvoid Fire%s_%s(", interface, pt[interface, i, "name"]) >> file

            a = 0
            for (j = 0; j < pt[interface, i, "func"]; j++) {
                if (a) {
                    printf(", ") >> file
                }
                a++
                ts = pt[interface, i, j, "atype"]
                if (!ts) {
                    ts = pt[interface, i, j, "type"]
                }
                printf("%s %s", ts, pt[interface, i, j, "arg"]) >> file
            }

            printf(")\n\t{\n\t\tFireEvent(%s, (BYTE *) ", pt[interface, i, "dispid"]) >> file

            a = 0
            if (pt[interface, i, "func"] > 0) {
                for (j = 0; j < pt[interface, i, "func"]; j++) {
                    s = vt[pt[interface, i, j, "type"]]
                    if (a) {
                        printf(" ") >> file
                    }
                    a++
                    printf("%s", s) >> file
                }
            }
            else {
                printf("VTS_NONE") >> file
            }

            for (j = 0; j < pt[interface, i, "func"]; j++) {
                printf(", %s", pt[interface, i, j, "arg"]) >> file
            }

            print ");\n\t}" >> file
        }
        else {
            
            print "*** AWK *** Property in event: " interface ":" pt[interface, i, "name"]  

        }
    }
    print "" >> file
}

function GenerateDISPIDs(class, file, i) {

    print "In class: " class > fileLog

    print "// DISPIDs for class" class "\n" >> file

    for (i = 0; i < ct[class]; i++) {

        print "Property: " pt[class, i, "name"] > fileLog

        if (pt[class, i, "dispid"]) {
            print "#define DISPID_" class "_" pt[class, i, "name"] "\t" pt[class, i, "dispid"] >> file
        }
    }

    print "" >> file
}

function GeneratePropMethodImpl(class, file, i, s, b, o, ps, ps) {

    print "In class: " class > fileLog

    print "// Property get/set method implementation for class " class >> file

    for (i = 0; i < ct[class]; i++) {

        if (pt[class, i, "func"] || pt[class, i, "abstract"])
            continue

        print "Property: " pt[class, i, "name"] > fileLog

        s = pt[class, i, "type"]
        if (s in et) {
            b = "ENUM"
            s = "Enum"
        }
        else {
            b = bt[s]
            s = dt[s]
        }

        if (pt[class, i, "bag"]) {
            o = "GetAttrBag()"
        }
        else if (pt[class, i, "spf"]) {
            o = "GetPF()"
        }
        else if (pt[class, i, "scf"]) {
            o = "GetCF()"
        }
        else if (pt[class, i, "indirect"]) {
            o = pt[class, i, "indirect"]
        }
        else {
            o = "this"
        }
        if (pt[class, i, "set"]) {
            print "STDMETHODIMP " class "::Set" pt[class, i, "name"] "(" pt[class, i, "atype"] " v)\n{" >> file
            if (pt[class, i, "atype"] == "VARIANT"  ) {
                print "\treturn s_propdesc" class pt[class, i, "name"] ".a.Handle" s "Property( HANDLEPROP_SET | HANDLEPROP_XERROR | (PROPTYPE_VARIANT << 16), &v, this, (CVoid *)(void *)" o ");\n}" >> file
            }
            else if (s == "Num" || s == "Enum") {
                print "\treturn s_propdesc" class pt[class, i, "name"] ".b.SetNumberProperty( v, this, (CVoid *)(void *)" o ", 0, 0);\n}" >> file
            }
            else {
                print "\treturn s_propdesc" class pt[class, i, "name"] ".b.Set" s "Property( v, this, (CVoid *)(void *)" o ");\n}" >> file
            }
            if (pt[class, i, "object"]) {
                print "STDMETHODIMP " class "::Set" pt[class, i, "name"] "ByRef(" pt[class, i, "atype"] " v)\n{" >> file
                print "\treturn Set" pt[class, i, "name"] "(v);\n}" >> file
            }
        }
        if (pt[class, i, "get"]) {
            print "STDMETHODIMP " class "::Get" pt[class, i, "name"] "(" pt[class, i, "atype"] " * p)\n{" >> file
            if (pt[class, i, "subobject"]) {
                print "\t return s_" pt[class, i, "subobject"] "_CreateSubObject ( this, (IUnknown **)p, (const PROPERTYDESC *)&s_propdesc" class pt[class, i, "name"] " );\n}" >> file
            }
            else if (pt[class, i, "atype"] == "VARIANT"  ) {
                print "\treturn s_propdesc" class pt[class, i, "name"] ".a.Handle" s "Property( HANDLEPROP_XERROR | (PROPTYPE_VARIANT << 16), p, this, (CVoid *)(void *)" o ");\n}" >> file
            }
            else if (s == "Num" || s == "Enum") {
                print "\treturn s_propdesc" class pt[class, i, "name"] ".b.GetNumberProperty( p, this, (CVoid *)(void *)" o ");\n}" >> file
            }
            else {
                print "\treturn s_propdesc" class pt[class, i, "name"] ".b.Get" s "Property( p, this, (CVoid *)(void *)" o ");\n}" >> file
            }
        }
    }

    if (ca[class, "interface"] && ca[class, "tearoff"]) {
        print "// Tear-off table for class " class >> file

        print "BEGIN_TEAROFF_TABLE(" class ", " ca[class, "interface"] ")" >> file
        print "\t(HRESULT (__stdcall CVoid::*)(void))" >> file
        print "\t\t(HRESULT (__stdcall CVoid::*)(unsigned int *))GetTypeInfoCount," >> file 
        print "\t(HRESULT (__stdcall CVoid::*)(void))" >> file
        print "\t\t(HRESULT (__stdcall CVoid::*)(unsigned int, unsigned long, ITypeInfo **))GetTypeInfo," >> file
        print "\t(HRESULT (__stdcall CVoid::*)(void))" >> file
        print "\t\t(HRESULT (__stdcall CVoid::*)(REFIID, LPOLESTR *, unsigned int, LCID, DISPID *))GetIDsOfNames," >> file
        print "\t(HRESULT (__stdcall CVoid::*)(void))" >> file
        print "\t\t(HRESULT (__stdcall CVoid::*)(DISPID, REFIID, LCID, WORD, DISPPARAMS *, VARIANT *, EXCEPINFO *, unsigned int *))Invoke," >> file

        GenerateTearOffMethods(ca[class, "interface"], file)
            
        print "END_TEAROFF_TABLE()" >> file
    }
}

function GenerateInterfaceDecl(interface, file, class, o, i, j, k, s, m, a, aa, ps) {

    if (ia[interface, "abstract"]) {
        return
    }

    print "In interface: " interface > fileLog

    print "\n#ifdef __MKTYPLIB__\n" >> file

    if (!ia[interface, "midl"]) {

        if (ia[interface, "event"]) {
            print "[\n\thidden," >> file
            print "\tuuid(" ia[interface, "guid"] ")\n]\ndispinterface " interface "\n{" >> file
            print "properties:\nmethods:\n" >> file
        }
        else {
            print "[\n\todl,\n\toleautomation,\n\tdual," >> file
            print "\tuuid(" ia[interface, "guid"] ")\n]\ninterface " interface " : " ia[interface, "super"] "\n{" >> file
        }

        for (o = 0; o < it[interface]; o++) {

            class = ic[interface, o]
            i = pi[class, im[interface, o]]

            if (pt[class, i, "func"]) {
                print o ". From class " class " func " i > fileLog

                for (j = pt[class, i] - 1; j > 0; j--) {
                    print "\t" pa[class, i, j] " : " pt[class, i, pa[class, i, j]] > fileLog
                }

                print "\tArguments: " pt[class, i, "func"] > fileLog
                for (j = 0; j < pt[class, i, "func"]; j++) {
                    for (k = pt[class, i, j] - 1; k >= 0; k--) {
                        print "\t" pa[class, i, j, k] " : " pt[class, i, j, pa[class, i, j, k]] > fileLog
                    }
                }

                if (pt[class, i, "dispid"]) {
                    printf("\t[id(%s)", pt[class, i, "dispid"]) >> file
                }
                else {
                    printf("\t[") >> file
                }

                for (j = pt[class, i] - 1; j > 0; j--) {
                    if (at[pa[class, i, j]] == 0) {
                        printf(", %s", pa[class, i, j]) >> file
                    }
                }

                printf("] %s %s(", pt[class, i, "return"], pt[class, i, "name"]) >> file

                a = 0
                for (j = 0; j < pt[class, i, "func"]; j++) {
                    if (a) {
                        printf(", ") >> file
                    }
                    a++
                    printf("[") >> file
                    aa = 0
                    for (k = pt[class, i, j] - 1; k >= 0; k--) {
                        if (at[pa[class, i, j, k]] == 0) {
                            if (aa) {
                                printf(", ") >> file
                            }
                            aa++
                            printf("%s", pa[class, i, j, k]) >> file
                        }
                        else if (pa[class, i, j, k] == "defaultvalue")
                        {
                            if (aa) {
                                printf(", ") >> file
                            }
                            printf("%s(%s)", pa[class, i, j, k], pt[class, i, j, pa[class, i, j, k]]) >> file
                            aa++
                        }
                    }
                    printf("] %s %s", pt[class, i, j, "type"], pt[class, i, j, "arg"]) >> file
                }
                print ");" >> file
            }
            else {
                print o ". From class " class " prop " i > fileLog

                for (j = pt[class, i] - 1; j > 0; j--) {
                    print "\t" pa[class, i, j] " : " pt[class, i, pa[class, i, j]] > fileLog
                }

                if (pt[class, i, "index"]) {
                    ps = "[in] " pt[class, i, "indextype"] " " pt[class, i, "index"] ", "
                }
                else {
                    ps = ""
                }
                if (pt[class, i, "index1"]) {
                    ps = ps "[in] " pt[class, i, "indextype1"] " " pt[class, i, "index1"] ", "
                }

                ts = pt[class, i, "atype"]
                if ( ts == "IDispatch*" && pt[class,i,"type"] ) {
                    ts = pt[class,i,"type"]
                }
                else if (pt[class, i, "object"]) {
                    ts = pt[class, i, "object"]
                }

                if (pt[class, i, "set"]) {
                    if (pt[class, i, "dispid"]) {
                        printf("\t[propput, id(%s)", pt[class, i, "dispid"]) >> file
                    }
                    else {
                        printf("\t[propput") >> file
                    }
        
                    for (j = pt[class, i] - 1; j > 0; j--) {
                        if (at[pa[class, i, j]] == 0) {
                            printf(", %s", pa[class, i, j]) >> file
                        }
                    }

                    printf("] HRESULT %s(%s[in] %s %s);\n", pt[class, i, "name"], ps, ts, pt[class, i, "name"]) >> file

                    if (pt[class, i, "object"]) {
                        if (pt[class, i, "dispid"]) {
                            printf("\t[propputref, id(%s)", pt[class, i, "dispid"]) >> file
                        }
                        else {
                            printf("\t[propputref") >> file
                        }

                        for (j = pt[class, i] - 1; j > 0; j--) {
                            if (at[pa[class, i, j]] == 0) {
                                printf(", %s", pa[class, i, j]) >> file
                            }
                        }

                        printf("] HRESULT %s(%s[in] %s %s);\n", pt[class, i, "name"], ps, ts, pt[class, i, "name"]) >> file
                    }
                }

                if (pt[class, i, "get"]) {
                    if (pt[class, i, "dispid"]) {
                        printf("\t[propget, id(%s)", pt[class, i, "dispid"]) >> file
                    }
                    else {
                        printf("\t[propget") >> file
                    }
        
                    for (j = pt[class, i] - 1; j > 0; j--) {
                        if (at[pa[class, i, j]] == 0) {
                            printf(", %s", pa[class, i, j]) >> file
                        }
                    }

                    printf("] HRESULT %s(%s[out, retval] %s * p%s);\n", pt[class, i, "name"], ps, ts, pt[class, i, "name"]) >> file
                }
            }
        }

        print "}" >> file
    }

    print "\n#else // __MKTYPLIB__\n" >> file

    if (!ia[interface, "typelib"] && !ia[interface, "event" ]) {
#    if (!ia[interface, "typelib"] ) {

        print "[\n\tlocal,\n\tobject,\n\tpointer_default(unique)," >> file
        print "\tuuid(" ia[interface, "guid"] ")\n]\ninterface " interface " : " ia[interface, "super"] "\n{" >> file

        for (o = 0; o < it[interface]; o++) {

            class = ic[interface, o]
            i = pi[class, im[interface, o]]

            if (pt[class, i, "func"]) {
                print o ". From class " class " func " i > fileLog

                for (j = pt[class, i] - 1; j > 0; j--) {
                    print "\t" pa[class, i, j] " : " pt[class, i, pa[class, i, j]] > fileLog
                }

                print "\tArguments: " pt[class, i, "func"] > fileLog
                for (j = 0; j < pt[class, i, "func"]; j++) {
                    for (k = pt[class, i, j] - 1; k >= 0; k--) {
                        print "\t" pa[class, i, j, k] " : " pt[class, i, j, pa[class, i, j, k]] > fileLog
                    }
                }

                printf("\t%s %s(", pt[class, i, "return"], pt[class, i, "name"]) >> file

                a = 0
                for (j = 0; j < pt[class, i, "func"]; j++) {
                    if (a) {
                        printf(", ") >> file
                    }
                    a++
                    printf("[") >> file
                    aa = 0
                    for (k = pt[class, i, j] - 1; k >= 0; k--) {
                        s = pa[class, i, j, k]
                        if (at[s] == 0 && s != "optional" && s != "retval") {
                            if (aa) {
                                printf(", ") >> file
                            }
                            aa++
                            printf("%s", s) >> file
                        }
                    }
                    printf("] %s %s", pt[class, i, j, "type"], pt[class, i, j, "arg"]) >> file
                }
                print ");" >> file
            }
            else {
                print o ". From class " class " prop " i > fileLog

                for (j = pt[class, i] - 1; j > 0; j--) {
                    print "\t" pa[class, i, j] " : " pt[class, i, pa[class, i, j]] > fileLog
                }

                if (pt[class, i, "index"]) {
                    ps = "[in] " pt[class, i, "indextype"] " " pt[class, i, "index"] ", "
                }
                else {
                    ps = ""
                }
                if (pt[class, i, "index1"]) {
                    ps = ps "[in] " pt[class, i, "indextype1"] " " pt[class, i, "index1"] ", "
                }

                if (pt[class, i, "set"]) {
                    printf("\tHRESULT Set%s(%s[in] %s %s);\n", pt[class, i, "name"], ps, pt[class, i, "atype"], pt[class, i, "name"]) >> file

                    if (pt[class, i, "object"]) {
                        printf("\tHRESULT Set%sByRef(%s[in] %s %s);\n", pt[class, i, "name"], ps, pt[class, i, "atype"], pt[class, i, "name"]) >> file
                    }
                }
                if (pt[class, i, "get"]) {
                    printf("\tHRESULT Get%s(%s[out] %s * p%s);\n", pt[class, i, "name"], ps, pt[class, i, "atype"], pt[class, i, "name"]) >> file
                }
            }
        }
    
        print "}" >> file
    }

    print "\n#endif // __MKTYPLIB__\n" >> file
}

function GenerateCoClassDecl(class, file) {

    if (ca[class, "guid"] && ca[class, "interface"]) {

        print "#ifdef __MKTYPLIB__\n" >> file

        print "[uuid(" ca[class, "guid"] ")]" >> file
        print "coclass " ca[class, "name"] "\n{" >> file
        print "[default]\t\tinterface " ca[class, "interface"] ";" >> file
        if (ca[class, "events"]) {
            print "[source, default]\tdispinterface " ca[class, "events"] ";" >> file
        }

        print "};" >> file

        print "#endif // __MKTYPLIB__\n" >> file
    }
}

function GenerateEnumDescHXX(enum, file, i) {

    print "In enum: " enum > fileLog

    if (!ea[enum, "guid"] && !ea[enum, "abstract"]) {

        print "enum " enum "\n{" >> file

        for (i = 0; i < et[enum]; i++) {
            print "\t" ea[enum, "prefix"] "_" en[enum, i] " = " ev[enum, i] "," >> file
        }

        print "};" >> file
    }

    print "EXTERN_C const ENUMDESC s_enumdesc" enum ";\n" >> file
}

function GenerateEnumDescCXX(enum, file, i) {

    print "In enum: " enum > fileLog

    print "\nEXTERN_C const ENUMDESC s_enumdesc" enum " = { " et[enum] ", {" >> file

    for (i = 0; i < et[enum]; i++) {
        print "\t{ _T(\"" es[enum, i] "\"), " ev[enum, i] "}," >> file
    }

    print "} };\n" >> file
}

function GenerateEnumDescIDL(enum, file, i) {

    print "In enum: " enum > fileLog

    if (ea[enum, "guid"] && !ea[enum, "abstract"]) {

        print "#ifdef __MKTYPLIB__\n" >> file

        print "typedef [uuid(" ea[enum, "guid"] ")] enum _" enum " {" >> file

        for (i = 0; i < et[enum]; i++) {
            print "\t[helpstring(\"" es[enum, i] "\")] " enum en[enum, i] " = " ev[enum, i] "," >> file
        }

        print "} " enum ";\n" >> file

        print "#else // __MKTYPLIB__\n" >> file

        print "typedef enum _" enum "\n{" >> file

        for (i = 0; i < et[enum]; i++) {
            print "\t" enum en[enum, i] " = " ev[enum, i] "," >> file
        }

        print "\t" enum "_Max = 2147483647L,\n} " enum ";\n" >> file

        print "#endif // __MKTYPLIB__\n" >> file
    }
}

BEGIN {

    # dt[]
    #   index is property type
    #   value is internal type (property descriptor type)
    dt["void*"]="Num"
    dt["DWORD"]="Num"
    dt["DISPID"]="Num"
    dt["SIZEL*"]="Num"
    dt["CStr"] = "String"
    dt["BSTR"] = "String"
    dt["long"] = "Num"
    dt["int"] = "Num"
    dt["short"] = "Num"
    dt["char"] = "Num"
    dt["enum"] = "Enum"
    dt["ULONG"] = "Num"
    dt["CStyleUrl"] = "StyleUrl"
    dt["CStyle"] = "Style"
    dt["CColorValue"] = "Color"
    dt["CUnitValue"] = "UnitValue"
    dt["VARIANT_BOOL"] = "Num"
    dt["OLE_XPOS_PIXELS"] = "Num"
    dt["OLE_YPOS_PIXELS"] = "Num"
    dt["OLE_XSIZE_PIXELS"] = "Num"
    dt["OLE_YSIZE_PIXELS"] = "Num"
    dt["OLE_XPOS_HIMETRIC"] = "Num"
    dt["OLE_YPOS_HIMETRIC"] = "Num"
    dt["OLE_XSIZE_HIMETRIC"] = "Num"
    dt["OLE_YSIZE_HIMETRIC"] = "Num"
    dt["OLE_XPOS_CONTAINER"] = "Float"
    dt["OLE_YPOS_CONTAINER"] = "Float"
    dt["OLE_XSIZE_CONTAINER"] = "Float"
    dt["OLE_YSIZE_CONTAINER"] = "Float"
    dt["OLE_CANCELBOOL"] = "Num"
    dt["OLE_ENABLEDEFAULTBOOL"] = "Num"
    dt["IDispatch*"] = "Object"
    dt["VARIANT"] = "Variant"
    dt["BYTE"] = "Num"
    dt["BOOL"] = "Boolean"

    # bt[]
    #   index is property type
    #   value is our abbriviation for the type

    bt["void*"]="NUM"
    bt["DWORD"]="NUM"
    bt["DISPID"]="NUM"
    bt["SIZEL*"]="NUM"
    bt["CStr"] = "STR"
    bt["BSTR"] = "BSTR"
    bt["long"] = "NUM"
    bt["int"] = "NUM"
    bt["short"] = "NUM"
    bt["char"] = "NUM"
    bt["enum"] = "ENUM"
    bt["ULONG"] = "NUM"
    bt["CStyleUrl"] = "STR"
    bt["CStyle"] = "BSTR"
    bt["CColorValue"] = "COLOR"
    bt["CUnitValue"] = "UNITVALUE"
    bt["VARIANT_BOOL"] = "NUM"
    bt["OLE_XPOS_PIXELS"] = "NUM"
    bt["OLE_YPOS_PIXELS"] = "NUM"
    bt["OLE_XSIZE_PIXELS"] = "NUM"
    bt["OLE_YSIZE_PIXELS"] = "NUM"
    bt["OLE_XPOS_HIMETRIC"] = "NUM"
    bt["OLE_YPOS_HIMETRIC"] = "NUM"
    bt["OLE_XSIZE_HIMETRIC"] = "NUM"
    bt["OLE_YSIZE_HIMETRIC"] = "NUM"
    bt["OLE_XPOS_CONTAINER"] = "FLOAT"
    bt["OLE_YPOS_CONTAINER"] = "FLOAT"
    bt["OLE_XSIZE_CONTAINER"] = "FLOAT"
    bt["OLE_YSIZE_CONTAINER"] = "FLOAT"
    bt["OLE_CANCELBOOL"] = "NUM"
    bt["OLE_ENABLEDEFAULTBOOL"] = "NUM"
    bt["IDispatch*"] = "OBJECT"
    bt["VARIANT"] = "VARIANT"
    bt["BYTE"] = "NUM"
    bt["BOOL"] = "BOOLEAN"

    # at[]
    #   index is our internal attributes 
    #   value is ordinal

    at["name"] = -1
    at["dwflags"] = -1
    at["default"] = -1
    at["object"] = -1
    at["get"] = 1
    at["set"] = 2
    at["bag"] = 3
    at["inherit"] = 4
    at["abstract"] = 5
    at["midl"] = 6
    at["typelib"] = 7
    at["defaultvalue"] = 8
    at["scf"] = 9
    at["spf"] = 10

    # vt[]
    #   index is argument type
    #   value is VTS type


    # BUBUG rgardner not sure of void* this check with istvanc
    vt["void*"]="VTS_PI4"
    vt["DWORD"]="VTS_I4"
    vt["DISPID"] = "VTS_I4"
    # BUBUG rgardner not sure of SIZEL* this check with istvanc
    vt["SIZEL*"] = "VTS_PI4" 
    vt["short"] = "VTS_I2"
    vt["long"] = "VTS_I4"
    vt["ULONG"]="VTS_I4"
    vt["ULONG*"] = "VTS_PI4"
    vt["int"] = "VTS_I4"
    vt["float"] = "VTS_R4"
    vt["double"] = "VTS_R8"
    vt["CY*"] = "VTS_CY"
    vt["DATE"] = "VTS_DATE"
    vt["BSTR"] = "VTS_BSTR"
    vt["LPCTSTR"] = "VTS_BSTR"
    vt["IDispatch*"] = "VTS_DISPATCH"
    vt["SCODE"] = "VTS_ERROR"
    vt["VARIANT_BOOL"] = "VTS_BOOL"
    vt["boolean"] = "VTS_BOOL"
    vt["ERROR"] = "VTS_ERROR"
    vt["BOOL"] = "VTS_BOOL"
    vt["VARIANT"] = "VTS_VARIANT"
    vt["IUnknown*"] = "VTS_UNKNOWN"
    vt["unsigned char"] = "VTS_UI1"
    vt["short*"] = "VTS_PI2"
    vt["long*"] = "VTS_PI4"
    vt["int*"] = "VTS_PI4"
    vt["float*"] = "VTS_PR4"
    vt["double*"] = "VTS_PR8"
    vt["CY*"] = "VTS_PCY"
    vt["DATE*"] = "VTS_PDATE"
    vt["BSTR*"] = "VTS_PBSTR"
    vt["IDispatch**"] = "VTS_PDISPATCH"
    vt["SCODE*"] = "VTS_PERROR"
    vt["VARIANT_BOOL*"] = "VTS_PBOOL"
    vt["boolean*"] = "VTS_PBOOL"
    vt["VARIANT*"] = "VTS_PVARIANT"
    vt["IUnknown**"] = "VTS_PUNKNOWN"
    vt["unsigned char *"] = "VTS_PUI1"
    vt[""] = "VTS_NONE"
    vt["OLE_COLOR"] = "VTS_COLOR"
    vt["OLE_XPOS_PIXELS"] = "VTS_XPOS_PIXELS"
    vt["OLE_YPOS_PIXELS"] = "VTS_YPOS_PIXELS"
    vt["OLE_XSIZE_PIXELS"] = "VTS_XSIZE_PIXELS"
    vt["OLE_YSIZE_PIXELS"] = "VTS_YSIZE_PIXELS"
    vt["OLE_XPOS_HIMETRIC"] = "VTS_XPOS_HIMETRIC"
    vt["OLE_YPOS_HIMETRIC"] = "VTS_YPOS_HIMETRIC"
    vt["OLE_XSIZE_HIMETRIC"] = "VTS_XSIZE_HIMETRIC"
    vt["OLE_YSIZE_HIMETRIC"] = "VTS_YSIZE_HIMETRIC"
    vt["OLE_XPOS_CONTAINER"] = "VTS_R4"
    vt["OLE_YPOS_CONTAINER"] = "VTS_R4"
    vt["OLE_XSIZE_CONTAINER"] = "VTS_R4"
    vt["OLE_YSIZE_CONTAINER"] = "VTS_R4"
    vt["OLE_TRISTATE"] = "VTS_TRISTATE"
    vt["OLE_OPTEXCLUSIVE"] = "VTS_OPTEXCLUSIVE"
    vt["OLE_COLOR*"] = "VTS_PCOLOR"
    vt["OLE_XPOS_PIXELS*"] = "VTS_PXPOS_PIXELS"
    vt["OLE_YPOS_PIXELS*"] = "VTS_PYPOS_PIXELS"
    vt["OLE_XSIZE_PIXELS*"] = "VTS_PXSIZE_PIXELS"
    vt["OLE_YSIZE_PIXELS*"] = "VTS_PYSIZE_PIXELS"
    vt["OLE_XPOS_HIMETRIC*"] = "VTS_PXPOS_HIMETRIC"
    vt["OLE_YPOS_HIMETRIC*"] = "VTS_PYPOS_HIMETRIC"
    vt["OLE_XSIZE_HIMETRIC*"] = "VTS_PXSIZE_HIMETRIC"
    vt["OLE_YSIZE_HIMETRIC*"] = "VTS_PYSIZE_HIMETRIC"
    vt["OLE_TRISTATE*"] = "VTS_PTRISTATE"
    vt["OLE_OPTEXCLUSIVE*"] = "VTS_POPTEXCLUSIVE"
    vt["IFontDispatch*"] = "VTS_FONT"
    vt["IPictureDispatch*"] = "VTS_PICTURE"
    vt["OLE_HANDLE"] = "VTS_HANDLE"
    vt["OLE_HANDLE*"] = "VTS_PHANDLE"
    vt["BYTE"] = "VTS_BOOL"

    # internal types

    vt["CStr"] = "VTS_BSTR"
    vt["CUnitValue"] = "VTS_I4"
    vt["CStyleUrl"] = "VTS_BSTR"
    vt["CStyle"] = "VTS_BSTR"
    vt["CColorValue"] = "VTS_BSTR"
}
{
#
# the input file contains the file name of the files to process and the output files
#
    fileInput = $1
    fileOutput = $2
    fileSource = $3
    fileLog = $4 "log"
#    fileOutput = fileOutput ".hdl"
    files[fileOutput] = 1
    print "Processing file: " fileInput " > " fileOutput " < " fileSource > fileLog
    line = 0
    file = ""
    cfile[fileOutput] = 0
    efile[fileOutput] = 0
    ifile[fileOutput] = 0
    lfile[fileOutput] = 0

    while(getline < fileInput > 0) {

        line++

        # strip separators out

#        gsub("[\(\),;]", " ", $0)
        s = tolower($1)

        if (s == "file") {
            file = $2
            print "From file: " file > fileLog
        }
        else if (s == "enum") {

            print "Found enum: " $2 > fileLog
            enumLast = $2

            # et[] 
            #   index is the enum type name 
            #   value is the number of enums in ev[]

            et[$2] = 0

            vt[$2] = "VTS_I4"
            vt[$2 "*"] = "VTS_PI4"

            # ea[]
            #   first index is enum name
            #   second index is enum attribute 
            #   value is the attribute value

            for (i = 3; i <= NF; i++)
            {
                split($i, t, "\:")
                s = tolower(t[1])
                ea[$2, s] = t[2]
                print "Enum attribute " s ":" ea[$2, s] > fileLog
            }

            # efile[] 
            #   index is the enum type name 
            #   value is file the enum should be defined

            if (fileSource == file) {
                efile[fileOutput, efile[fileOutput]] = $2
                efile[fileOutput]++
                print "eFile: " fileOutput " " efile[fileOutput] " " $2 > fileLog
            }

            valueLast = 0
        }
        else if (s == "eval") {

            print "Enum value: " $2 > fileLog
            o = et[enumLast]++

            # en[] 
            #   first index is the enum type name
            #   second is the enum ord
            #   value is enum name

            en[enumLast, o] = $2

            # es[] 
            #   first index is the enum type name
            #   second is the enum ord
            #   value is enum string

            es[enumLast, o] = $2

            # ev[] 
            #   first index is the enum type name
            #   second is the enum ord
            #   value is enum value

            for (i = 3; i <= NF; i++)
            {
                split($i, t, "\:")
                s = tolower(t[1])
                if (s == "string") {
                    es[enumLast, o] = t[2]
                }
                else if (s == "value") {
                    valueLast = 0 + t[2]
                }
            }

            ev[enumLast, o] = valueLast++
        }
        else if (s == "class") {

            print "Found class: " $2 > fileLog
            classLast = $2

            # ct[]
            #   index is class name
            #   value is the number of properties in pt[]

            ct[$2] = 0

# Automaticaly create vt's for any class pointers encounter 
# e.g. IDispath* and IDispatch**           
            vt[$2 "*"] = "VTS_DISPATCH"
            vt[$2 "**"] = "VTS_DISPATCH"

            # ca[]
            #   first index is class name
            #   second index is class attribute 
            #   value is the attribute value

            for (i = 3; i <= NF; i++)
            {
                if (split($i, t, "\:") == 2) {
                    s = tolower(t[1])
                    ca[$2, s] = t[2]
                    print "class attr " s ":" ca[$2, s] >> fileLog
                }
                else {
                    ca[$2, tolower($i)] = 1
                    print "class attr " $i ":" 1 >> fileLog
                }
            }

            cp[$2] = ca[$2, "super"]

            # cfile[] 
            #   index is file name
            #   second index is ord
            #   value is class name

            if (fileSource == file) {
                cfile[fileOutput, cfile[fileOutput]] = $2
                cfile[fileOutput]++
                print "cFile: " fileOutput " " cfile[fileOutput] " " $2 > fileLog
            }
            else {
                ca[$2, "imported"] = 1
            }
        }
        else if (s == "property") {

            print "Property: " $0 > fileLog
            o = ct[classLast]++

            if (classLast == interfaceLast)
            {
                print "Interface property: " $0 > fileLog

                # inside an interface declaration, set interface info
                it[interfaceLast]++
                ic[interfaceLast, o] = interfaceLast
                im[interfaceLast, o] = $2
            }

            # pt[]
            #   first index is class name
            #   second index is property ord 
            #   third index is property attribute name
            #   value is property attribute value
    
            # pa[]
            #   first index is class name
            #   second index is property ord 
            #   third index is property attribute ord
            #   value is property attribute name

            # pi[]
            #   first index is class name
            #   second index is property name
            #   value is index of property

            a = 0
            pt[classLast, o, "name"] = $2
            pt[classLast, o, "dwflags"] = "0"
            pt[classLast, o, "default"] = "0"

            pi[classLast, $2] = o

            print NF - 2 " attributes" > fileLog
            for (i = 3; i <= NF; i++) {
                if (split($i, t, "\:") == 2) {
                    s = tolower(t[1])
                    pt[classLast, o, s] = t[2]
                    pa[classLast, o, a++] = s
                    print s " : " t[2] > fileLog
                    at[s] = -1
                }
                else {
                    pt[classLast, o, $i] = at[$i]
                    pa[classLast, o, a++] = $i
                    print $i " : " at[$i] > fileLog
                }
            }
            pt[classLast, o] = a
            s = pt[classLast, o, "type"]
            if (!(s in et) && !(s in dt) && !pt[classLast, o, "abstract"] || !(s in vt)) {
                print "*** AWK *** Unknown type: " s " in file " fileInput " line " line
                print $0
                print s " " dt[s]
            }
            # BUGBUG (cthrash) we have a problem here if you really want
            # the default color to be black.
            if (s == "CColorValue" && pt[classLast, o, "default"] == 0) {
                pt[classLast, o, "default"] = -1
            }
            if (vt[s] == "VTS_DISPATCH" && !pt[classLast, o, "object"]) {
                pt[classLast, o, "object"] = s
            }
        } 
        else if (s == "method") {

            print "method: " $0 > fileLog
            o = ct[classLast]++

            if (classLast == interfaceLast)
            {
                print "Interface method: " $0 > fileLog

                # inside an interface declaration, set interface info
                it[interfaceLast]++
                ic[interfaceLast, o] = interfaceLast
                im[interfaceLast, o] = $3
            }

            # here we are reusing the property arrays but add an other
            # dimension for the arguments...

            # pt[]
            #   first index is class name
            #   second index is method ord 
            #   third index is method attribute name
            #   value is method attribute value
    
            # pa[]
            #   first index is class name
            #   second index is method ord 
            #   third index is method attribute ord
            #   value is method attribute name

            # pi[]
            #   first index is class name
            #   second index is method name
            #   value is index of method

            a = 0
            aa = 0
            pt[classLast, o, "return"] = $2
            pt[classLast, o, "name"] = $3

            pi[classLast, $3] = o

            print NF - 3 " attributes and args" > fileLog
            args = 0
            paren = 0
            for (i = 3; i <= NF; i++) {
                s = $i
                if (split(s, t, "\(") == 2) {
                    paren++
                    if (paren == 1) {
                        s = t[2]
                    }
                }
                p = paren
                if (split(s, t, "\)") == 2) {
                    paren--
                    if (paren == 0) {
                        s = t[1]
                    }
                }

                if (p) {

                    # inside parentheses, processing arguments

                    if (split(s, t, "\:") == 2) {
                        s = tolower(t[1])
                        pt[classLast, o, args, s] = t[2]
                        pa[classLast, o, args, aa++] = s
                        print "Argument " args " attribute " s " : " t[2] > fileLog
                        if (s != "defaultvalue") {
                            at[s] = -1
                        }
                        if (s == "arg") {
                            pt[classLast, o, args] = aa
                            args++
                            aa = 0
                        }
                        else if (s == "type") {
                            s = t[2]
                            if (!(s in vt)) {
                                print "*** AWK *** Unknown type: " s " in file " fileInput " line " line
                                print $0
                                print s " " vt[s]
                            }
                        }
                    }
                    else {
                        pt[classLast, o, args, s] = at[s]
                        pa[classLast, o, args, aa++] = s
                        print "Argument " args " attribute " s " : " at[s] > fileLog
                    }
                }
                else {

                    # outside parentheses, processing method attributes

                    if (split(s, t, "\:") == 2) {
                        s = tolower(t[1])
                        pt[classLast, o, s] = t[2]
                        pa[classLast, o, a++] = s
                        print s " : " t[2] > fileLog
                        at[s] = -1
                    }
                    else {
                        pt[classLast, o, s] = at[s]
                        pa[classLast, o, a++] = s
                        print s " : " at[s] > fileLog
                    }
                }
            }
            pt[classLast, o] = a
            if (args) {
                pt[classLast, o, "func"] = args
            }
            else {
                pt[classLast, o, "func"] = -1
            }
            print args " argument in " $3 > fileLog
        } 
        else if (s == "interface" || s == "event") {

            print "Found interface: " $2 > fileLog
            interfaceLast = $2
            classLast = $2

            if (s == "event") {
                ia[intefaceLast, "typelib"] = 1
            }

            if ((interfaceLast in it) && !ia[interfaceLast, "abstract"]) {
                print "*** AWK *** interface " interfaceLast " is redefined"
                print $0
            }                

            # it[]
            #   index is interface name
            #   value is the number of properties/methods in it[]

            it[$2] = 0

# Automaticaly create vt's for any class pointers encounter 
# e.g. IDispath* and IDispatch**      
            vt[$2 "*"] = "VTS_DISPATCH"
            vt[$2 "**"] = "VTS_DISPATCH"

            # ia[]
            #   first index is interface name
            #   second index is interface attribute
            #   value of the interface attribute

            ia[$2, s] = 1

            for (i = 3; i <= NF; i++)
            {
                if (split($i, t, "\:") == 2) {
                    s = tolower(t[1])
                    ia[$2, s] = t[2]
                    print "interface attr " s ":" ia[$2, s] >> fileLog
                }
                else {
                    ia[$2, tolower($i)] = 1
                    print "interface attr " $i ":" 1 >> fileLog
                }
            }

            ip[$2] = ia[$2, "super"]

            # ifile[] 
            #   index is the interface name 
            #   value is file the interface should be declared

            if (fileSource == file) {
                ifile[fileOutput, ifile[fileOutput]] = $2
                ifile[fileOutput]++
                print "iFile: " fileOutput " " ifile[fileOutput] " " $2 > fileLog
            }
        }
        else if (s == "refprop") {

            print "Interface ref property: " $0 > fileLog
            o = it[interfaceLast]++
            
            split($2, t, "\:")

            # ic[]
            #   first index is interface name
            #   second index is property/method ord
            #   value is class name the property/method is implemented
            
            ic[interfaceLast, o] = t[1]

            # im[]

            #   first index is interface name
            #   second index is property/method ord
            #   value is the name of the property/method
            
            im[interfaceLast, o] = t[2]

            print "Ref property " o " (" ic[interfaceLast, o] ":" im[interfaceLast, o] > fileLog
        }
        else if (s == "refmethod") {

            print "Interface ref method: " $0 > fileLog
            o = it[interfaceLast]++
            
            split($2, t, "\:")

            # ic[]
            #   first index is interface name
            #   second index is property/method ord
            #   value is class name the property/method is implemented
            
            ic[interfaceLast, o] = t[1]

            # im[]

            #   first index is interface name
            #   second index is property/method ord
            #   value is the name of the property/method
            
            im[interfaceLast, o] = t[2]

            print "Ref method " o " (" ic[interfaceLast, o] ":" im[interfaceLast, o] > fileLog
        }
        else if (s == "import") {

            print "Import file: " $2 " " fileSource " " fileOutput " " file > fileLog

            # lfile[] 
            #   index is import file name
            #   value is fileOutput

            if (fileSource == file) {
                lfile[fileOutput, lfile[fileOutput]] = tolower($2)
                lfile[fileOutput]++
                print "lFile: " fileOutput " " lfile[fileOutput] " " tolower($2) > fileLog
            }
        }
    }
    close(fileInput)
    if (line == 0) {
        print "*** AWK *** Couldn't open \"" fileInput "\" for input"
    }
#    CheckFile(fileOutput ".hdl")
#    CheckFile(fileOutput ".idl")
}
END {
    print "Generating results" > fileLog
    for (file in files) {

        fileRoot = file

        file = fileRoot ".idl"

        # idl part

        hfile = 0

        print "\n// " file "\n" > file

#        print "#ifdef _idl_\n" >> file

        print "#if !defined(__MKTYPLIB__) && !defined(TYPE_PHASE2)\n" >> file

        for (iimport = 0; iimport < lfile[fileRoot]; iimport++) {

            f = lfile[fileRoot, iimport]

            #
            # generate import statements for parent files
            #

            sub(".pdl", ".idl", f)
#                print "#define _idl_" >> file
            print "import \"" f "\";\n" >> file
        }

        print "#endif // !__MKTYPLIB__ && !TYPE_PHASE2" >> file

        for (ienum = 0; ienum < efile[fileRoot]; ienum++) {

            enum = efile[fileRoot, ienum]

            #
            # generate IDL enum declarations o be included 
            # in the IDL file
            #

            if (ea[enum, "guid"])
            {
                print "Generate IDL for enum:" enum > fileLog

                GenerateEnumDescIDL(enum, file)

                hfile++
            }
        }

        for (iinterface = 0; iinterface < ifile[fileRoot]; iinterface++) {

            interface = ifile[fileRoot, iinterface]

            #
            # generate IDL interface declarations to be included 
            # in the IDL file
            #

            print "Generate IDL for interface:" interface > fileLog

            GenerateInterfaceDecl(interface, file)

            hfile++
        }

        for (iclass = 0; iclass < cfile[fileRoot]; iclass++) {

            class = cfile[fileRoot, iclass]
                           
            #
            # generate coclass declaration
            #

            print "Generate ODL coclass for class:" class > fileLog

            GenerateCoClassDecl(class, file)

            print "cpp_quote(\"EXTERN_C const GUID CLSID_" ca[class, "name"] ";\")\n\n" >> file
        }

#        print "#endif _idl_\n" >> file
#        print "#undef _idl_\n" >> file

        close(file)

        file = fileRoot ".hdl"

        print "\n// " file "\n" > file

        # hxx part

        print "#ifdef _hxx_\n" >> file

        if (hfile) {
            print "#include \"" fileRoot ".h\"\n" >> file
        }

        for (ienum = 0; ienum < efile[fileRoot]; ienum++) {

            enum = efile[fileRoot, ienum]

            #
            # generate HXX enum declaration to be included
            # in the HXX file 
            #

            print "Generate HXX for enum:" enum > fileLog

            GenerateEnumDescHXX(enum, file)
        }

        for (iclass = 0; iclass < cfile[fileRoot]; iclass++) {

            class = cfile[fileRoot, iclass]

            #
            # generate DISPIDs to be included
            # in the HXX file 
            #

            print "Generate HXX for class:" class > fileLog

            GenerateDISPIDs(class, file)

            print "EXTERN_C const GUID CLSID_" ca[class, "name"] ";\n\n" >> file
        }

        for (iinterface = 0; iinterface < ifile[fileRoot]; iinterface++) {

            interface = ifile[fileRoot, iinterface]

            #
            # generate DISPIDs to be included
            # in the HXX file 
            #

            print "Generate HXX for interface:" interface > fileLog

            GenerateDISPIDs(interface, file)

            if (ia[interface, "event" ])
                print "EXTERN_C const GUID DIID_" interface ";" >> file;
        }

        print "#endif _hxx_\n" >> file
        print "#undef _hxx_\n" >> file

        # cxx part

        print "#ifdef _cxx_\n" >> file

        for (ienum = 0; ienum < efile[fileRoot]; ienum++) {

            enum = efile[fileRoot, ienum]

            #
            # generate CXX enum description structures to be included 
            # in the CXX file
            #

            print "Generate CXX for enum:" enum > fileLog

            GenerateEnumDescCXX(enum, file)
        }

        for (iclass = 0; iclass < cfile[fileRoot]; iclass++) {

            class = cfile[fileRoot, iclass]

            #
            # generate CXX property description structures and array of pointers 
            # to them to be included in the CXX file
            #

            print "Generate CXX for class:" class > fileLog

            GeneratePropertyDesc(class, file)

            if (ca[class, "super"] != "") {
                GenerateExternProp(ca[class, "super"], file)
            }

            if (ct[class]) {
                print "\nconst PROPERTYDESC * const " class "::s_appropdescs [] = {" >> file
                GenerateReferProp(class, file)
                print "NULL };\n" >> file
            }

            GeneratePropMethodImpl(class, file)
        }

        print "#endif _cxx_\n" >> file
        print "#undef _cxx_\n" >> file

        # class part

        for (iclass = 0; iclass < cfile[fileRoot]; iclass++) {

            class = cfile[fileRoot, iclass]

            #
            # generate HXX property get/set methods for the header file to be included 
            # in the HXX file into the class declaration
            #

            print "#ifdef _" class "_\n" >> file

            print "Generate HXX for class:" class > fileLog

            if (ca[class, "interface"] && ca[class, "tearoff"]) {
                print "static HRESULT (__stdcall CVoid::*s_apfn" ca[class, "interface"] "[])(void);" >> file
            }
            
            GeneratePropMethodDecl(class, file)

            if (ca[class, "events"]) {
                GenerateEventFireDecl(ca[class, "events"], file)
            }

            print "#endif _" class "_\n" >> file
            print "#undef _" class "_\n" >> file
        }

        close(file)

    }
}