import ldns
import sys

if len(sys.argv) <= 1:
    print("Usage: %s zone_file" % sys.argv[0])
    sys.exit()

inp = open(sys.argv[1],"r");
for rr in ldns.ldns_rr_iter_frm_fp_l(inp):
  print(rr)

inp.close()
