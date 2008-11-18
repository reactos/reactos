#!/usr/bin/perl -w
# -*- Mode: perl; indent-tabs-mode: nil -*-
#
# The contents of this file are subject to the Mozilla Public
# License Version 1.1 (the “License”); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an “AS
# IS” basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is the Bugzilla Bug Tracking System.
#
# Contributor(s): Marc Schumann <wurblzap@gmail.com>
#                 Mads Bondo Dydensborg <mbd@dbc.dk>

=head1 NAME

bz_webservice_demo.pl - Show how to talk to Bugzilla via XMLRPC

=head1 SYNOPSIS

C<bz_webservice_demo.pl [options]>

C<bz_webservice_demo.pl --help> for detailed help

=cut

use strict;
use Getopt::Long;
use Pod::Usage;
use File::Basename qw(dirname);
use File::Spec;
use HTTP::Cookies;
use XMLRPC::Lite;

# If you want, say “use Bugzilla::WebService::Constants” here to get access
# to Bugzilla's web service error code constants.
# If you do this, remember to issue a “use lib” pointing to your Bugzilla
# installation directory, too.

my $help;
my $Bugzilla_uri;
my $Bugzilla_login;
my $Bugzilla_password;
my $Bugzilla_remember;
my $bug_id;
my $product_name;
my $create_file_name;
my $legal_field_values;

GetOptions('help|h|?'       => \$help,
           'uri=s'          => \$Bugzilla_uri,
           'login:s'        => \$Bugzilla_login,
           'password=s'     => \$Bugzilla_password,
           'rememberlogin!' => \$Bugzilla_remember,
           'bug_id:s'       => \$bug_id,
           'product_name:s' => \$product_name,
           'create:s'       => \$create_file_name,
           'field:s'        => \$legal_field_values
          ) or pod2usage({'-verbose' => 0, '-exitval' => 1});

=head1 OPTIONS

=over

=item --help, -h, -?

Print a short help message and exit.

=item --uri

URI to Bugzilla's C<xmlrpc.cgi> script, along the lines of
C<http://your.bugzilla.installation/path/to/bugzilla/xmlrpc.cgi>.

=item --login

Bugzilla login name. Specify this together with B<--password> in order to log in.

Specify this without a value in order to log out.

=item --password

Bugzilla password. Specify this together with B<--login> in order to log in.

=item --rememberlogin

Gives access to Bugzilla's “Bugzilla_remember” option.
Specify this option while logging in to do the same thing as ticking the
C<Bugzilla_remember> box on Bugilla's log in form.
Don't specify this option to do the same thing as unchecking the box.

See Bugzilla's rememberlogin parameter for details.

=item --bug_id

Pass a bug ID to have C<bz_webservice_demo.pl> do some bug-related test calls.

=item --product_name

Pass a product name to have C<bz_webservice_demo.pl> do some product-related
test calls.

=item --create

Specify a file that contains settings for the creating of a new bug.

=item --field

Pass a field name to get legal values for this field. It must be either a
global select field (such as bug_status, resolution, rep_platform, op_sys,
priority, bug_severity) or a custom select field.

=back

=head1 DESCRIPTION

=cut

pod2usage({'-verbose' => 1, '-exitval' => 0}) if $help;
_syntaxhelp('URI unspecified') unless $Bugzilla_uri;

# We will use this variable for SOAP call results.
my $soapresult;

# We will use this variable for function call results.
my $result;

# Open our cookie jar. We save it into a file so that we may re-use cookies
# to avoid the need of logging in every time. You're encouraged, but not
# required, to do this in your applications, too.
# Cookies are only saved if Bugzilla's rememberlogin parameter is set to one of
#    - on
#    - defaulton (and you didn't pass 0 as third parameter to User.login)
#    - defaultoff (and you passed 1 as third parameter to User.login)
my $cookie_jar =
    new HTTP::Cookies('file' => File::Spec->catdir(dirname($0), 'cookies.txt'),
                      'autosave' => 1);

=head2 Initialization

Using the XMLRPC::Lite class, you set up a proxy, as shown in this script.
Bugzilla's XMLRPC URI ends in C<xmlrpc.cgi>, so your URI looks along the lines
of C<http://your.bugzilla.installation/path/to/bugzilla/xmlrpc.cgi>.

=cut

my $proxy = XMLRPC::Lite->proxy($Bugzilla_uri,
                                'cookie_jar' => $cookie_jar);

=head2 Checking Bugzilla's version

To make sure the Bugzilla you're connecting to supports the methods you wish to
call, you may want to compare the result of C<Bugzilla.version> to the
minimum required version your application needs.

=cut

$soapresult = $proxy->call('Bugzilla.version');
_die_on_fault($soapresult);
print 'Connecting to a Bugzilla of version ' . $soapresult->result()->{version} . ".\n";

=head2 Checking Bugzilla's timezone

To make sure that you understand the dates and times that Bugzilla returns to you, you may want to call C<Bugzilla.timezone>.

=cut

$soapresult = $proxy->call('Bugzilla.timezone');
_die_on_fault($soapresult);
print 'Bugzilla\'s timezone is ' . $soapresult->result()->{timezone} . ".\n";

=head2 Logging In and Out

=head3 Using Bugzilla's Environment Authentication

Use a
C<http://login:password@your.bugzilla.installation/path/to/bugzilla/xmlrpc.cgi>
style URI.
You don't log out if you're using this kind of authentication.

=head3 Using Bugzilla's CGI Variable Authentication

Use the C<User.login> and C<User.logout> calls to log in and out, as shown
in this script.

The C<Bugzilla_remember> parameter is optional.
If omitted, Bugzilla's defaults apply (as specified by its C<rememberlogin>
parameter).

Bugzilla hands back cookies you'll need to pass along during your work calls.

=cut

if (defined($Bugzilla_login)) {
    if ($Bugzilla_login ne '') {
        # Log in.
        $soapresult = $proxy->call('User.login',
                                   { login => $Bugzilla_login, 
                                     password => $Bugzilla_password,
                                     remember => $Bugzilla_remember } );
        _die_on_fault($soapresult);
        print "Login successful.\n";
    }
    else {
        # Log out.
        $soapresult = $proxy->call('User.logout');
        _die_on_fault($soapresult);
        print "Logout successful.\n";
    }
}

=head2 Retrieving Bug Information

Call C<Bug.get_bug> with the ID of the bug you want to know more of.
The call will return a C<Bugzilla::Bug> object.

=cut

if ($bug_id) {
    $soapresult = $proxy->call('Bug.get_bugs', { ids => [$bug_id] });
    _die_on_fault($soapresult);
    $result = $soapresult->result;
    my $bug = $result->{bugs}->[0];
    foreach my $field (keys(%$bug)) {
        my $value = $bug->{$field};
        if (ref($value) eq 'HASH') {
            foreach (keys %$value) {
                print "$_: " . $value->{$_} . "\n";
            }
        }
        else {
            print "$field: $value\n";
        }
    }
}

=head2 Retrieving Product Information

Call C<Product.get_product> with the name of the product you want to know more
of.
The call will return a C<Bugzilla::Product> object.

=cut

if ($product_name) {
    $soapresult = $proxy->call('Product.get_product', $product_name);
    _die_on_fault($soapresult);
    $result = $soapresult->result;

    if (ref($result) eq 'HASH') {
        foreach (keys(%$result)) {
            print "$_: $$result{$_}\n";
        }
    }
    else {
        print "$result\n";
    }
}

=head2 Creating A Bug

Call C<Bug.create> with the settings read from the file indicated on
the command line. The file must contain a valid anonymous hash to use 
as argument for the call to C<Bug.create>.
The call will return a hash with a bug id for the newly created bug.

=cut

if ($create_file_name) {
    $soapresult = $proxy->call('Bug.create', do "$create_file_name" );
    _die_on_fault($soapresult);
    $result = $soapresult->result;

    if (ref($result) eq 'HASH') {
        foreach (keys(%$result)) {
            print "$_: $$result{$_}\n";
        }
    }
    else {
        print "$result\n";
    }

}

=head2 Getting Legal Field Values

Call C<Bug.legal_values> with the name of the field (including custom
select fields). The call will return a reference to an array with the
list of legal values for this field.

=cut

if ($legal_field_values) {
    $soapresult = $proxy->call('Bug.legal_values', {field => $legal_field_values} );
    _die_on_fault($soapresult);
    $result = $soapresult->result;

    print join("\n", @{$result->{values}}) . "\n";
}


=head1 NOTES

=head2 Character Set Encoding

Make sure that your application either uses the same character set
encoding as Bugzilla does, or that it converts correspondingly when using the
web service API.
By default, Bugzilla uses UTF-8 as its character set encoding.

=head2 Format For Create File

The create format file is a piece of Perl code, that should look something like
this:

    {
        product     => "TestProduct", 
        component   => "TestComponent",
        summary     => "TestBug - created from bz_webservice_demo.pl",
        version     => "unspecified",
        description => "This is a description of the bug... hohoho",
        op_sys      => "All",
        platform    => "All",	
        priority    => "P4",
        severity    => "normal"
    };

=head1 SEE ALSO

There are code comments in C<bz_webservice_demo.pl> which might be of further
help to you.

=cut

sub _die_on_fault {
    my $soapresult = shift;

    if ($soapresult->fault) {
        my ($package, $filename, $line) = caller;
        die $soapresult->faultcode . ' ' . $soapresult->faultstring .
            " in SOAP call near $filename line $line.\n";
    }
}

sub _syntaxhelp {
    my $msg = shift;

    print "Error: $msg\n";
    pod2usage({'-verbose' => 0, '-exitval' => 1});
}
