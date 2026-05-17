## Copyright (C) 2002, 2005-2015, 2018-2021 Free Software Foundation,
## Inc.

## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <https://www.gnu.org/licenses/>.

dist_pkgdata_DATA =                             \
  data/README.md                                \
  data/bison-default.css

skeletonsdir = $(pkgdatadir)/skeletons
dist_skeletons_DATA =                           \
  data/skeletons/bison.m4                       \
  data/skeletons/c++-skel.m4                    \
  data/skeletons/c++.m4                         \
  data/skeletons/c-like.m4                      \
  data/skeletons/c-skel.m4                      \
  data/skeletons/c.m4                           \
  data/skeletons/glr.c                          \
  data/skeletons/glr.cc                         \
  data/skeletons/glr2.cc                        \
  data/skeletons/java-skel.m4                   \
  data/skeletons/java.m4                        \
  data/skeletons/lalr1.cc                       \
  data/skeletons/lalr1.java                     \
  data/skeletons/location.cc                    \
  data/skeletons/stack.hh                       \
  data/skeletons/traceon.m4                     \
  data/skeletons/variant.hh                     \
  data/skeletons/yacc.c

# Experimental support for the D language.
dist_skeletons_DATA +=                          \
  data/skeletons/d-skel.m4                      \
  data/skeletons/d.m4                           \
  data/skeletons/lalr1.d

m4sugardir = $(pkgdatadir)/m4sugar
dist_m4sugar_DATA =                             \
  data/m4sugar/foreach.m4                       \
  data/m4sugar/m4sugar.m4

xsltdir = $(pkgdatadir)/xslt
dist_xslt_DATA =                                \
  data/xslt/bison.xsl                           \
  data/xslt/xml2dot.xsl                         \
  data/xslt/xml2text.xsl                        \
  data/xslt/xml2xhtml.xsl
