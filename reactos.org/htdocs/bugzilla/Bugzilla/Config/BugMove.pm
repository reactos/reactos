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

package Bugzilla::Config::BugMove;

use strict;

use Bugzilla::Config::Common;

$Bugzilla::Config::BugMove::sortkey = "05";

sub get_param_list {
  my $class = shift;
  my @param_list = (
  {
   name => 'move-enabled',
   type => 'b',
   default => 0
  },

  {
   name => 'move-button-text',
   type => 't',
   default => 'Move To Bugscape'
  },

  {
   name => 'move-to-url',
   type => 't',
   default => ''
  },

  {
   name => 'move-to-address',
   type => 't',
   default => 'bugzilla-import'
  },

  {
   name => 'moved-from-address',
   type => 't',
   default => 'bugzilla-admin'
  },

  {
   name => 'movers',
   type => 't',
   default => ''
  },

  {
   name => 'moved-default-product',
   type => 't',
   default => ''
  },

  {
   name => 'moved-default-component',
   type => 't',
   default => ''
  } );
  return @param_list;
}

1;
