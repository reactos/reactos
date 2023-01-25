import ldns
import sys

if len(sys.argv) <= 1:
    print("Usage: %s zone_file" % sys.argv[0])
    sys.exit()

inp = open(sys.argv[1],"r");
# variables that preserve the parsers state
my_ttl = 3600;
my_origin = None
my_prev = None
# additional state variables
last_pos = 0
line_nr = 0

while True:
    ret = ldns.ldns_rr_new_frm_fp_l_(inp, my_ttl, my_origin, my_prev)
    s, rr, line_inc, new_ttl, new_origin, new_prev = ret  # unpack the result
    line_nr += line_inc # increase number of parsed lines
    my_prev = new_prev  # update ref to previous owner

    if s == ldns.LDNS_STATUS_SYNTAX_TTL:
        my_ttl = new_ttl  # update default TTL
        print("$TTL:", my_ttl)
    elif s == ldns.LDNS_STATUS_SYNTAX_ORIGIN:
        my_origin = new_origin  # update reference to origin
        print("$ORIGIN:", my_origin)
    elif s == ldns.LDNS_STATUS_SYNTAX_EMPTY:
        if last_pos == inp.tell():
            break  # no advance since last read - EOF
        last_pos = inp.tell()
    elif s != ldns.LDNS_STATUS_OK:
        print("! parse error in line", line_nr)
    else:
        # we are sure to have LDNS_STATUS_OK
        print(rr)

inp.close()
print("--------------------")
print("Read %d lines" % line_nr)
    

