#!/usr/bin/python

import ldns

buf = ldns.ldns_buffer(1024)
buf.printf("Test buffer")
print(buf)

