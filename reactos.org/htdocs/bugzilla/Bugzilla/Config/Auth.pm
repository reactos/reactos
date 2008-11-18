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

package Bugzilla::Config::Auth;

use strict;

use Bugzilla::Config::Common;

$Bugzilla::Config::Auth::sortkey = "02";

sub get_param_list {
  my $class = shift;
  my @param_list = (
  {
   name => 'auth_env_id',
   type    => 't',
   default => '',
  },

  {
   name    => 'auth_env_email',
   type    => 't',
   default => '',
  },

  {
   name    => 'auth_env_realname',
   type    => 't',
   default => '',
  },

  # XXX in the future:
  #
  # user_verify_class and user_info_class should have choices gathered from
  # whatever sits in their respective directories
  #
  # rather than comma-separated lists, these two should eventually become
  # arrays, but that requires alterations to editparams first

  {
   name => 'user_info_class',
   type => 's',
   choices => [ 'CGI', 'Env', 'Env,CGI', 'ROSCMS' ],
   default => 'CGI',
   checker => \&check_multi
  },

  {
   name => 'user_verify_class',
   type => 's',
   choices => [ 'DB', 'LDAP', 'DB,LDAP', 'LDAP,DB', 'ROSCMS' ],
   default => 'DB',
   checker => \&check_user_verify_class
  },

  {
   name => 'rememberlogin',
   type => 's',
   choices => ['on', 'defaulton', 'defaultoff', 'off'],
   default => 'on',
   checker => \&check_multi
  },

  {
   name => 'loginnetmask',
   type => 't',
   default => '0',
   checker => \&check_netmask
  },

  {
   name => 'requirelogin',
   type => 'b',
   default => '0'
  },

  {
   name => 'emailregexp',
   type => 't',
   default => q:^[\\w\\.\\+\\-=]+@[\\w\\.\\-]+\\.[\\w\\-]+$:,
   checker => \&check_regexp
  },

  {
   name => 'emailregexpdesc',
   type => 'l',
   default => 'A legal address must contain exactly one \'@\', and at least ' .
              'one \'.\' after the @.'
  },

  {
   name => 'emailsuffix',
   type => 't',
   default => ''
  },

  {
   name => 'createemailregexp',
   type => 't',
   default => q:.*:,
   checker => \&check_regexp
  } );
  return @param_list;
}

1;
