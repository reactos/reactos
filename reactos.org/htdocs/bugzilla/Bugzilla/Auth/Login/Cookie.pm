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
# Contributor(s): Bradley Baetz <bbaetz@acm.org>
#                 Max Kanat-Alexander <mkanat@bugzilla.org>

package Bugzilla::Auth::Login::Cookie;
use strict;
use base qw(Bugzilla::Auth::Login);

use Bugzilla::Constants;
use Bugzilla::Util;

use List::Util qw(first);

use constant requires_persistence  => 0;
use constant requires_verification => 0;
use constant can_login => 0;

# Note that Cookie never consults the Verifier, it always assumes
# it has a valid DB account or it fails.
sub get_login_info {
    my ($self) = @_;
    my $cgi = Bugzilla->cgi;
    my $dbh = Bugzilla->dbh;

    my $ip_addr      = $cgi->remote_addr();
    my $net_addr     = get_netaddr($ip_addr);
    my $login_cookie = $cgi->cookie("Bugzilla_logincookie");
    my $user_id      = $cgi->cookie("Bugzilla_login");

    # If cookies cannot be found, this could mean that they haven't
    # been made available yet. In this case, look at Bugzilla_cookie_list.
    unless ($login_cookie) {
        my $cookie = first {$_->name eq 'Bugzilla_logincookie'}
                            @{$cgi->{'Bugzilla_cookie_list'}};
        $login_cookie = $cookie->value if $cookie;
    }
    unless ($user_id) {
        my $cookie = first {$_->name eq 'Bugzilla_login'}
                            @{$cgi->{'Bugzilla_cookie_list'}};
        $user_id = $cookie->value if $cookie;
    }

    if ($login_cookie && $user_id) {
        # Anything goes for these params - they're just strings which
        # we're going to verify against the db
        trick_taint($ip_addr);
        trick_taint($login_cookie);
        detaint_natural($user_id);

        my $query = "SELECT userid
                       FROM logincookies
                      WHERE logincookies.cookie = ?
                            AND logincookies.userid = ?
                            AND (logincookies.ipaddr = ?";

        # If we have a network block that's allowed to use this cookie,
        # as opposed to just a single IP.
        my @params = ($login_cookie, $user_id, $ip_addr);
        if (defined $net_addr) {
            trick_taint($net_addr);
            $query .= " OR logincookies.ipaddr = ?";
            push(@params, $net_addr);
        }
        $query .= ")";

        # If the cookie is valid, return a valid username.
        if ($dbh->selectrow_array($query, undef, @params)) {
            # If we logged in successfully, then update the lastused 
            # time on the login cookie
            $dbh->do("UPDATE logincookies SET lastused = NOW() 
                       WHERE cookie = ?", undef, $login_cookie);
            return { user_id => $user_id };
        }
    }

    # Either the he cookie is invalid, or we got no cookie. We don't want 
    # to ever return AUTH_LOGINFAILED, because we don't want Bugzilla to 
    # actually throw an error when it gets a bad cookie. It should just 
    # look like there was no cookie to begin with.
    return { failure => AUTH_NODATA };
}

1;
