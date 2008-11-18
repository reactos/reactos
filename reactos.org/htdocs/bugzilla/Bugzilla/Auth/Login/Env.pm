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
# Contributor(s): Erik Stambaugh <erik@dasbistro.com>
#                 Max Kanat-Alexander <mkanat@bugzilla.org>

package Bugzilla::Auth::Login::Env;
use strict;
use base qw(Bugzilla::Auth::Login);

use Bugzilla::Constants;
use Bugzilla::Error;

use constant can_logout => 0;
use constant can_login  => 0;
use constant requires_verification => 0;

sub get_login_info {
    my ($self) = @_;
    my $dbh = Bugzilla->dbh;

    my $env_id       = $ENV{Bugzilla->params->{"auth_env_id"}} || '';
    my $env_email    = $ENV{Bugzilla->params->{"auth_env_email"}} || '';
    my $env_realname = $ENV{Bugzilla->params->{"auth_env_realname"}} || '';

    return { failure => AUTH_NODATA } if !$env_email;

    return { username => $env_email, extern_id => $env_id, 
             realname => $env_realname };
}

sub fail_nodata {
    ThrowCodeError('env_no_email');
}

1;
