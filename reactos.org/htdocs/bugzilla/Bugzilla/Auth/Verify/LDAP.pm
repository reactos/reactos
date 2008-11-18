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

package Bugzilla::Auth::Verify::LDAP;
use strict;
use base qw(Bugzilla::Auth::Verify);
use fields qw(
    ldap
);

use Bugzilla::Constants;
use Bugzilla::Error;

use Net::LDAP;

use constant admin_can_create_account => 0;
use constant user_can_create_account  => 0;

sub check_credentials {
    my ($self, $params) = @_;
    my $dbh = Bugzilla->dbh;

    # We need to bind anonymously to the LDAP server.  This is
    # because we need to get the Distinguished Name of the user trying
    # to log in.  Some servers (such as iPlanet) allow you to have unique
    # uids spread out over a subtree of an area (such as "People"), so
    # just appending the Base DN to the uid isn't sufficient to get the
    # user's DN.  For servers which don't work this way, there will still
    # be no harm done.
    $self->_bind_ldap_anonymously();

    # Now, we verify that the user exists, and get a LDAP Distinguished
    # Name for the user.
    my $username = $params->{username};
    my $dn_result = $self->ldap->search(_bz_search_params($username),
                                        attrs  => ['dn']);
    return { failure => AUTH_ERROR, error => "ldap_search_error",
             details => {errstr => $dn_result->error, username => $username}
    } if $dn_result->code;

    return { failure => AUTH_NO_SUCH_USER } if !$dn_result->count;

    my $dn = $dn_result->shift_entry->dn;

    # Check the password.   
    my $pw_result = $self->ldap->bind($dn, password => $params->{password});
    return { failure => AUTH_LOGINFAILED } if $pw_result->code;

    # And now we fill in the user's details.
    my $detail_result = $self->ldap->search(_bz_search_params($username));
    return { failure => AUTH_ERROR, error => "ldap_search_error",
             details => {errstr => $detail_result->error, username => $username}
    } if $detail_result->code;

    my $user_entry = $detail_result->shift_entry;

    my $mail_attr = Bugzilla->params->{"LDAPmailattribute"};
    if ($mail_attr) {
        if (!$user_entry->exists($mail_attr)) {
            return { failure => AUTH_ERROR,
                     error   => "ldap_cannot_retreive_attr",
                     details => {attr => $mail_attr} };
        }

        $params->{bz_username} = $user_entry->get_value($mail_attr);
    } else {
        $params->{bz_username} = $username;
    }

    $params->{realname}  ||= $user_entry->get_value("displayName");
    $params->{realname}  ||= $user_entry->get_value("cn");

    return $params;
}

sub _bz_search_params {
    my ($username) = @_;
    return (base   => Bugzilla->params->{"LDAPBaseDN"},
            scope  => "sub",
            filter => '(&(' . Bugzilla->params->{"LDAPuidattribute"} 
                      . "=$username)"
                      . Bugzilla->params->{"LDAPfilter"} . ')');
}

sub _bind_ldap_anonymously {
    my ($self) = @_;
    my $bind_result;
    if (Bugzilla->params->{"LDAPbinddn"}) {
        my ($LDAPbinddn,$LDAPbindpass) = 
            split(":",Bugzilla->params->{"LDAPbinddn"});
        $bind_result = 
            $self->ldap->bind($LDAPbinddn, password => $LDAPbindpass);
    }
    else {
        $bind_result = $self->ldap->bind();
    }
    ThrowCodeError("ldap_bind_failed", {errstr => $bind_result->error})
        if $bind_result->code;
}

# We can't just do this in new(), because we're not allowed to throw any
# error from anywhere under Bugzilla::Auth::new -- otherwise we
# could create a situation where the admin couldn't get to editparams
# to fix his mistake. (Because Bugzilla->login always calls 
# Bugzilla::Auth->new, and almost every page calls Bugzilla->login.)
sub ldap {
    my ($self) = @_;
    return $self->{ldap} if $self->{ldap};

    my $server = Bugzilla->params->{"LDAPserver"};
    ThrowCodeError("ldap_server_not_defined") unless $server;

    $self->{ldap} = new Net::LDAP($server)
        || ThrowCodeError("ldap_connect_failed", { server => $server });

    # try to start TLS if needed
    if (Bugzilla->params->{"LDAPstarttls"}) {
        my $mesg = $self->{ldap}->start_tls();
        ThrowCodeError("ldap_start_tls_failed", { error => $mesg->error() })
            if $mesg->code();
    }

    return $self->{ldap};
}

1;
