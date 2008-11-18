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
# The Original Code is mozilla.org code.
#
# The Initial Developer of the Original Code is Holger
# Schurig. Portions created by Holger Schurig are
# Copyright (C) 1999 Holger Schurig. All
# Rights Reserved.
#
# Contributor(s): Holger Schurig <holgerschurig@nikocity.de>
#                 Terry Weissman <terry@mozilla.org>
#                 Frédéric Buclin <LpSolit@gmail.com>
#                 Akamai Technologies <bugzilla-dev@akamai.com>
#
# Direct any questions on this source code to
#
# Holger Schurig <holgerschurig@nikocity.de>

use strict;
use lib ".";

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Series;
use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::User;
use Bugzilla::Component;
use Bugzilla::Bug;
use Bugzilla::Token;

###############
# Subroutines #
###############

# Takes an arrayref of login names and returns an arrayref of user ids.
sub check_initial_cc {
    my ($user_names) = @_;

    my %cc_ids;
    foreach my $cc (@$user_names) {
        my $id = login_to_id($cc, THROW_ERROR);
        $cc_ids{$id} = 1;
    }
    return [keys %cc_ids];
}

###############
# Main Script #
###############

my $cgi = Bugzilla->cgi;
my $dbh = Bugzilla->dbh;
my $template = Bugzilla->template;
my $vars = {};

#
# Preliminary checks:
#

my $user = Bugzilla->login(LOGIN_REQUIRED);
my $whoid = $user->id;

print $cgi->header();

$user->in_group('editcomponents')
  || scalar(@{$user->get_products_by_permission('editcomponents')})
  || ThrowUserError("auth_failure", {group  => "editcomponents",
                                     action => "edit",
                                     object => "components"});

#
# often used variables
#
my $product_name  = trim($cgi->param('product')     || '');
my $comp_name     = trim($cgi->param('component')   || '');
my $action        = trim($cgi->param('action')      || '');
my $showbugcounts = (defined $cgi->param('showbugcounts'));
my $token         = $cgi->param('token');

#
# product = '' -> Show nice list of products
#

unless ($product_name) {
    my $selectable_products = $user->get_selectable_products;
    # If the user has editcomponents privs for some products only,
    # we have to restrict the list of products to display.
    unless ($user->in_group('editcomponents')) {
        $selectable_products = $user->get_products_by_permission('editcomponents');
    }
    $vars->{'products'} = $selectable_products;
    $vars->{'showbugcounts'} = $showbugcounts;

    $template->process("admin/components/select-product.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
    exit;
}

my $product = $user->check_can_admin_product($product_name);

#
# action='' -> Show nice list of components
#

unless ($action) {

    $vars->{'showbugcounts'} = $showbugcounts;
    $vars->{'product'} = $product;
    $template->process("admin/components/list.html.tmpl", $vars)
        || ThrowTemplateError($template->error());

    exit;
}


#
# action='add' -> present form for parameters for new component
#
# (next action will be 'new')
#

if ($action eq 'add') {
    $vars->{'token'} = issue_session_token('add_component');
    $vars->{'product'} = $product;
    $template->process("admin/components/create.html.tmpl", $vars)
        || ThrowTemplateError($template->error());

    exit;
}



#
# action='new' -> add component entered in the 'action=add' screen
#

if ($action eq 'new') {
    check_token_data($token, 'add_component');
    # Do the user matching
    Bugzilla::User::match_field ($cgi, {
        'initialowner'     => { 'type' => 'single' },
        'initialqacontact' => { 'type' => 'single' },
        'initialcc'        => { 'type' => 'multi'  },
    });

    my $default_assignee   = trim($cgi->param('initialowner')     || '');
    my $default_qa_contact = trim($cgi->param('initialqacontact') || '');
    my $description        = trim($cgi->param('description')      || '');
    my @initial_cc         = $cgi->param('initialcc');

    $comp_name || ThrowUserError('component_blank_name');

    if (length($comp_name) > 64) {
        ThrowUserError('component_name_too_long',
                       {'name' => $comp_name});
    }

    my $component =
        new Bugzilla::Component({product => $product,
                                 name => $comp_name});

    if ($component) {
        ThrowUserError('component_already_exists',
                       {'name' => $component->name});
    }

    $description || ThrowUserError('component_blank_description',
                                   {name => $comp_name});

    $default_assignee || ThrowUserError('component_need_initialowner',
                                        {name => $comp_name});

    my $default_assignee_id   = login_to_id($default_assignee);
    my $default_qa_contact_id = Bugzilla->params->{'useqacontact'} ?
        (login_to_id($default_qa_contact) || undef) : undef;

    my $initial_cc_ids = check_initial_cc(\@initial_cc);

    trick_taint($comp_name);
    trick_taint($description);

    $dbh->bz_lock_tables('components WRITE', 'component_cc WRITE');

    $dbh->do("INSERT INTO components
                (product_id, name, description, initialowner,
                 initialqacontact)
              VALUES (?, ?, ?, ?, ?)", undef,
             ($product->id, $comp_name, $description,
              $default_assignee_id, $default_qa_contact_id));

    $component = new Bugzilla::Component({ product => $product,
                                           name => $comp_name });

    my $sth = $dbh->prepare("INSERT INTO component_cc 
                             (user_id, component_id) VALUES (?, ?)");
    foreach my $user_id (@$initial_cc_ids) {
        $sth->execute($user_id, $component->id);
    }

    $dbh->bz_unlock_tables;

    # Insert default charting queries for this product.
    # If they aren't using charting, this won't do any harm.
    my @series;

    my $prodcomp = "&product="   . url_quote($product->name) .
                   "&component=" . url_quote($comp_name);

    # For localization reasons, we get the title of the queries from the
    # submitted form.
    my $open_name = $cgi->param('open_name');
    my $nonopen_name = $cgi->param('nonopen_name');
    my $open_query = "field0-0-0=resolution&type0-0-0=notregexp&value0-0-0=." .
                     $prodcomp;
    my $nonopen_query = "field0-0-0=resolution&type0-0-0=regexp&value0-0-0=." .
                        $prodcomp;

    # trick_taint is ok here, as these variables aren't used as a command
    # or in SQL unquoted
    trick_taint($open_name);
    trick_taint($nonopen_name);
    trick_taint($open_query);
    trick_taint($nonopen_query);

    push(@series, [$open_name, $open_query]);
    push(@series, [$nonopen_name, $nonopen_query]);

    foreach my $sdata (@series) {
        my $series = new Bugzilla::Series(undef, $product->name,
                                          $comp_name, $sdata->[0],
                                          $whoid, 1, $sdata->[1], 1);
        $series->writeToDatabase();
    }

    $vars->{'comp'} = $component;
    $vars->{'product'} = $product;
    delete_token($token);

    $template->process("admin/components/created.html.tmpl",
                       $vars)
      || ThrowTemplateError($template->error());

    exit;
}



#
# action='del' -> ask if user really wants to delete
#
# (next action would be 'delete')
#

if ($action eq 'del') {
    $vars->{'token'} = issue_session_token('delete_component');
    $vars->{'comp'} =
        Bugzilla::Component::check_component($product, $comp_name);

    $vars->{'product'} = $product;

    $template->process("admin/components/confirm-delete.html.tmpl", $vars)
        || ThrowTemplateError($template->error());

    exit;
}



#
# action='delete' -> really delete the component
#

if ($action eq 'delete') {
    check_token_data($token, 'delete_component');
    my $component =
        Bugzilla::Component::check_component($product, $comp_name);

    if ($component->bug_count) {
        if (Bugzilla->params->{"allowbugdeletion"}) {
            foreach my $bug_id (@{$component->bug_ids}) {
                # Note: We allow admins to delete bugs even if they can't
                # see them, as long as they can see the product.
                my $bug = new Bugzilla::Bug($bug_id);
                $bug->remove_from_db();
            }
        } else {
            ThrowUserError("component_has_bugs",
                           {nb => $component->bug_count });
        }
    }
    
    $dbh->bz_lock_tables('components WRITE', 'component_cc WRITE',
                         'flaginclusions WRITE', 'flagexclusions WRITE');

    $dbh->do("DELETE FROM flaginclusions WHERE component_id = ?",
             undef, $component->id);
    $dbh->do("DELETE FROM flagexclusions WHERE component_id = ?",
             undef, $component->id);
    $dbh->do("DELETE FROM component_cc WHERE component_id = ?",
             undef, $component->id);
    $dbh->do("DELETE FROM components WHERE id = ?",
             undef, $component->id);

    $dbh->bz_unlock_tables();

    $vars->{'comp'} = $component;
    $vars->{'product'} = $product;
    delete_token($token);

    $template->process("admin/components/deleted.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
    exit;
}



#
# action='edit' -> present the edit component form
#
# (next action would be 'update')
#

if ($action eq 'edit') {
    $vars->{'token'} = issue_session_token('edit_component');
    my $component =
        Bugzilla::Component::check_component($product, $comp_name);
    $vars->{'comp'} = $component;

    $vars->{'initial_cc_names'} = 
        join(', ', map($_->login, @{$component->initial_cc}));

    $vars->{'product'} = $product;

    $template->process("admin/components/edit.html.tmpl",
                       $vars)
      || ThrowTemplateError($template->error());

    exit;
}



#
# action='update' -> update the component
#

if ($action eq 'update') {
    check_token_data($token, 'edit_component');
    # Do the user matching
    Bugzilla::User::match_field ($cgi, {
        'initialowner'     => { 'type' => 'single' },
        'initialqacontact' => { 'type' => 'single' },
        'initialcc'        => { 'type' => 'multi'  },
    });

    my $comp_old_name         = trim($cgi->param('componentold')     || '');
    my $default_assignee      = trim($cgi->param('initialowner')     || '');
    my $default_qa_contact    = trim($cgi->param('initialqacontact') || '');
    my $description           = trim($cgi->param('description')      || '');
    my @initial_cc            = $cgi->param('initialcc');

    my $component_old =
        Bugzilla::Component::check_component($product, $comp_old_name);

    $comp_name || ThrowUserError('component_blank_name');

    if (length($comp_name) > 64) {
        ThrowUserError('component_name_too_long',
                       {'name' => $comp_name});
    }

    if ($comp_name ne $component_old->name) {
        my $component =
            new Bugzilla::Component({product => $product,
                                     name => $comp_name});
        if ($component) {
            ThrowUserError('component_already_exists',
                           {'name' => $component->name});
        }
    }

    $description || ThrowUserError('component_blank_description',
                                   {'name' => $component_old->name});

    $default_assignee || ThrowUserError('component_need_initialowner',
                                        {name => $comp_name});

    my $default_assignee_id   = login_to_id($default_assignee);
    my $default_qa_contact_id = login_to_id($default_qa_contact) || undef;

    my $initial_cc_ids = check_initial_cc(\@initial_cc);

    $dbh->bz_lock_tables('components WRITE', 'component_cc WRITE', 
                         'profiles READ');

    if ($comp_name ne $component_old->name) {

        trick_taint($comp_name);
        $dbh->do("UPDATE components SET name = ? WHERE id = ?",
                 undef, ($comp_name, $component_old->id));

        $vars->{'updated_name'} = 1;

    }

    if ($description ne $component_old->description) {
    
        trick_taint($description);
        $dbh->do("UPDATE components SET description = ? WHERE id = ?",
                 undef, ($description, $component_old->id));

        $vars->{'updated_description'} = 1;
    }

    if ($default_assignee ne $component_old->default_assignee->login) {

        $dbh->do("UPDATE components SET initialowner = ? WHERE id = ?",
                 undef, ($default_assignee_id, $component_old->id));

        $vars->{'updated_initialowner'} = 1;
    }

    if (Bugzilla->params->{'useqacontact'}
        && $default_qa_contact ne $component_old->default_qa_contact->login) {
        $dbh->do("UPDATE components SET initialqacontact = ?
                  WHERE id = ?", undef,
                 ($default_qa_contact_id, $component_old->id));

        $vars->{'updated_initialqacontact'} = 1;
    }

    my @initial_cc_old = map($_->id, @{$component_old->initial_cc});
    my ($removed, $added) = diff_arrays(\@initial_cc_old, $initial_cc_ids);

    foreach my $user_id (@$removed) {
        $dbh->do('DELETE FROM component_cc 
                   WHERE component_id = ? AND user_id = ?', undef,
                 $component_old->id, $user_id);
        $vars->{'updated_initialcc'} = 1;
    }

    foreach my $user_id (@$added) {
        $dbh->do("INSERT INTO component_cc (user_id, component_id) 
                       VALUES (?, ?)", undef, $user_id, $component_old->id);
        $vars->{'updated_initialcc'} = 1;
    }

    $dbh->bz_unlock_tables();

    my $component = new Bugzilla::Component($component_old->id);
    
    $vars->{'comp'} = $component;
    $vars->{'initial_cc_names'} = 
        join(', ', map($_->login, @{$component->initial_cc}));
    $vars->{'product'} = $product;
    delete_token($token);

    $template->process("admin/components/updated.html.tmpl",
                       $vars)
      || ThrowTemplateError($template->error());

    exit;
}

#
# No valid action found
#
ThrowUserError('no_valid_action', {'field' => "component"});
