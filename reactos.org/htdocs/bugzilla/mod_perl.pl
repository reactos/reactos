#!/usr/bin/perl -wT
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
# Contributor(s): Max Kanat-Alexander <mkanat@bugzilla.org>

package Bugzilla::ModPerl;

use strict;

# If you have an Apache2::Status handler in your Apache configuration,
# you need to load Apache2::Status *here*, so that Apache::DBI can
# report information to Apache2::Status.
#use Apache2::Status ();

# We don't want to import anything into the global scope during
# startup, so we always specify () after using any module in this
# file.

use Apache2::ServerUtil;
use Apache2::SizeLimit;
use ModPerl::RegistryLoader ();
use CGI ();
CGI->compile(qw(:cgi -no_xhtml -oldstyle_urls :private_tempfiles
                :unique_headers SERVER_PUSH :push));
use Template::Config ();
Template::Config->preload();

use Bugzilla ();
use Bugzilla::Constants ();
use Bugzilla::CGI ();
use Bugzilla::Mailer ();
use Bugzilla::Template ();
use Bugzilla::Util ();

# This means that every httpd child will die after processing
# a CGI if it is taking up more than 70MB of RAM all by itself.
$Apache2::SizeLimit::MAX_UNSHARED_SIZE = 70000;

my $cgi_path = Bugzilla::Constants::bz_locations()->{'cgi_path'};

# Set up the configuration for the web server
my $server = Apache2::ServerUtil->server;
my $conf = <<EOT;
<Directory "$cgi_path">
    AddHandler perl-script .cgi
    # No need to PerlModule these because they're already defined in mod_perl.pl
    PerlResponseHandler Bugzilla::ModPerl::ResponseHandler
    PerlCleanupHandler  Bugzilla::ModPerl::CleanupHandler
    PerlCleanupHandler  Apache2::SizeLimit
    PerlOptions +ParseHeaders
    Options +ExecCGI
    AllowOverride Limit
    DirectoryIndex index.cgi index.html
</Directory>
EOT

$server->add_config([split("\n", $conf)]);

# Have ModPerl::RegistryLoader pre-compile all CGI scripts.
my $rl = new ModPerl::RegistryLoader();
# If we try to do this in "new" it fails because it looks for a 
# Bugzilla/ModPerl/ResponseHandler.pm
$rl->{package} = 'Bugzilla::ModPerl::ResponseHandler';
# Note that $cgi_path will be wrong if somebody puts the libraries
# in a different place than the CGIs.
foreach my $file (glob "$cgi_path/*.cgi") {
    Bugzilla::Util::trick_taint($file);
    $rl->handler($file, $file);
}


package Bugzilla::ModPerl::ResponseHandler;
use strict;
use base qw(ModPerl::Registry);
use Bugzilla;

sub handler : method {
    my $class = shift;

    # $0 is broken under mod_perl before 2.0.2, so we have to set it
    # here explicitly or init_page's shutdownhtml code won't work right.
    $0 = $ENV{'SCRIPT_FILENAME'};
    Bugzilla::init_page();
    return $class->SUPER::handler(@_);
}


package Bugzilla::ModPerl::CleanupHandler;
use strict;
use Apache2::Const -compile => qw(OK);

sub handler {
    my $r = shift;

    # Sometimes mod_perl doesn't properly call DESTROY on all
    # the objects in pnotes()
    foreach my $key (keys %{$r->pnotes}) {
        delete $r->pnotes->{$key};
    }

    return Apache2::Const::OK;
}

1;
