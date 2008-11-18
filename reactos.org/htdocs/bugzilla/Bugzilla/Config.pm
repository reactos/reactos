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
#                 Jake <jake@bugzilla.org>
#                 J. Paul Reed <preed@sigkill.com>
#                 Bradley Baetz <bbaetz@student.usyd.edu.au>
#                 Christopher Aillon <christopher@aillon.com>
#                 Erik Stambaugh <erik@dasbistro.com>
#                 Frédéric Buclin <LpSolit@gmail.com>

package Bugzilla::Config;

use strict;

use base qw(Exporter);
use Bugzilla::Constants;
use Data::Dumper;
use File::Temp;

# Don't export localvars by default - people should have to explicitly
# ask for it, as a (probably futile) attempt to stop code using it
# when it shouldn't
%Bugzilla::Config::EXPORT_TAGS =
  (
   admin => [qw(update_params SetParam write_params)],
  );
Exporter::export_ok_tags('admin');

use vars qw(@param_list);

# INITIALISATION CODE
# Perl throws a warning if we use bz_locations() directly after do.
our %params;
# Load in the param definitions
sub _load_params {
    foreach my $module (param_panels()) {
        eval("require Bugzilla::Config::$module") || die $@;
        my @new_param_list = "Bugzilla::Config::$module"->get_param_list();
        foreach my $item (@new_param_list) {
            $params{$item->{'name'}} = $item;
        }
        push(@param_list, @new_param_list);
    }
}
# END INIT CODE

# Subroutines go here

sub param_panels {
    my @param_panels;
    my $libpath = bz_locations()->{'libpath'};
    foreach my $item ((glob "$libpath/Bugzilla/Config/*.pm")) {
        $item =~ m#/([^/]+)\.pm$#;
        my $module = $1;
        push(@param_panels, $module) unless $module eq 'Common';
    }
    return @param_panels;
}

sub SetParam {
    my ($name, $value) = @_;

    _load_params unless %params;
    die "Unknown param $name" unless (exists $params{$name});

    my $entry = $params{$name};

    # sanity check the value

    # XXX - This runs the checks. Which would be good, except that
    # check_shadowdb creates the database as a side effect, and so the
    # checker fails the second time around...
    if ($name ne 'shadowdb' && exists $entry->{'checker'}) {
        my $err = $entry->{'checker'}->($value, $entry);
        die "Param $name is not valid: $err" unless $err eq '';
    }

    Bugzilla->params->{$name} = $value;
}

sub update_params {
    my ($params) = @_;
    my $answer = Bugzilla->installation_answers;

    my $param = read_param_file();

    # If we didn't return any param values, then this is a new installation.
    my $new_install = !(keys %$param);

    # --- UPDATE OLD PARAMS ---

    # Old Bugzilla versions stored the version number in the params file
    # We don't want it, so get rid of it
    delete $param->{'version'};

    # Change from usebrowserinfo to defaultplatform/defaultopsys combo
    if (exists $param->{'usebrowserinfo'}) {
        if (!$param->{'usebrowserinfo'}) {
            if (!exists $param->{'defaultplatform'}) {
                $param->{'defaultplatform'} = 'Other';
            }
            if (!exists $param->{'defaultopsys'}) {
                $param->{'defaultopsys'} = 'Other';
            }
        }
        delete $param->{'usebrowserinfo'};
    }

    # Change from a boolean for quips to multi-state
    if (exists $param->{'usequip'} && !exists $param->{'enablequips'}) {
        $param->{'enablequips'} = $param->{'usequip'} ? 'on' : 'off';
        delete $param->{'usequip'};
    }

    # Change from old product groups to controls for group_control_map
    # 2002-10-14 bug 147275 bugreport@peshkin.net
    if (exists $param->{'usebuggroups'} && 
        !exists $param->{'makeproductgroups'}) 
    {
        $param->{'makeproductgroups'} = $param->{'usebuggroups'};
    }
    if (exists $param->{'usebuggroupsentry'} 
       && !exists $param->{'useentrygroupdefault'}) {
        $param->{'useentrygroupdefault'} = $param->{'usebuggroupsentry'};
    }

    # Modularise auth code
    if (exists $param->{'useLDAP'} && !exists $param->{'loginmethod'}) {
        $param->{'loginmethod'} = $param->{'useLDAP'} ? "LDAP" : "DB";
    }

    # set verify method to whatever loginmethod was
    if (exists $param->{'loginmethod'} 
        && !exists $param->{'user_verify_class'}) 
    {
        $param->{'user_verify_class'} = $param->{'loginmethod'};
        delete $param->{'loginmethod'};
    }

    # Remove quip-display control from parameters
    # and give it to users via User Settings (Bug 41972)
    if ( exists $param->{'enablequips'} 
         && !exists $param->{'quip_list_entry_control'}) 
    {
        my $new_value;
        ($param->{'enablequips'} eq 'on')       && do {$new_value = 'open';};
        ($param->{'enablequips'} eq 'approved') && do {$new_value = 'moderated';};
        ($param->{'enablequips'} eq 'frozen')   && do {$new_value = 'closed';};
        ($param->{'enablequips'} eq 'off')      && do {$new_value = 'closed';};
        $param->{'quip_list_entry_control'} = $new_value;
        delete $param->{'enablequips'};
    }

    # Old mail_delivery_method choices contained no uppercase characters
    if (exists $param->{'mail_delivery_method'}
        && $param->{'mail_delivery_method'} !~ /[A-Z]/) {
        my $method = $param->{'mail_delivery_method'};
        my %translation = (
            'sendmail' => 'Sendmail',
            'smtp'     => 'SMTP',
            'qmail'    => 'Qmail',
            'testfile' => 'Test',
            'none'     => 'None');
        $param->{'mail_delivery_method'} = $translation{$method};
    }

    # --- DEFAULTS FOR NEW PARAMS ---

    _load_params unless %params;
    foreach my $item (@param_list) {
        my $name = $item->{'name'};
        unless (exists $param->{$name}) {
            print "New parameter: $name\n" unless $new_install;
            $param->{$name} = $answer->{$name} || $item->{'default'};
        }
    }

    $param->{'utf8'} = 1 if $new_install;

    # --- REMOVE OLD PARAMS ---

    my @oldparams;
    # Remove any old params, put them in old-params.txt
    foreach my $item (keys %$param) {
        if (!grep($_ eq $item, map ($_->{'name'}, @param_list))) {
            local $Data::Dumper::Terse  = 1;
            local $Data::Dumper::Indent = 0;
            push (@oldparams, [$item, Data::Dumper->Dump([$param->{$item}])]);
            delete $param->{$item};
        }
    }

    if (@oldparams) {
        my $op_file = new IO::File('old-params.txt', '>>', 0600)
          || die "old-params.txt: $!";

        print "The following parameters are no longer used in Bugzilla,",
              " and so have been\nmoved from your parameters file into",
              " old-params.txt:\n";

        foreach my $p (@oldparams) {
            my ($item, $value) = @$p;
            print $op_file "\n\n$item:\n$value\n";
            print $item;
            print ", " unless $item eq $oldparams[$#oldparams]->[0];
        }
        print "\n";
        $op_file->close;
    }

    if (ON_WINDOWS && !-e SENDMAIL_EXE
        && $param->{'mail_delivery_method'} eq 'Sendmail')
    {
        my $smtp = $answer->{'SMTP_SERVER'};
        if (!$smtp) {
            print "\nBugzilla requires an SMTP server to function on",
                  " Windows.\nPlease enter your SMTP server's hostname: ";
            $smtp = <STDIN>;
            chomp $smtp;
            if ($smtp) {
                $param->{'smtpserver'} = $smtp;
             }
             else {
                print "\nWarning: No SMTP Server provided, defaulting to",
                      " localhost\n";
            }
        }

        $param->{'mail_delivery_method'} = 'SMTP';
    }

    write_params($param);
}

sub write_params {
    my ($param_data) = @_;
    $param_data ||= Bugzilla->params;

    my $datadir    = bz_locations()->{'datadir'};
    my $param_file = "$datadir/params";

    # This only has an affect for Data::Dumper >= 2.12 (ie perl >= 5.8.0)
    # Its just cosmetic, though, so that doesn't matter
    local $Data::Dumper::Sortkeys = 1;

    my ($fh, $tmpname) = File::Temp::tempfile('params.XXXXX',
                                              DIR => $datadir );

    print $fh (Data::Dumper->Dump([$param_data], ['*param']))
      || die "Can't write param file: $!";

    close $fh;

    rename $tmpname, $param_file
      || die "Can't rename $tmpname to $param_file: $!";

    ChmodDataFile($param_file, 0666);

    # And now we have to reset the params cache so that Bugzilla will re-read
    # them.
    delete Bugzilla->request_cache->{params};
}

# Some files in the data directory must be world readable if and only if
# we don't have a webserver group. Call this function to do this.
# This will become a private function once all the datafile handling stuff
# moves into this package

# This sub is not perldoc'd for that reason - noone should know about it
sub ChmodDataFile {
    my ($file, $mask) = @_;
    my $perm = 0770;
    if ((stat(bz_locations()->{'datadir'}))[2] & 0002) {
        $perm = 0777;
    }
    $perm = $perm & $mask;
    chmod $perm,$file;
}

sub read_param_file {
    my %params;
    my $datadir = bz_locations()->{'datadir'};
    if (-e "$datadir/params") {
        # Note that checksetup.pl sets file permissions on '$datadir/params'

        # Using Safe mode is _not_ a guarantee of safety if someone does
        # manage to write to the file. However, it won't hurt...
        # See bug 165144 for not needing to eval this at all
        my $s = new Safe;

        $s->rdo("$datadir/params");
        die "Error reading $datadir/params: $!" if $!;
        die "Error evaluating $datadir/params: $@" if $@;

        # Now read the param back out from the sandbox
        %params = %{$s->varglob('param')};
    }
    elsif ($ENV{'SERVER_SOFTWARE'}) {
       # We're in a CGI, but the params file doesn't exist. We can't
       # Template Toolkit, or even install_string, since checksetup
       # might not have thrown an error. Bugzilla::CGI->new
       # hasn't even been called yet, so we manually use CGI::Carp here
       # so that the user sees the error.
       require CGI::Carp;
       CGI::Carp->import('fatalsToBrowser');
       die "The $datadir/params file does not exist."
           . ' You probably need to run checksetup.pl.',
    }
    return \%params;
}

1;

__END__

=head1 NAME

Bugzilla::Config - Configuration parameters for Bugzilla

=head1 SYNOPSIS

  # Administration functions
  use Bugzilla::Config qw(:admin);

  update_params();
  SetParam($param, $value);
  write_params();

=head1 DESCRIPTION

This package contains ways to access Bugzilla configuration parameters.

=head1 FUNCTIONS

=head2 Parameters

Parameters can be set, retrieved, and updated.

=over 4

=item C<SetParam($name, $value)>

Sets the param named $name to $value. Values are checked using the checker
function for the given param if one exists.

=item C<update_params()>

Updates the parameters, by transitioning old params to new formats, setting
defaults for new params, and removing obsolete ones. Used by F<checksetup.pl>
in the process of an installation or upgrade.

Prints out information about what it's doing, if it makes any changes.

May prompt the user for input, if certain required parameters are not
specified.

=item C<write_params($params)>

Description: Writes the parameters to disk.

Params:      C<$params> (optional) - A hashref to write to the disk
               instead of C<Bugzilla->params>. Used only by
               C<update_params>.

Returns:     nothing

=item C<read_param_file()>

Description: Most callers should never need this. This is used
             by C<Bugzilla->params> to directly read C<$datadir/params>
             and load it into memory. Use C<Bugzilla->params> instead.

Params:      none

Returns:     A hashref containing the current params in C<$datadir/params>.

=back
