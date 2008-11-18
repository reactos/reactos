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
# The Original Code are the Bugzilla Tests.
# 
# The Initial Developer of the Original Code is Zach Lipton
# Portions created by Zach Lipton are 
# Copyright (C) 2001 Zach Lipton.  All
# Rights Reserved.
# 
# Contributor(s): Zach Lipton <zach@zachlipton.com>
#                 Joel Peshkin <bugreport@peshkin.net>


package Support::Files;

use File::Find;

# exclude_deps is a hash of arrays listing the files to be excluded
# if a module is not available
#
@additional_files = ();
%exclude_deps = (
    'XML::Twig' => ['importxml.pl'],
    'Net::LDAP' => ['Bugzilla/Auth/Verify/LDAP.pm'],
    'Email::Reply' => ['email_in.pl'],
    'Email::MIME::Attachment::Stripper' => ['email_in.pl']
);


@files = glob('*');
find(sub { push(@files, $File::Find::name) if $_ =~ /\.pm$/;}, 'Bugzilla');

sub have_pkg {
    my ($pkg) = @_;
    my ($msg, $vnum, $vstr);
    no strict 'refs';
    eval { my $p; ($p = $pkg . ".pm") =~ s!::!/!g; require $p; };
    return !($@);
}

@exclude_files    = ();
foreach $dep (keys(%exclude_deps)) {
    if (!have_pkg($dep)) {
        push @exclude_files, @{$exclude_deps{$dep}};
    }
}

sub isTestingFile {
    my ($file) = @_;
    my $exclude;
    foreach $exclude (@exclude_files) {
        if ($file eq $exclude) { return undef; } # get rid of excluded files.
    }

    if ($file =~ /\.cgi$|\.pl$|\.pm$/) {
        return 1;
    }
    my $additional;
    foreach $additional (@additional_files) {
        if ($file eq $additional) { return 1; }
    }
    return undef;
}

foreach $currentfile (@files) {
    if (isTestingFile($currentfile)) {
        push(@testitems,$currentfile);
    }
}


1;
