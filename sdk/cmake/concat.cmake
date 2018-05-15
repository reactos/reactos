file(READ ${SRC1} S1)
file(READ ${SRC2} S2)
file(WRITE ${DST} "${S1}${S2}")
