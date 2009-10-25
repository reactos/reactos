# *   Copyright (C) 1997-2005, International Business Machines
# *   Corporation and others.  All Rights Reserved.
# A list of txt's to build
# Note: 
#
#   If you are thinking of modifying this file, READ THIS. 
#
# Instead of changing this file [unless you want to check it back in],
# you should consider creating a 'reslocal.mk' file in this same directory.
# Then, you can have your local changes remain even if you upgrade or
# reconfigure ICU.
#
# Example 'rbnflocal.mk' files:
#
#  * To add an additional locale to the list: 
#    _____________________________________________________
#    |  RBNF_SOURCE_LOCAL =   myLocale.txt ...
#
#  * To REPLACE the default list and only build with a few
#     locale:
#    _____________________________________________________
#    |  RBNF_SOURCE = ar.txt ar_AE.txt en.txt de.txt zh.txt
#
#


# This is the list of locales that are built, but not considered installed in ICU.
# These are usually aliased locales or the root locale.
RBNF_ALIAS_SOURCE = 


# Please try to keep this list in alphabetical order
RBNF_SOURCE = \
da.txt \
de.txt \
en.txt en_GB.txt\
eo.txt \
es.txt \
fa.txt fa_AF.txt \
fr.txt fr_BE.txt fr_CH.txt \
ga.txt \
he.txt \
it.txt \
ja.txt \
mt.txt \
nl.txt \
pl.txt \
pt.txt \
ru.txt \
sv.txt \
th.txt \
uk.txt 

#These are not in use yet
# el.txt \
