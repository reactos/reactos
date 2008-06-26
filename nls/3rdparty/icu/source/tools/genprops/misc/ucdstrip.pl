#!/usr/lib/perl -p
# Copyright (c) 2001-2003 International Business Machines
# Corporation and others. All Rights Reserved.
# Simple tool for Unicode Character Database files with semicolon-delimited fields.
# Removes comments behind data lines but not in others.
# The Perl option -p above runs a while(<>) loop and prints the expression output.
s/^([0-9a-fA-F]+.+?) *#.*/\1/;
