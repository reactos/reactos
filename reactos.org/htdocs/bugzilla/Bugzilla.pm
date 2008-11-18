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
# Contributor(s): Bradley Baetz <bbaetz@student.usyd.edu.au>
#                 Erik Stambaugh <erik@dasbistro.com>
#                 A. Karl Kornel <karl@kornel.name>
#                 Marc Schumann <wurblzap@gmail.com>

package Bugzilla;

use strict;

use Bugzilla::Config;
use Bugzilla::Constants;
use Bugzilla::Auth;
use Bugzilla::Auth::Persist::Cookie;
use Bugzilla::CGI;
use Bugzilla::DB;
use Bugzilla::Install::Localconfig qw(read_localconfig);
use Bugzilla::Template;
use Bugzilla::User;
use Bugzilla::Error;
use Bugzilla::Util;
use Bugzilla::Field;

use File::Basename;
use Safe;

# This creates the request cache for non-mod_perl installations.
our $_request_cache = {};

#####################################################################
# Constants
#####################################################################

# Scripts that are not stopped by shutdownhtml being in effect.
use constant SHUTDOWNHTML_EXEMPT => [
    'editparams.cgi',
    'checksetup.pl',
    'recode.pl',
];

# Non-cgi scripts that should silently exit.
use constant SHUTDOWNHTML_EXIT_SILENTLY => [
    'whine.pl'
];

#####################################################################
# Global Code
#####################################################################

# The following subroutine is for debugging purposes only.
# Uncommenting this sub and the $::SIG{__DIE__} trap underneath it will
# cause any fatal errors to result in a call stack trace to help track
# down weird errors.
#
#sub die_with_dignity {
#    use Carp ();
#    my ($err_msg) = @_;
#    print $err_msg;
#    Carp::confess($err_msg);
#}
#$::SIG{__DIE__} = \&Bugzilla::die_with_dignity;

sub init_page {

    # Some environment variables are not taint safe
    delete @::ENV{'PATH', 'IFS', 'CDPATH', 'ENV', 'BASH_ENV'};
    # Some modules throw undefined errors (notably File::Spec::Win32) if
    # PATH is undefined.
    $ENV{'PATH'} = '';

    # IIS prints out warnings to the webpage, so ignore them, or log them
    # to a file if the file exists.
    if ($ENV{SERVER_SOFTWARE} && $ENV{SERVER_SOFTWARE} =~ /microsoft-iis/i) {
        $SIG{__WARN__} = sub {
            my ($msg) = @_;
            my $datadir = bz_locations()->{'datadir'};
            if (-w "$datadir/errorlog") {
                my $warning_log = new IO::File(">>$datadir/errorlog");
                print $warning_log $msg;
                $warning_log->close();
            }
        };
    }

    # If Bugzilla is shut down, do not allow anything to run, just display a
    # message to the user about the downtime and log out.  Scripts listed in 
    # SHUTDOWNHTML_EXEMPT are exempt from this message.
    #
    # Because this is code which is run live from perl "use" commands of other
    # scripts, we're skipping this part if we get here during a perl syntax 
    # check -- runtests.pl compiles scripts without running them, so we 
    # need to make sure that this check doesn't apply to 'perl -c' calls.
    #
    # This code must go here. It cannot go anywhere in Bugzilla::CGI, because
    # it uses Template, and that causes various dependency loops.
    if (!$^C && Bugzilla->params->{"shutdownhtml"} 
        && lsearch(SHUTDOWNHTML_EXEMPT, basename($0)) == -1)
    {
        # Allow non-cgi scripts to exit silently (without displaying any
        # message), if desired. At this point, no DBI call has been made
        # yet, and no error will be returned if the DB is inaccessible.
        if (lsearch(SHUTDOWNHTML_EXIT_SILENTLY, basename($0)) > -1
            && !i_am_cgi())
        {
            exit;
        }

        # For security reasons, log out users when Bugzilla is down.
        # Bugzilla->login() is required to catch the logincookie, if any.
        my $user = Bugzilla->login(LOGIN_OPTIONAL);
        my $userid = $user->id;
        Bugzilla->logout();

        my $template = Bugzilla->template;
        my $vars = {};
        $vars->{'message'} = 'shutdown';
        $vars->{'userid'} = $userid;
        # Generate and return a message about the downtime, appropriately
        # for if we're a command-line script or a CGI script.
        my $extension;
        if (i_am_cgi() && (!Bugzilla->cgi->param('ctype')
                           || Bugzilla->cgi->param('ctype') eq 'html')) {
            $extension = 'html';
        }
        else {
            $extension = 'txt';
        }
        print Bugzilla->cgi->header() if i_am_cgi();
        my $t_output;
        $template->process("global/message.$extension.tmpl", $vars, \$t_output)
            || ThrowTemplateError($template->error);
        print $t_output . "\n";
        exit;
    }
}

init_page() if !$ENV{MOD_PERL};

#####################################################################
# Subroutines and Methods
#####################################################################

sub template {
    my $class = shift;
    request_cache()->{language} = "";
    request_cache()->{template} ||= Bugzilla::Template->create();
    return request_cache()->{template};
}

sub template_inner {
    my ($class, $lang) = @_;
    $lang = defined($lang) ? $lang : (request_cache()->{language} || "");
    request_cache()->{language} = $lang;
    request_cache()->{"template_inner_$lang"} ||= Bugzilla::Template->create();
    return request_cache()->{"template_inner_$lang"};
}

sub cgi {
    my $class = shift;
    request_cache()->{cgi} ||= new Bugzilla::CGI();
    return request_cache()->{cgi};
}

sub localconfig {
    request_cache()->{localconfig} ||= read_localconfig();
    return request_cache()->{localconfig};
}

sub params {
    my $class = shift;
    request_cache()->{params} ||= Bugzilla::Config::read_param_file();
    return request_cache()->{params};
}

sub user {
    my $class = shift;
    request_cache()->{user} ||= new Bugzilla::User;
    return request_cache()->{user};
}

sub set_user {
    my ($class, $user) = @_;
    $class->request_cache->{user} = $user;
}

sub sudoer {
    my $class = shift;    
    return request_cache()->{sudoer};
}

sub sudo_request {
    my $class = shift;
    my $new_user = shift;
    my $new_sudoer = shift;

    request_cache()->{user}   = $new_user;
    request_cache()->{sudoer} = $new_sudoer;

    # NOTE: If you want to log the start of an sudo session, do it here.

    return;
}

sub login {
    my ($class, $type) = @_;

    return Bugzilla->user if Bugzilla->user->id;

    my $authorizer = new Bugzilla::Auth();
    $type = LOGIN_REQUIRED if Bugzilla->cgi->param('GoAheadAndLogIn');
    if (!defined $type || $type == LOGIN_NORMAL) {
        $type = Bugzilla->params->{'requirelogin'} ? LOGIN_REQUIRED : LOGIN_NORMAL;
    }
    my $authenticated_user = $authorizer->login($type);
    
    # At this point, we now know if a real person is logged in.
    # We must now check to see if an sudo session is in progress.
    # For a session to be in progress, the following must be true:
    # 1: There must be a logged in user
    # 2: That user must be in the 'bz_sudoer' group
    # 3: There must be a valid value in the 'sudo' cookie
    # 4: A Bugzilla::User object must exist for the given cookie value
    # 5: That user must NOT be in the 'bz_sudo_protect' group
    my $sudo_cookie = $class->cgi->cookie('sudo');
    detaint_natural($sudo_cookie) if defined($sudo_cookie);
    my $sudo_target;
    $sudo_target = new Bugzilla::User($sudo_cookie) if defined($sudo_cookie);
    if (defined($authenticated_user)                 &&
        $authenticated_user->in_group('bz_sudoers')  &&
        defined($sudo_cookie)                        &&
        defined($sudo_target)                        &&
        !($sudo_target->in_group('bz_sudo_protect'))
       )
    {
        Bugzilla->set_user($sudo_target);
        request_cache()->{sudoer} = $authenticated_user;
        # And make sure that both users have the same Auth object,
        # since we never call Auth::login for the sudo target.
        $sudo_target->set_authorizer($authenticated_user->authorizer);

        # NOTE: If you want to do any special logging, do it here.
    }
    else {
        Bugzilla->set_user($authenticated_user);
    }
    
    return Bugzilla->user;
}

sub logout {
    my ($class, $option) = @_;

    # If we're not logged in, go away
    return unless user->id;

    $option = LOGOUT_CURRENT unless defined $option;
    Bugzilla::Auth::Persist::Cookie->logout({type => $option});
    Bugzilla->logout_request() unless $option eq LOGOUT_KEEP_CURRENT;
}

sub logout_user {
    my ($class, $user) = @_;
    # When we're logging out another user we leave cookies alone, and
    # therefore avoid calling Bugzilla->logout() directly.
    Bugzilla::Auth::Persist::Cookie->logout({user => $user});
}

# just a compatibility front-end to logout_user that gets a user by id
sub logout_user_by_id {
    my ($class, $id) = @_;
    my $user = new Bugzilla::User($id);
    $class->logout_user($user);
}

# hack that invalidates credentials for a single request
sub logout_request {
    delete request_cache()->{user};
    delete request_cache()->{sudoer};
    # We can't delete from $cgi->cookie, so logincookie data will remain
    # there. Don't rely on it: use Bugzilla->user->login instead!
}

sub dbh {
    my $class = shift;

    # If we're not connected, then we must want the main db
    request_cache()->{dbh} ||= request_cache()->{dbh_main} 
        = Bugzilla::DB::connect_main();

    return request_cache()->{dbh};
}

sub error_mode {
    my $class = shift;
    my $newval = shift;
    if (defined $newval) {
        request_cache()->{error_mode} = $newval;
    }
    return request_cache()->{error_mode}
        || Bugzilla::Constants::ERROR_MODE_WEBPAGE;
}

sub usage_mode {
    my $class = shift;
    my $newval = shift;
    if (defined $newval) {
        if ($newval == USAGE_MODE_BROWSER) {
            $class->error_mode(ERROR_MODE_WEBPAGE);
        }
        elsif ($newval == USAGE_MODE_CMDLINE) {
            $class->error_mode(ERROR_MODE_DIE);
        }
        elsif ($newval == USAGE_MODE_WEBSERVICE) {
            $class->error_mode(ERROR_MODE_DIE_SOAP_FAULT);
        }
        elsif ($newval == USAGE_MODE_EMAIL) {
            $class->error_mode(ERROR_MODE_DIE);
        }
        else {
            ThrowCodeError('usage_mode_invalid',
                           {'invalid_usage_mode', $newval});
        }
        request_cache()->{usage_mode} = $newval;
    }
    return request_cache()->{usage_mode}
        || Bugzilla::Constants::USAGE_MODE_BROWSER;
}

sub installation_mode {
    my ($class, $newval) = @_;
    ($class->request_cache->{installation_mode} = $newval) if defined $newval;
    return $class->request_cache->{installation_mode}
        || INSTALLATION_MODE_INTERACTIVE;
}

sub installation_answers {
    my ($class, $filename) = @_;
    if ($filename) {
        my $s = new Safe;
        $s->rdo($filename);

        die "Error reading $filename: $!" if $!;
        die "Error evaluating $filename: $@" if $@;

        # Now read the param back out from the sandbox
        $class->request_cache->{installation_answers} = $s->varglob('answer');
    }
    return $class->request_cache->{installation_answers} || {};
}

sub switch_to_shadow_db {
    my $class = shift;

    if (!request_cache()->{dbh_shadow}) {
        if (Bugzilla->params->{'shadowdb'}) {
            request_cache()->{dbh_shadow} = Bugzilla::DB::connect_shadow();
        } else {
            request_cache()->{dbh_shadow} = request_cache()->{dbh_main};
        }
    }

    request_cache()->{dbh} = request_cache()->{dbh_shadow};
    # we have to return $class->dbh instead of {dbh} as
    # {dbh_shadow} may be undefined if no shadow DB is used
    # and no connection to the main DB has been established yet.
    return $class->dbh;
}

sub switch_to_main_db {
    my $class = shift;

    request_cache()->{dbh} = request_cache()->{dbh_main};
    # We have to return $class->dbh instead of {dbh} as
    # {dbh_main} may be undefined if no connection to the main DB
    # has been established yet.
    return $class->dbh;
}

sub get_fields {
    my $class = shift;
    my $criteria = shift;
    # This function may be called during installation, and Field::match
    # may fail at that time. so we want to return an empty list in that
    # case.
    my $fields = eval { Bugzilla::Field->match($criteria) } || [];
    return @$fields;
}

sub custom_field_names {
    # Get a list of custom fields and convert it into a list of their names.
    return map($_->{name}, 
               @{Bugzilla::Field->match({ custom=>1, obsolete=>0 })});
}

sub hook_args {
    my ($class, $args) = @_;
    $class->request_cache->{hook_args} = $args if $args;
    return $class->request_cache->{hook_args};
}

sub request_cache {
    if ($ENV{MOD_PERL}) {
        require Apache2::RequestUtil;
        return Apache2::RequestUtil->request->pnotes();
    }
    return $_request_cache;
}

# Private methods

# Per process cleanup
sub _cleanup {

    # When we support transactions, need to ->rollback here
    my $main   = request_cache()->{dbh_main};
    my $shadow = request_cache()->{dbh_shadow};
    $main->disconnect if $main;
    $shadow->disconnect if $shadow && Bugzilla->params->{"shadowdb"};
    undef $_request_cache;
}

sub END {
    # Bugzilla.pm cannot compile in mod_perl.pl if this runs.
    _cleanup() unless $ENV{MOD_PERL};
}

1;

__END__

=head1 NAME

Bugzilla - Semi-persistent collection of various objects used by scripts
and modules

=head1 SYNOPSIS

  use Bugzilla;

  sub someModulesSub {
    Bugzilla->dbh->prepare(...);
    Bugzilla->template->process(...);
  }

=head1 DESCRIPTION

Several Bugzilla 'things' are used by a variety of modules and scripts. This
includes database handles, template objects, and so on.

This module is a singleton intended as a central place to store these objects.
This approach has several advantages:

=over 4

=item *

They're not global variables, so we don't have issues with them staying around
with mod_perl

=item *

Everything is in one central place, so it's easy to access, modify, and maintain

=item *

Code in modules can get access to these objects without having to have them
all passed from the caller, and the caller's caller, and....

=item *

We can reuse objects across requests using mod_perl where appropriate (eg
templates), whilst destroying those which are only valid for a single request
(such as the current user)

=back

Note that items accessible via this object are demand-loaded when requested.

For something to be added to this object, it should either be able to benefit
from persistence when run under mod_perl (such as the a C<template> object),
or should be something which is globally required by a large ammount of code
(such as the current C<user> object).

=head1 METHODS

Note that all C<Bugzilla> functionality is method based; use C<Bugzilla-E<gt>dbh>
rather than C<Bugzilla::dbh>. Nothing cares about this now, but don't rely on
that.

=over 4

=item C<template>

The current C<Template> object, to be used for output

=item C<template_inner>

If you ever need a L<Bugzilla::Template> object while you're already
processing a template, use this. Also use it if you want to specify
the language to use. If no argument is passed, it uses the last
language set. If the argument is "" (empty string), the language is
reset to the current one (the one used by Bugzilla->template).

=item C<cgi>

The current C<cgi> object. Note that modules should B<not> be using this in
general. Not all Bugzilla actions are cgi requests. Its useful as a convenience
method for those scripts/templates which are only use via CGI, though.

=item C<user>

C<undef> if there is no currently logged in user or if the login code has not
yet been run.  If an sudo session is in progress, the C<Bugzilla::User>
corresponding to the person who is being impersonated.  If no session is in
progress, the current C<Bugzilla::User>.

=item C<set_user>

Allows you to directly set what L</user> will return. You can use this
if you want to bypass L</login> for some reason and directly "log in"
a specific L<Bugzilla::User>. Be careful with it, though!

=item C<sudoer>

C<undef> if there is no currently logged in user, the currently logged in user
is not in the I<sudoer> group, or there is no session in progress.  If an sudo
session is in progress, returns the C<Bugzilla::User> object corresponding to
the person who logged in and initiated the session.  If no session is in
progress, returns the C<Bugzilla::User> object corresponding to the currently
logged in user.

=item C<sudo_request>
This begins an sudo session for the current request.  It is meant to be 
used when a session has just started.  For normal use, sudo access should 
normally be set at login time.

=item C<login>

Logs in a user, returning a C<Bugzilla::User> object, or C<undef> if there is
no logged in user. See L<Bugzilla::Auth|Bugzilla::Auth>, and
L<Bugzilla::User|Bugzilla::User>.

=item C<logout($option)>

Logs out the current user, which involves invalidating user sessions and
cookies. Three options are available from
L<Bugzilla::Constants|Bugzilla::Constants>: LOGOUT_CURRENT (the
default), LOGOUT_ALL or LOGOUT_KEEP_CURRENT.

=item C<logout_user($user)>

Logs out the specified user (invalidating all his sessions), taking a
Bugzilla::User instance.

=item C<logout_by_id($id)>

Logs out the user with the id specified. This is a compatibility
function to be used in callsites where there is only a userid and no
Bugzilla::User instance.

=item C<logout_request>

Essentially, causes calls to C<Bugzilla-E<gt>user> to return C<undef>. This has the
effect of logging out a user for the current request only; cookies and
database sessions are left intact.

=item C<error_mode>

Call either C<Bugzilla->error_mode(Bugzilla::Constants::ERROR_MODE_DIE)>
or C<Bugzilla->error_mode(Bugzilla::Constants::ERROR_MODE_DIE_SOAP_FAULT)> to
change this flag's default of C<Bugzilla::Constants::ERROR_MODE_WEBPAGE> and to
indicate that errors should be passed to error mode specific error handlers
rather than being sent to a browser and finished with an exit().

This is useful, for example, to keep C<eval> blocks from producing wild HTML
on errors, making it easier for you to catch them.
(Remember to reset the error mode to its previous value afterwards, though.)

C<Bugzilla->error_mode> will return the current state of this flag.

Note that C<Bugzilla->error_mode> is being called by C<Bugzilla->usage_mode> on
usage mode changes.

=item C<usage_mode>

Call either C<Bugzilla->usage_mode(Bugzilla::Constants::USAGE_MODE_CMDLINE)>
or C<Bugzilla->usage_mode(Bugzilla::Constants::USAGE_MODE_WEBSERVICE)> near the
beginning of your script to change this flag's default of
C<Bugzilla::Constants::USAGE_MODE_BROWSER> and to indicate that Bugzilla is
being called in a non-interactive manner.
This influences error handling because on usage mode changes, C<usage_mode>
calls C<Bugzilla->error_mode> to set an error mode which makes sense for the
usage mode.

C<Bugzilla->usage_mode> will return the current state of this flag.

=item C<installation_mode>

Determines whether or not installation should be silent. See 
L<Bugzilla::Constants> for the C<INSTALLATION_MODE> constants.

=item C<installation_answers>

Returns a hashref representing any "answers" file passed to F<checksetup.pl>,
used to automatically answer or skip prompts.

=item C<dbh>

The current database handle. See L<DBI>.

=item C<switch_to_shadow_db>

Switch from using the main database to using the shadow database.

=item C<switch_to_main_db>

Change the database object to refer to the main database.

=item C<params>

The current Parameters of Bugzilla, as a hashref. If C<data/params>
does not exist, then we return an empty hashref. If C<data/params>
is unreadable or is not valid perl, we C<die>.

=item C<hook_args>

If you are running inside a code hook (see L<Bugzilla::Hook>) this
is how you get the arguments passed to the hook.

=back
