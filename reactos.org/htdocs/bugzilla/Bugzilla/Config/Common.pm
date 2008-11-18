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

package Bugzilla::Config::Common;

use strict;

use Socket;
use Time::Zone;

use Bugzilla::Util;
use Bugzilla::Constants;
use Bugzilla::Field;
use Bugzilla::Group;

use base qw(Exporter);
@Bugzilla::Config::Common::EXPORT =
    qw(check_multi check_numeric check_regexp check_url check_group
       check_sslbase check_priority check_severity check_platform
       check_opsys check_shadowdb check_urlbase check_webdotbase
       check_netmask check_user_verify_class check_image_converter
       check_languages check_mail_delivery_method check_notification
       check_timezone check_utf8
);

# Checking functions for the various values

sub check_multi {
    my ($value, $param) = (@_);

    if ($param->{'type'} eq "s") {
        unless (scalar(grep {$_ eq $value} (@{$param->{'choices'}}))) {
            return "Invalid choice '$value' for single-select list param '$param->{'name'}'";
        }

        return "";
    }
    elsif ($param->{'type'} eq "m") {
        foreach my $chkParam (@$value) {
            unless (scalar(grep {$_ eq $chkParam} (@{$param->{'choices'}}))) {
                return "Invalid choice '$chkParam' for multi-select list param '$param->{'name'}'";
            }
        }

        return "";
    }
    else {
        return "Invalid param type '$param->{'type'}' for check_multi(); " .
          "contact your Bugzilla administrator";
    }
}

sub check_numeric {
    my ($value) = (@_);
    if ($value !~ /^[0-9]+$/) {
        return "must be a numeric value";
    }
    return "";
}

sub check_regexp {
    my ($value) = (@_);
    eval { qr/$value/ };
    return $@;
}

sub check_sslbase {
    my $url = shift;
    if ($url ne '') {
        if ($url !~ m#^https://([^/]+).*/$#) {
            return "must be a legal URL, that starts with https and ends with a slash.";
        }
        my $host = $1;
        if ($host =~ /:\d+$/) {
            return "must not contain a port.";
        }
        local *SOCK;
        my $proto = getprotobyname('tcp');
        socket(SOCK, PF_INET, SOCK_STREAM, $proto);
        my $sin = sockaddr_in(443, inet_aton($host));
        if (!connect(SOCK, $sin)) {
            return "Failed to connect to " . html_quote($host) . 
                   ":443, unable to enable SSL.";
        }
    }
    return "";
}

sub check_utf8 {
    my $utf8 = shift;
    # You cannot turn off the UTF-8 parameter if you've already converted
    # your tables to utf-8.
    my $dbh = Bugzilla->dbh;
    if ($dbh->isa('Bugzilla::DB::Mysql') && $dbh->bz_db_is_utf8 && !$utf8) {
        return "You cannot disable UTF-8 support, because your MySQL database"
               . " is encoded in UTF-8";
    }
    return "";
}

sub check_priority {
    my ($value) = (@_);
    my $legal_priorities = get_legal_field_values('priority');
    if (lsearch($legal_priorities, $value) < 0) {
        return "Must be a legal priority value: one of " .
            join(", ", @$legal_priorities);
    }
    return "";
}

sub check_severity {
    my ($value) = (@_);
    my $legal_severities = get_legal_field_values('bug_severity');
    if (lsearch($legal_severities, $value) < 0) {
        return "Must be a legal severity value: one of " .
            join(", ", @$legal_severities);
    }
    return "";
}

sub check_platform {
    my ($value) = (@_);
    my $legal_platforms = get_legal_field_values('rep_platform');
    if (lsearch(['', @$legal_platforms], $value) < 0) {
        return "Must be empty or a legal platform value: one of " .
            join(", ", @$legal_platforms);
    }
    return "";
}

sub check_opsys {
    my ($value) = (@_);
    my $legal_OS = get_legal_field_values('op_sys');
    if (lsearch(['', @$legal_OS], $value) < 0) {
        return "Must be empty or a legal operating system value: one of " .
            join(", ", @$legal_OS);
    }
    return "";
}

sub check_group {
    my $group_name = shift;
    return "" unless $group_name;
    my $group = new Bugzilla::Group({'name' => $group_name});
    unless (defined $group) {
        return "Must be an existing group name";
    }
    return "";
}

sub check_shadowdb {
    my ($value) = (@_);
    $value = trim($value);
    if ($value eq "") {
        return "";
    }

    if (!Bugzilla->params->{'shadowdbhost'}) {
        return "You need to specify a host when using a shadow database";
    }

    # Can't test existence of this because ConnectToDatabase uses the param,
    # but we can't set this before testing....
    # This can really only be fixed after we can use the DBI more openly
    return "";
}

sub check_urlbase {
    my ($url) = (@_);
    if ($url && $url !~ m:^http.*/$:) {
        return "must be a legal URL, that starts with http and ends with a slash.";
    }
    return "";
}

sub check_url {
    my ($url) = (@_);
    return '' if $url eq ''; # Allow empty URLs
    if ($url !~ m:/$:) {
        return 'must be a legal URL, absolute or relative, ending with a slash.';
    }
    return '';
}

sub check_webdotbase {
    my ($value) = (@_);
    $value = trim($value);
    if ($value eq "") {
        return "";
    }
    if($value !~ /^https?:/) {
        if(! -x $value) {
            return "The file path \"$value\" is not a valid executable.  Please specify the complete file path to 'dot' if you intend to generate graphs locally.";
        }
        # Check .htaccess allows access to generated images
        my $webdotdir = bz_locations()->{'webdotdir'};
        if(-e "$webdotdir/.htaccess") {
            open HTACCESS, "$webdotdir/.htaccess";
            if(! grep(/ \\\.png\$/,<HTACCESS>)) {
                return "Dependency graph images are not accessible.\nAssuming that you have not modified the file, delete $webdotdir/.htaccess and re-run checksetup.pl to rectify.\n";
            }
            close HTACCESS;
        }
    }
    return "";
}

sub check_netmask {
    my ($mask) = @_;
    my $res = check_numeric($mask);
    return $res if $res;
    if ($mask < 0 || $mask > 32) {
        return "an IPv4 netmask must be between 0 and 32 bits";
    }
    # Note that if we changed the netmask from anything apart from 32, then
    # existing logincookies which aren't for a single IP won't work
    # any more. We can't know which ones they are, though, so they'll just
    # take space until they're periodically cleared, later.

    return "";
}

sub check_user_verify_class {
    # doeditparams traverses the list of params, and for each one it checks,
    # then updates. This means that if one param checker wants to look at 
    # other params, it must be below that other one. So you can't have two 
    # params mutually dependent on each other.
    # This means that if someone clears the LDAP config params after setting
    # the login method as LDAP, we won't notice, but all logins will fail.
    # So don't do that.

    my ($list, $entry) = @_;
    for my $class (split /,\s*/, $list) {
        my $res = check_multi($class, $entry);
        return $res if $res;
        if ($class eq 'DB') {
            # No params
        } elsif ($class eq 'LDAP') {
            eval "require Net::LDAP";
            return "Error requiring Net::LDAP: '$@'" if $@;
            return "LDAP servername is missing" unless Bugzilla->params->{"LDAPserver"};
            return "LDAPBaseDN is empty" unless Bugzilla->params->{"LDAPBaseDN"};
        } elsif ($class eq 'ROSCMS') {
        	# No params
        } else {
                return "Unknown user_verify_class '$class' in check_user_verify_class";
        }
    }
    return "";
}

sub check_image_converter {
    my ($value, $hash) = @_;
    if ($value == 1){
       eval "require Image::Magick";
       return "Error requiring Image::Magick: '$@'" if $@;
    } 
    return "";
}

sub check_languages {
    my ($lang, $param) = @_;
    my @languages = split(/[,\s]+/, trim($lang));
    if(!scalar(@languages)) {
       return "You need to specify a language tag."
    }
    if (scalar(@languages) > 1 && $param && $param->{'name'} eq 'defaultlanguage') {
        return "You can only specify one language tag";
    }
    my $templatedir = bz_locations()->{'templatedir'};
    my %lang_seen;
    my @validated_languages;
    foreach my $language (@languages) {
       if(   ! -d "$templatedir/$language/custom" 
          && ! -d "$templatedir/$language/default") {
          return "The template directory for $language does not exist";
       }
       push(@validated_languages, $language) unless $lang_seen{$language}++;
    }
    # Rebuild the list of language tags, avoiding duplicates.
    $_[0] = join(', ', @validated_languages);
    return "";
}

sub check_mail_delivery_method {
    my $check = check_multi(@_);
    return $check if $check;
    my $mailer = shift;
    if ($mailer eq 'sendmail' && $^O =~ /MSWin32/i) {
        # look for sendmail.exe 
        return "Failed to locate " . SENDMAIL_EXE
            unless -e SENDMAIL_EXE;
    }
    return "";
}

sub check_notification {
    my $option = shift;
    my @current_version =
        (BUGZILLA_VERSION =~ m/^(\d+)\.(\d+)(?:(rc|\.)(\d+))?\+?$/);
    if ($current_version[1] % 2 && $option eq 'stable_branch_release') {
        return "You are currently running a development snapshot, and so your " .
               "installation is not based on a branch. If you want to be notified " .
               "about the next stable release, you should select " .
               "'latest_stable_release' instead";
    }
    return "";
}

sub check_timezone {
    my $tz = shift;
    unless (defined(tz_offset($tz))) {
        return "must be empty or a legal timezone name, such as PDT or JST";
    }
    return "";
}


# OK, here are the parameter definitions themselves.
#
# Each definition is a hash with keys:
#
# name    - name of the param
# desc    - description of the param (for editparams.cgi)
# type    - see below
# choices - (optional) see below
# default - default value for the param
# checker - (optional) checking function for validating parameter entry
#           It is called with the value of the param as the first arg and a
#           reference to the param's hash as the second argument
#
# The type value can be one of the following:
#
# t -- A short text entry field (suitable for a single line)
# l -- A long text field (suitable for many lines)
# b -- A boolean value (either 1 or 0)
# m -- A list of values, with many selectable (shows up as a select box)
#      To specify the list of values, make the 'choices' key be an array
#      reference of the valid choices. The 'default' key should be an array
#      reference for the list of selected values (which must appear in the
#      first anonymous array), i.e.:
#       {
#         name => 'multiselect',
#         desc => 'A list of options, choose many',
#         type => 'm',
#         choices => [ 'a', 'b', 'c', 'd' ],
#         default => [ 'a', 'd' ],
#         checker => \&check_multi
#       }
#
#      Here, 'a' and 'd' are the default options, and the user may pick any
#      combination of a, b, c, and d as valid options.
#
#      &check_multi should always be used as the param verification function
#      for list (single and multiple) parameter types.
#
# s -- A list of values, with one selectable (shows up as a select box)
#      To specify the list of values, make the 'choices' key be an array
#      reference of the valid choices. The 'default' key should be one of
#      those values, i.e.:
#       {
#         name => 'singleselect',
#         desc => 'A list of options, choose one',
#         type => 's',
#         choices => [ 'a', 'b', 'c' ],
#         default => 'b',
#         checker => \&check_multi
#       }
#
#      Here, 'b' is the default option, and 'a' and 'c' are other possible
#      options, but only one at a time! 
#
#      &check_multi should always be used as the param verification function
#      for list (single and multiple) parameter types.

sub get_param_list {
    return;
}

1;

__END__

=head1 NAME

Bugzilla::Config::Common - Parameter checking functions

=head1 DESCRIPTION

All parameter checking functions are called with two parameters:

=head2 Functions

=over

=item C<check_multi>

Checks that a multi-valued parameter (ie type C<s> or type C<m>) satisfies
its contraints.

=item C<check_numeric>

Checks that the value is a valid number

=item C<check_regexp>

Checks that the value is a valid regexp

=back
