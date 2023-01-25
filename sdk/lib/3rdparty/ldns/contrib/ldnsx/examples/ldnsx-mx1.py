import ldnsx

resolver = ldnsx.resolver()

pkt = resolver.query("nic.cz", "MX")

if (pkt):
    mx = pkt.answer()
    if (mx):
       mx.sort()
       print mx
