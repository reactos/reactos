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
#                 Dan Mosedale <dmose@mozilla.org>
#                 Joe Robins <jmrobins@tgix.com>
#                 Dave Miller <justdave@syndicomm.com>
#                 Christopher Aillon <christopher@aillon.com>
#                 Gervase Markham <gerv@gerv.net>
#                 Christian Reis <kiko@async.com.br>
#                 Bradley Baetz <bbaetz@acm.org>
#                 Erik Stambaugh <erik@dasbistro.com>

# Auth::Verify class for RosCMS
# developed by Colin Finck based on the Auth::Verify::DB class (2007-08-06)

package Bugzilla::Auth::Verify::ROSCMS;
use strict;
use base qw(Bugzilla::Auth::Verify);
use constant can_change_password => 0;		# Password has to be changed at myReactOS

use Bugzilla::Constants;
use Bugzilla::Token;
use Bugzilla::Util;
use Bugzilla::User;

use Digest::MD5 qw(md5_hex);

my $roscms_db_name = "roscms";

sub check_credentials {
	my ($self, $login_data) = @_;
	my $dbh = Bugzilla->dbh;
	
	my $username = $login_data->{username};
	my $user_id  = login_to_id($username);
	
	return { failure => AUTH_NO_SUCH_USER } unless $user_id;
	
	$login_data->{bz_username} = $username;
	my $md5_password = $login_data->{md5_password};
	
	if( !defined $md5_password )
	{
		my $password = $login_data->{password};
		$md5_password = md5_hex($password);
	}
	
	my $query = "SELECT u.user_roscms_password " .
			"FROM $roscms_db_name.users u, " .
			"   $roscms_db_name.subsys_mappings m " .
			"WHERE u.user_id = m.map_roscms_userid " .
			"   AND m.map_subsys_name = 'bugzilla' " .
			"   AND m.map_subsys_userid = ?";
	(my $valid_md5_password) = $dbh->selectrow_array($query, undef, $user_id);
	
	return { failure => AUTH_LOGINFAILED }
		if $md5_password ne $valid_md5_password;
	
	# The user's credentials are okay, so delete any outstanding
	# password tokens they may have generated.
	Bugzilla::Token::DeletePasswordTokens($user_id, "user_logged_in");
	
	return $login_data;
}

1;
