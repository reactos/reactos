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
# Contributor(s): Zach Lipton <zach@zachlipton.com>
#

package Bugzilla::Hook;

use Bugzilla::Constants;
use Bugzilla::Util;
use Bugzilla::Error;

use strict;

sub process {
    my ($name, $args) = @_;
    
    # get a list of all extensions
    my @extensions = glob(bz_locations()->{'extensionsdir'} . "/*");
    
    # check each extension to see if it uses the hook
    # if so, invoke the extension source file:
    foreach my $extension (@extensions) {
        # all of these variables come directly from code or directory names. 
        # If there's malicious data here, we have much bigger issues to 
        # worry about, so we can safely detaint them:
        trick_taint($extension);
        if (-e $extension.'/code/'.$name.'.pl') {
            Bugzilla->hook_args($args);
            do($extension.'/code/'.$name.'.pl');
            ThrowCodeError('extension_invalid', 
                { errstr => $@, name => $name, extension => $extension }) if $@;
            # Flush stored data.
            Bugzilla->hook_args({});
        }
    }
    
}

1;

__END__

=head1 NAME

Bugzilla::Hook - Extendible extension hooks for Bugzilla code

=head1 SYNOPSIS

 use Bugzilla::Hook;

 Bugzilla::Hook::process("hookname", { arg => $value, arg2 => $value2 });

=head1 DESCRIPTION

Bugzilla allows extension modules to drop in and add routines at 
arbitrary points in Bugzilla code. These points are refered to as 
hooks. When a piece of standard Bugzilla code wants to allow an extension
to perform additional functions, it uses Bugzilla::Hook's L</process>
subroutine to invoke any extension code if installed. 

=head2 How Hooks Work

When a hook named C<HOOK_NAME> is run, Bugzilla will attempt to invoke any 
source files named F<extensions/*/code/HOOK_NAME.pl>.

So, for example, if your extension is called "testopia", and you
want to have code run during the L</install-update_db> hook, you
would have a file called F<extensions/testopia/code/install-update_db.pl>
that contained perl code to run during that hook.

=head2 Arguments Passed to Hooks

Some L<hooks|/HOOKS> have params that are passed to them.

These params are accessible through L<Bugzilla/hook_args>.
That returns a hashref. Very frequently, if you want your
hook to do anything, you have to modify these variables.

=head1 SUBROUTINES

=over

=item C<process>

=over

=item B<Description>

Invoke any code hooks with a matching name from any installed extensions.

See C<customization.xml> in the Bugzilla Guide for more information on
Bugzilla's extension mechanism.

=item B<Params>

=over

=item C<$name> - The name of the hook to invoke.

=item C<$args> - A hashref. The named args to pass to the hook. 
They will be accessible to the hook via L<Bugzilla/hook_args>.

=back

=item B<Returns> (nothing)

=back

=back

=head1 HOOKS

This describes what hooks exist in Bugzilla currently.

=head2 enter_bug-entrydefaultvars

This happens right before the template is loaded on enter_bug.cgi.

Params:

=over

=item C<vars> - A hashref. The variables that will be passed into the template.

=back

=head2 install-requirements

Because of the way Bugzilla installation works, there can't be a normal
hook during the time that F<checksetup.pl> checks what modules are
installed. (C<Bugzilla::Hook> needs to have those modules installed--it's
a chicken-and-egg problem.)

So instead of the way hooks normally work, this hook just looks for two 
subroutines (or constants, since all constants are just subroutines) in 
your file, called C<OPTIONAL_MODULES> and C<REQUIRED_MODULES>,
which should return arrayrefs in the same format as C<OPTIONAL_MODULES> and
C<REQUIRED_MODULES> in L<Bugzilla::Install::Requirements>.

These subroutines will be passed an arrayref that contains the current
Bugzilla requirements of the same type, in case you want to modify
Bugzilla's requirements somehow. (Probably the most common would be to
alter a version number or the "feature" element of C<OPTIONAL_MODULES>.)

F<checksetup.pl> will add these requirements to its own.

Please remember--if you put something in C<REQUIRED_MODULES>, then
F<checksetup.pl> B<cannot complete> unless the user has that module
installed! So use C<OPTIONAL_MODULES> whenever you can.

=head2 install-update_db

This happens at the very end of all the tables being updated
during an installation or upgrade. If you need to modify your custom
schema, do it here. No params are passed.

=head2 db_schema-abstract_schema

This allows you to add tables to Bugzilla. Note that we recommend that you 
prefix the names of your tables with some word, so that they don't conflict 
with any future Bugzilla tables.

If you wish to add new I<columns> to existing Bugzilla tables, do that
in L</install-update_db>.

Params:

=over

=item C<schema> - A hashref, in the format of 
L<Bugzilla::DB::Schema/ABSTRACT_SCHEMA>. Add new hash keys to make new table
definitions. F<checksetup.pl> will automatically add these tables to the
database when run.

=back
