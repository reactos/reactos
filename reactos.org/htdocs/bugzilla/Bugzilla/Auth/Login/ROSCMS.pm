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
#                 Max Kanat-Alexander <mkanat@bugzilla.org>

# Auth::Login class for RosCMS
# based on the former class for Bugzilla 2.x by Gé van Geldorp and Michael Wirth and the Auth::Login::CGI class
# improved and made compatible with Bugzilla 3.x and Deskzilla by Colin Finck (2007-08-06)

package Bugzilla::Auth::Login::ROSCMS;
use strict;
use base qw(Bugzilla::Auth::Login);
use constant can_logout => 0;						# The Bugzilla Logout feature has to be disabled, so the user can only log out with the RosCMS Logout feature

use URI;
use URI::Escape;

use Bugzilla::Constants;
use Bugzilla::WebService::Constants;
use Bugzilla::User;
use Bugzilla::Util;
use Bugzilla::Error;

my $session_cookie_name = "roscmsusrkey";
my $roscms_db_name      = "roscms";
my $roscms_login_page   = "/roscms/?page=login&target=";

sub get_login_info {
	my ($self) = @_;
	my $cgi = Bugzilla->cgi;
	
	# Check if we have username and password given (usual CGI method for apps like Deskzilla)
	my $username = trim($cgi->param("Bugzilla_login"));
	my $password = $cgi->param("Bugzilla_password");
	$cgi->delete('Bugzilla_login', 'Bugzilla_password');
	
	if(defined $username && defined $password) {
		return { username => $username, password => $password };
	}
	
	# No, then check for the RosCMS Login cookie
	my $dbh = Bugzilla->dbh;
	my $user_id;
	my $roscms_user_id;
	my $session_id = $cgi->cookie($session_cookie_name);
	
	if ( defined $session_id ) {
		my $session_id_clean = $session_id;
		trick_taint($session_id_clean);
		my $remote_addr_clean;
		if ($ENV{'REMOTE_ADDR'} =~ m/^(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})/) {
			$remote_addr_clean = $1;
		} else {
			$remote_addr_clean = 'invalid';
		}
		my $browser_agent_clean = $ENV{'HTTP_USER_AGENT'};
		trick_taint($browser_agent_clean);
		my $query = "SELECT m.map_subsys_userid, m.map_roscms_userid " .
				"  FROM $roscms_db_name.user_sessions s, " .
				"       $roscms_db_name.users u, " .
				"       $roscms_db_name.subsys_mappings m " .
				" WHERE s.usersession_id = ? " .
				"   AND (s.usersession_expires IS NULL OR " .
				"        NOW() <= s.usersession_expires) " .
				"   AND u.user_id = s.usersession_user_id " .
				"   AND (u.user_setting_ipaddress = 'false' OR " .
				"        s.usersession_ipaddress = ?) " .
				"   AND (u.user_setting_browseragent = 'false' OR " .
				"        s.usersession_browseragent = ?) " .
				"   AND m.map_roscms_userid = s.usersession_user_id " .
				"   AND m.map_subsys_name = 'bugzilla'";
		my @params = ($session_id_clean, $remote_addr_clean, $browser_agent_clean);
		($user_id, $roscms_user_id) = $dbh->selectrow_array($query, undef, @params);
		
		if ($user_id) {
			# Update time of last session use
			$query = "UPDATE $roscms_db_name.user_sessions " .
					"   SET usersession_expires = DATE_ADD(NOW(), INTERVAL 30 MINUTE) " .
					" WHERE usersession_id = ? " .
					"   AND usersession_expires IS NOT NULL";
			@params = ($session_id_clean);
			$dbh->do($query, undef, @params);
			
			# Get the user name and the MD5 password from the database
			# We don't check the password explicitly here as we only deal with the session cookie.
			# To show the Verify module that it should trust us, we pass the MD5 password hash to it. This should be secure as long as we're the only one who knows this MD5 hash.
			my $username = user_id_to_login($user_id);
			(my $md5_password) = $dbh->selectrow_array("SELECT user_roscms_password FROM $roscms_db_name.users WHERE user_id = ?", undef, $roscms_user_id);
			
			# We need to set a parameter for the Auth::Persist::ROSCMS module
			$cgi->param('ROSCMS_login', 1);
			
			return { username => $username, md5_password => $md5_password };
		}
	}
	
	return { failure => AUTH_NODATA };
}

sub fail_nodata {
	my ($self) = @_;
	my $cgi = Bugzilla->cgi;

	# Throw up the login page
	my $this_uri = uri_escape($cgi->url(-absolute=>1, -path_info=>1, -query=>1));
	print $cgi->redirect($roscms_login_page .  $this_uri);
	exit;
}

1;
