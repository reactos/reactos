#!/usr/bin/perl -wT
# -*- Mode: perl; indent-tabs-mode: nil -*-

#
# This is a script to edit the target milestones. It is largely a copy of
# the editversions.cgi script, since the two fields were set up in a
# very similar fashion.
#
# (basically replace each occurrence of 'milestone' with 'version', and
# you'll have the original script)
#
# Matt Masson <matthew@zeroknowledge.com>
#
# Contributors : Gavin Shelley <bugzilla@chimpychompy.org>
#                Frédéric Buclin <LpSolit@gmail.com>
#


use strict;
use lib ".";

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::Milestone;
use Bugzilla::Bug;
use Bugzilla::Token;

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
                                     object => "milestones"});

#
# often used variables
#
my $product_name   = trim($cgi->param('product')     || '');
my $milestone_name = trim($cgi->param('milestone')   || '');
my $sortkey        = trim($cgi->param('sortkey')     || 0);
my $action         = trim($cgi->param('action')      || '');
my $showbugcounts = (defined $cgi->param('showbugcounts'));
my $token          = $cgi->param('token');

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

    $template->process("admin/milestones/select-product.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
    exit;
}

my $product = $user->check_can_admin_product($product_name);

#
# action='' -> Show nice list of milestones
#

unless ($action) {

    $vars->{'showbugcounts'} = $showbugcounts;
    $vars->{'product'} = $product;
    $template->process("admin/milestones/list.html.tmpl",
                       $vars)
      || ThrowTemplateError($template->error());

    exit;
}




#
# action='add' -> present form for parameters for new milestone
#
# (next action will be 'new')
#

if ($action eq 'add') {
    $vars->{'token'} = issue_session_token('add_milestone');
    $vars->{'product'} = $product;
    $template->process("admin/milestones/create.html.tmpl",
                       $vars)
      || ThrowTemplateError($template->error());

    exit;
}



#
# action='new' -> add milestone entered in the 'action=add' screen
#

if ($action eq 'new') {
    check_token_data($token, 'add_milestone');
    $milestone_name || ThrowUserError('milestone_blank_name');

    if (length($milestone_name) > 20) {
        ThrowUserError('milestone_name_too_long',
                       {'name' => $milestone_name});
    }

    $sortkey = Bugzilla::Milestone::check_sort_key($milestone_name,
                                                   $sortkey);

    my $milestone = new Bugzilla::Milestone(
        { product => $product, name => $milestone_name });

    if ($milestone) {
        ThrowUserError('milestone_already_exists',
                       {'name' => $milestone->name,
                        'product' => $product->name});
    }

    # Add the new milestone
    trick_taint($milestone_name);
    $dbh->do('INSERT INTO milestones ( value, product_id, sortkey )
              VALUES ( ?, ?, ? )',
             undef, $milestone_name, $product->id, $sortkey);

    $milestone = new Bugzilla::Milestone(
        { product => $product, name => $milestone_name });
    delete_token($token);

    $vars->{'milestone'} = $milestone;
    $vars->{'product'} = $product;
    $template->process("admin/milestones/created.html.tmpl",
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
    my $milestone = Bugzilla::Milestone::check_milestone($product,
                                                         $milestone_name);
    
    $vars->{'milestone'} = $milestone;
    $vars->{'product'} = $product;

    # The default milestone cannot be deleted.
    if ($product->default_milestone eq $milestone->name) {
        ThrowUserError("milestone_is_default", $vars);
    }
    $vars->{'token'} = issue_session_token('delete_milestone');

    $template->process("admin/milestones/confirm-delete.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
    exit;
}



#
# action='delete' -> really delete the milestone
#

if ($action eq 'delete') {
    check_token_data($token, 'delete_milestone');
    my $milestone =
        Bugzilla::Milestone::check_milestone($product,
                                             $milestone_name);
    $vars->{'milestone'} = $milestone;
    $vars->{'product'} = $product;

    # The default milestone cannot be deleted.
    if ($milestone->name eq $product->default_milestone) {
        ThrowUserError("milestone_is_default", $vars);
    }

    if ($milestone->bug_count) {
        # We don't want to delete bugs when deleting a milestone.
        # Bugs concerned are reassigned to the default milestone.
        my $bug_ids =
          $dbh->selectcol_arrayref("SELECT bug_id FROM bugs
                                    WHERE product_id = ? AND target_milestone = ?",
                                    undef, ($product->id, $milestone->name));
        my $timestamp = $dbh->selectrow_array("SELECT NOW()");
        foreach my $bug_id (@$bug_ids) {
            $dbh->do("UPDATE bugs SET target_milestone = ?,
                      delta_ts = ? WHERE bug_id = ?",
                      undef, ($product->default_milestone, $timestamp,
                              $bug_id));
            # We have to update the 'bugs_activity' table too.
            LogActivityEntry($bug_id, 'target_milestone',
                             $milestone->name,
                             $product->default_milestone,
                             $whoid, $timestamp);
        }
    }

    $dbh->do("DELETE FROM milestones WHERE product_id = ? AND value = ?",
             undef, ($product->id, $milestone->name));

    delete_token($token);

    $template->process("admin/milestones/deleted.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
    exit;
}



#
# action='edit' -> present the edit milestone form
#
# (next action would be 'update')
#

if ($action eq 'edit') {

    my $milestone =
        Bugzilla::Milestone::check_milestone($product,
                                             $milestone_name);

    $vars->{'milestone'} = $milestone;
    $vars->{'product'} = $product;
    $vars->{'token'} = issue_session_token('edit_milestone');

    $template->process("admin/milestones/edit.html.tmpl",
                       $vars)
      || ThrowTemplateError($template->error());

    exit;
}



#
# action='update' -> update the milestone
#

if ($action eq 'update') {
    check_token_data($token, 'edit_milestone');
    my $milestone_old_name = trim($cgi->param('milestoneold') || '');
    my $milestone_old =
        Bugzilla::Milestone::check_milestone($product,
                                             $milestone_old_name);

    if (length($milestone_name) > 20) {
        ThrowUserError('milestone_name_too_long',
                       {'name' => $milestone_name});
    }

    $dbh->bz_lock_tables('bugs WRITE',
                         'milestones WRITE',
                         'products WRITE');

    if ($sortkey ne $milestone_old->sortkey) {
        $sortkey = Bugzilla::Milestone::check_sort_key($milestone_name,
                                                       $sortkey);

        $dbh->do('UPDATE milestones SET sortkey = ?
                  WHERE product_id = ?
                  AND value = ?',
                 undef,
                 $sortkey,
                 $product->id,
                 $milestone_old->name);

        $vars->{'updated_sortkey'} = 1;
    }

    if ($milestone_name ne $milestone_old->name) {
        unless ($milestone_name) {
            ThrowUserError('milestone_blank_name');
        }
        my $milestone = new Bugzilla::Milestone(
            { product => $product, name => $milestone_name });
        if ($milestone) {
            ThrowUserError('milestone_already_exists',
                           {'name' => $milestone->name,
                            'product' => $product->name});
        }

        trick_taint($milestone_name);

        $dbh->do('UPDATE bugs
                  SET target_milestone = ?
                  WHERE target_milestone = ?
                  AND product_id = ?',
                 undef,
                 $milestone_name,
                 $milestone_old->name,
                 $product->id);

        $dbh->do("UPDATE milestones
                  SET value = ?
                  WHERE product_id = ?
                  AND value = ?",
                 undef,
                 $milestone_name,
                 $product->id,
                 $milestone_old->name);

        $dbh->do("UPDATE products
                  SET defaultmilestone = ?
                  WHERE id = ?
                  AND defaultmilestone = ?",
                 undef,
                 $milestone_name,
                 $product->id,
                 $milestone_old->name);

        $vars->{'updated_name'} = 1;
    }

    $dbh->bz_unlock_tables();

    my $milestone =
        Bugzilla::Milestone::check_milestone($product,
                                             $milestone_name);
    delete_token($token);

    $vars->{'milestone'} = $milestone;
    $vars->{'product'} = $product;
    $template->process("admin/milestones/updated.html.tmpl",
                       $vars)
      || ThrowTemplateError($template->error());

    exit;
}


#
# No valid action found
#
ThrowUserError('no_valid_action', {'field' => "target_milestone"});
