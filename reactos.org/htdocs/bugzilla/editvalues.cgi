#!/usr/bin/perl -wT
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
#                 Frédéric Buclin <LpSolit@gmail.com>

# This is a script to edit the values of fields that have drop-down
# or select boxes. It is largely a copy of editmilestones.cgi, but 
# with some cleanup.

use strict;
use lib ".";

use Bugzilla;
use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::Constants;
use Bugzilla::Config qw(:admin);
use Bugzilla::Token;

# List of different tables that contain the changeable field values
# (the old "enums.") Keep them in alphabetical order by their 
# English name from field-descs.html.tmpl.
# Format: Array of valid field names.
# Admins may add bug_status to this list, but they do so at their own risk.
our @valid_fields = ('op_sys', 'rep_platform', 'priority', 'bug_severity',
                     'resolution');

# Add custom select fields.
my @custom_fields = Bugzilla->get_fields({custom => 1,
                                          type => FIELD_TYPE_SINGLE_SELECT});

push(@valid_fields, map { $_->name } @custom_fields);

######################################################################
# Subroutines
######################################################################

# Returns whether or not the specified table exists in the @tables array.
sub FieldExists {
  my ($field) = @_;

  return lsearch(\@valid_fields, $field) >= 0;
}

# Same as FieldExists, but emits and error and dies if it fails.
sub FieldMustExist {
    my ($field)= @_;

    $field ||
        ThrowUserError('fieldname_not_specified');

    # Is it a valid field to be editing?
    FieldExists($field) ||
        ThrowUserError('fieldname_invalid', {'field' => $field});
}

# Returns if the specified value exists for the field specified.
sub ValueExists {
    my ($field, $value) = @_;
    FieldMustExist($field);
    trick_taint($field);
    # Value is safe because it's being passed only to a SELECT
    # statement via a placeholder.
    trick_taint($value);

    my $dbh = Bugzilla->dbh;
    my $value_count = 
        $dbh->selectrow_array("SELECT COUNT(*) FROM $field "
                           .  " WHERE value = ?", undef, $value);

    return $value_count;
}

# Same check as ValueExists, emits an error text and dies if it fails.
sub ValueMustExist {
    my ($field, $value)= @_;

    # Values may not be empty (it's very difficult to deal 
    # with empty values in the admin interface).
    trim($value) || ThrowUserError('fieldvalue_not_specified');

    # Does it exist in the DB?
    ValueExists($field, $value) ||
        ThrowUserError('fieldvalue_doesnt_exist', {'value' => $value,
                                                   'field' => $field});
}

######################################################################
# Main Body Execution
######################################################################

# require the user to have logged in
Bugzilla->login(LOGIN_REQUIRED);

my $dbh      = Bugzilla->dbh;
my $cgi      = Bugzilla->cgi;
my $template = Bugzilla->template;
my $vars = {};

print $cgi->header();

exists Bugzilla->user->groups->{'admin'} ||
    ThrowUserError('auth_failure', {group  => "admin",
                                    action => "edit",
                                    object => "field_values"});

#
# often-used variables
#
my $field   = trim($cgi->param('field')   || '');
my $value   = trim($cgi->param('value')   || '');
my $sortkey = trim($cgi->param('sortkey') || '0');
my $action  = trim($cgi->param('action')  || '');
my $token   = $cgi->param('token');

# Gives the name of the parameter associated with the field
# and representing its default value.
my %defaults;
$defaults{'op_sys'} = 'defaultopsys';
$defaults{'rep_platform'} = 'defaultplatform';
$defaults{'priority'} = 'defaultpriority';
$defaults{'bug_severity'} = 'defaultseverity';

# Alternatively, a list of non-editable values can be specified.
# In this case, only the sortkey can be altered.
my %static;
$static{'resolution'} = ['', 'FIXED', 'MOVED', 'DUPLICATE'];
$static{$_->name} = ['---'] foreach (@custom_fields);

#
# field = '' -> Show nice list of fields
#
unless ($field) {
    # Convert @valid_fields into the format that select-field wants.
    my @field_list = ();
    foreach my $field_name (@valid_fields) {
        push(@field_list, {name => $field_name});
    }

    $vars->{'fields'} = \@field_list;
    $template->process("admin/fieldvalues/select-field.html.tmpl",
                       $vars)
      || ThrowTemplateError($template->error());
    exit;
}


#
# action='' -> Show nice list of values.
#
unless ($action) {
    FieldMustExist($field);
    # Now we know the $field is valid.
    trick_taint($field);

    my $fieldvalues = 
        $dbh->selectall_arrayref("SELECT value AS name, sortkey"
                               . "  FROM $field ORDER BY sortkey, value",
                                 {Slice =>{}});
    $vars->{'field'} = $field;
    $vars->{'values'} = $fieldvalues;
    $vars->{'default'} = Bugzilla->params->{$defaults{$field}} if defined $defaults{$field};
    $vars->{'static'} = $static{$field} if exists $static{$field};
    $template->process("admin/fieldvalues/list.html.tmpl", $vars)
      || ThrowTemplateError($template->error());

    exit;
}


#
# action='add' -> show form for adding new field value.
# (next action will be 'new')
#
if ($action eq 'add') {
    FieldMustExist($field);

    $vars->{'value'} = $value;
    $vars->{'field'} = $field;
    $vars->{'token'} = issue_session_token('add_field_value');
    $template->process("admin/fieldvalues/create.html.tmpl",
                       $vars)
      || ThrowTemplateError($template->error());

    exit;
}


#
# action='new' -> add field value entered in the 'action=add' screen
#
if ($action eq 'new') {
    check_token_data($token, 'add_field_value');
    FieldMustExist($field);
    trick_taint($field);

    # Cleanups and validity checks
    $value || ThrowUserError('fieldvalue_undefined');

    if (length($value) > 60) {
        ThrowUserError('fieldvalue_name_too_long',
                       {'value' => $value});
    }
    # Need to store in case detaint_natural() clears the sortkey
    my $stored_sortkey = $sortkey;
    if (!detaint_natural($sortkey)) {
        ThrowUserError('fieldvalue_sortkey_invalid',
                       {'name' => $field,
                        'sortkey' => $stored_sortkey});
    }
    if (ValueExists($field, $value)) {
        ThrowUserError('fieldvalue_already_exists',
                       {'field' => $field,
                        'value' => $value});
    }
    # Value is only used in a SELECT placeholder and through the HTML filter.
    trick_taint($value);

    # Add the new field value.
    my $sth = $dbh->prepare("INSERT INTO $field ( value, sortkey )
                             VALUES ( ?, ? )");
    $sth->execute($value, $sortkey);

    delete_token($token);

    $vars->{'value'} = $value;
    $vars->{'field'} = $field;
    $template->process("admin/fieldvalues/created.html.tmpl",
                       $vars)
      || ThrowTemplateError($template->error());

    exit;
}


#
# action='del' -> ask if user really wants to delete
# (next action would be 'delete')
#
if ($action eq 'del') {
    ValueMustExist($field, $value);
    trick_taint($field);
    trick_taint($value);

    # See if any bugs are still using this value.
    $vars->{'bug_count'} = 
        $dbh->selectrow_array("SELECT COUNT(*) FROM bugs WHERE $field = ?",
                              undef, $value) || 0;
    $vars->{'value_count'} = 
        $dbh->selectrow_array("SELECT COUNT(*) FROM $field");

    $vars->{'value'} = $value;
    $vars->{'field'} = $field;
    $vars->{'param_name'} = $defaults{$field};

    # If the value cannot be deleted, throw an error.
    if (lsearch($static{$field}, $value) >= 0) {
        ThrowUserError('fieldvalue_not_deletable', $vars);
    }
    $vars->{'token'} = issue_session_token('delete_field_value');

    $template->process("admin/fieldvalues/confirm-delete.html.tmpl",
                       $vars)
      || ThrowTemplateError($template->error());

    exit;
}


#
# action='delete' -> really delete the field value
#
if ($action eq 'delete') {
    check_token_data($token, 'delete_field_value');
    ValueMustExist($field, $value);

    $vars->{'value'} = $value;
    $vars->{'field'} = $field;
    $vars->{'param_name'} = $defaults{$field};

    if (defined $defaults{$field}
        && ($value eq Bugzilla->params->{$defaults{$field}}))
    {
        ThrowUserError('fieldvalue_is_default', $vars);
    }
    # If the value cannot be deleted, throw an error.
    if (lsearch($static{$field}, $value) >= 0) {
        ThrowUserError('fieldvalue_not_deletable', $vars);
    }

    trick_taint($field);
    trick_taint($value);

    $dbh->bz_lock_tables('bugs READ', "$field WRITE");

    # Check if there are any bugs that still have this value.
    my $bug_ids = $dbh->selectcol_arrayref(
        "SELECT bug_id FROM bugs WHERE $field = ?", undef, $value);

    if (scalar(@$bug_ids)) {
        # You tried to delete a field that bugs are still using.
        # You can't just delete the bugs. That's ridiculous. 
        ThrowUserError("fieldvalue_still_has_bugs", 
                       { field => $field, value => $value,
                         count => scalar(@$bug_ids) });
    }

    $dbh->do("DELETE FROM $field WHERE value = ?", undef, $value);

    $dbh->bz_unlock_tables();
    delete_token($token);

    $template->process("admin/fieldvalues/deleted.html.tmpl",
                       $vars)
      || ThrowTemplateError($template->error());
    exit;
}


#
# action='edit' -> present the edit-value form
# (next action would be 'update')
#
if ($action eq 'edit') {
    ValueMustExist($field, $value);
    trick_taint($field);
    trick_taint($value);

    $vars->{'sortkey'} = $dbh->selectrow_array(
        "SELECT sortkey FROM $field WHERE value = ?", undef, $value) || 0;

    $vars->{'value'} = $value;
    $vars->{'field'} = $field;
    $vars->{'is_static'} = (lsearch($static{$field}, $value) >= 0) ? 1 : 0;
    $vars->{'token'} = issue_session_token('edit_field_value');

    $template->process("admin/fieldvalues/edit.html.tmpl", $vars)
      || ThrowTemplateError($template->error());

    exit;
}


#
# action='update' -> update the field value
#
if ($action eq 'update') {
    check_token_data($token, 'edit_field_value');
    my $valueold   = trim($cgi->param('valueold')   || '');
    my $sortkeyold = trim($cgi->param('sortkeyold') || '0');

    ValueMustExist($field, $valueold);
    trick_taint($field);
    trick_taint($valueold);

    $vars->{'value'} = $value;
    $vars->{'field'} = $field;

    # If the value cannot be renamed, throw an error.
    if (lsearch($static{$field}, $valueold) >= 0 && $value ne $valueold) {
        $vars->{'old_value'} = $valueold;
        ThrowUserError('fieldvalue_not_editable', $vars);
    }

    if (length($value) > 60) {
        ThrowUserError('fieldvalue_name_too_long', $vars);
    }

    $dbh->bz_lock_tables('bugs WRITE', "$field WRITE");

    # Need to store because detaint_natural() will delete this if
    # invalid
    my $stored_sortkey = $sortkey;
    if ($sortkey != $sortkeyold) {

        if (!detaint_natural($sortkey)) {
            ThrowUserError('fieldvalue_sortkey_invalid',
                           {'name' => $field,
                            'sortkey' => $stored_sortkey});

        }

        $dbh->do("UPDATE $field SET sortkey = ? WHERE value = ?",
                 undef, $sortkey, $valueold);

        $vars->{'updated_sortkey'} = 1;
        $vars->{'sortkey'} = $sortkey;
    }

    if ($value ne $valueold) {

        unless ($value) {
            ThrowUserError('fieldvalue_undefined');
        }
        if (ValueExists($field, $value)) {
            ThrowUserError('fieldvalue_already_exists', $vars);
        }
        trick_taint($value);

        $dbh->do("UPDATE bugs SET $field = ? WHERE $field = ?",
                 undef, $value, $valueold);

        $dbh->do("UPDATE $field SET value = ? WHERE value = ?",
                 undef, $value, $valueold);

        $vars->{'updated_value'} = 1;
    }

    $dbh->bz_unlock_tables();

    # If the old value was the default value for the field,
    # update data/params accordingly.
    # This update is done while tables are unlocked due to the
    # annoying calls in Bugzilla/Config/Common.pm.
    if (defined $defaults{$field}
        && $value ne $valueold
        && $valueold eq Bugzilla->params->{$defaults{$field}})
    {
        SetParam($defaults{$field}, $value);
        write_params();
        $vars->{'default_value_updated'} = 1;
    }
    delete_token($token);

    $template->process("admin/fieldvalues/updated.html.tmpl",
                       $vars)
      || ThrowTemplateError($template->error());

    exit;
}


#
# No valid action found
#
# We can't get here without $field being defined --
# See the unless($field) block at the top.
ThrowUserError('no_valid_action', { field => $field } );
