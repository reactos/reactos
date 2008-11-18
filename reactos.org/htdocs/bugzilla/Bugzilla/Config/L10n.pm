# -*- Mode: perl; indent-tabs-mode: nil -*-
#
# The contents of this file are subject to the Mozilla Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is the Bugzilla Bug Tracking System.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): Terry Weissman <terry@mozilla.org>
#                 Dawn Endico <endico@mozilla.org>
#                 Dan Mosedale <dmose@mozilla.org>
#                 Joe Robins <jmrobins@tgix.com>
#                 Jacob Steenhagen <jake@bugzilla.org>
#                 J. Paul Reed <preed@sigkill.com>
#                 Bradley Baetz <bbaetz@student.usyd.edu.au>
#                 Joseph Heenan <joseph@heenan.me.uk>
#                 Erik Stambaugh <erik@dasbistro.com>
#                 Frédéric Buclin <LpSolit@gmail.com>
#

package Bugzilla::Config::L10n;

use strict;

use File::Spec; # for find_languages

use Bugzilla::Constants;
use Bugzilla::Config::Common;

$Bugzilla::Config::L10n::sortkey = "08";

sub get_param_list {
  my $class = shift;
  my @param_list = (
  {
   name => 'languages' ,
   extra_desc => { available_languages => find_languages() },
   type => 't' ,
   default => 'en' ,
   checker => \&check_languages
  },

  {
   name => 'defaultlanguage',
   type => 't' ,
   default => 'en' ,
   checker => \&check_languages
  } );
  return @param_list;
}

sub find_languages {
    my @languages = ();
    opendir(DIR, bz_locations()->{'templatedir'})
      || return "Can't open 'template' directory: $!";
    foreach my $dir (readdir(DIR)) {
        next unless $dir =~ /^([a-z-]+)$/i;
        my $lang = $1;
        next if($lang =~ /^CVS$/i);
        my $deft_path = File::Spec->catdir('template', $lang, 'default');
        my $cust_path = File::Spec->catdir('template', $lang, 'custom');
        push(@languages, $lang) if(-d $deft_path or -d $cust_path);
    }
    closedir DIR;
    return join(', ', @languages);
}

1;
