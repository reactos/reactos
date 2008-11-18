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

package Bugzilla::Config::BugChange;

use strict;

use Bugzilla::Config::Common;

$Bugzilla::Config::BugChange::sortkey = "03";

sub get_param_list {
  my $class = shift;
  my @param_list = (
  {
   name => 'letsubmitterchoosepriority',
   type => 'b',
   default => 1
  },

  {
   name => 'letsubmitterchoosemilestone',
   type => 'b',
   default => 1
  },

  {
   name => 'musthavemilestoneonaccept',
   type => 'b',
   default => 0
  },

  {
   name => 'commentoncreate',
   type => 'b',
   default => 0
  },

  {
   name => 'commentonaccept',
   type => 'b',
   default => 0
  },

  {
   name => 'commentonclearresolution',
   type => 'b',
   default => 0
  },

  {
   name => 'commentonconfirm',
   type => 'b',
   default => 0
  },

  {
   name => 'commentonresolve',
   type => 'b',
   default => 0
  },

  {
   name => 'commentonreassign',
   type => 'b',
   default => 0
  },

  {
   name => 'commentonreassignbycomponent',
   type => 'b',
   default => 0
  },

  {
   name => 'commentonreopen',
   type => 'b',
   default => 0
  },

  {
   name => 'commentonverify',
   type => 'b',
   default => 0
  },

  {
   name => 'commentonclose',
   type => 'b',
   default => 0
  },

  {
   name => 'commentonduplicate',
   type => 'b',
   default => 0
  },

  {
   name    => 'noresolveonopenblockers',
   type    => 'b',
   default => 0,
  } );
  return @param_list;
}

1;
