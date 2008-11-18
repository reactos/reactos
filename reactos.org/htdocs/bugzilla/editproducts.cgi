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
#               Terry Weissman <terry@mozilla.org>
#               Dawn Endico <endico@mozilla.org>
#               Joe Robins <jmrobins@tgix.com>
#               Gavin Shelley <bugzilla@chimpychompy.org>
#               Frédéric Buclin <LpSolit@gmail.com>
#               Greg Hendricks <ghendricks@novell.com>
#               Lance Larsh <lance.larsh@oracle.com>
#               Elliotte Martin <elliotte.martin@yahoo.com>

use strict;
use lib ".";

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::Bug;
use Bugzilla::Series;
use Bugzilla::Mailer;
use Bugzilla::Product;
use Bugzilla::Classification;
use Bugzilla::Milestone;
use Bugzilla::Group;
use Bugzilla::User;
use Bugzilla::Field;
use Bugzilla::Token;

#
# Preliminary checks:
#

my $user = Bugzilla->login(LOGIN_REQUIRED);
my $whoid = $user->id;

my $dbh = Bugzilla->dbh;
my $cgi = Bugzilla->cgi;
my $template = Bugzilla->template;
my $vars = {};

print $cgi->header();

$user->in_group('editcomponents')
  || scalar(@{$user->get_products_by_permission('editcomponents')})
  || ThrowUserError("auth_failure", {group  => "editcomponents",
                                     action => "edit",
                                     object => "products"});

#
# often used variables
#
my $classification_name = trim($cgi->param('classification') || '');
my $product_name = trim($cgi->param('product') || '');
my $action  = trim($cgi->param('action')  || '');
my $showbugcounts = (defined $cgi->param('showbugcounts'));
my $token = $cgi->param('token');

#
# product = '' -> Show nice list of classifications (if
# classifications enabled)
#

if (Bugzilla->params->{'useclassification'} 
    && !$classification_name
    && !$product_name)
{
    $vars->{'classifications'} = $user->get_selectable_classifications;
    
    $template->process("admin/products/list-classifications.html.tmpl", $vars)
        || ThrowTemplateError($template->error());
    exit;
}


#
# action = '' -> Show a nice list of products, unless a product
#                is already specified (then edit it)
#

if (!$action && !$product_name) {
    my $classification;
    my $products;

    if (Bugzilla->params->{'useclassification'}) {
        $classification =
            Bugzilla::Classification::check_classification($classification_name);

        $products = $user->get_selectable_products($classification->id);
        $vars->{'classification'} = $classification;
    } else {
        $products = $user->get_selectable_products;
    }

    # If the user has editcomponents privs for some products only,
    # we have to restrict the list of products to display.
    unless ($user->in_group('editcomponents')) {
        $products = $user->get_products_by_permission('editcomponents');
        if (Bugzilla->params->{'useclassification'}) {
            @$products = grep {$_->classification_id == $classification->id} @$products;
        }
    }
    $vars->{'products'} = $products;
    $vars->{'showbugcounts'} = $showbugcounts;

    $template->process("admin/products/list.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
    exit;
}




#
# action='add' -> present form for parameters for new product
#
# (next action will be 'new')
#

if ($action eq 'add') {
    # The user must have the global editcomponents privs to add
    # new products.
    $user->in_group('editcomponents')
      || ThrowUserError("auth_failure", {group  => "editcomponents",
                                         action => "add",
                                         object => "products"});

    if (Bugzilla->params->{'useclassification'}) {
        my $classification = 
            Bugzilla::Classification::check_classification($classification_name);
        $vars->{'classification'} = $classification;
    }
    $vars->{'token'} = issue_session_token('add_product');

    $template->process("admin/products/create.html.tmpl", $vars)
      || ThrowTemplateError($template->error());

    exit;
}


#
# action='new' -> add product entered in the 'action=add' screen
#

if ($action eq 'new') {
    # The user must have the global editcomponents privs to add
    # new products.
    $user->in_group('editcomponents')
      || ThrowUserError("auth_failure", {group  => "editcomponents",
                                         action => "add",
                                         object => "products"});

    check_token_data($token, 'add_product');
    # Cleanups and validity checks

    my $classification_id = 1;
    if (Bugzilla->params->{'useclassification'}) {
        my $classification = 
            Bugzilla::Classification::check_classification($classification_name);
        $classification_id = $classification->id;
        $vars->{'classification'} = $classification;
    }

    unless ($product_name) {
        ThrowUserError("product_blank_name");  
    }

    my $product = new Bugzilla::Product({name => $product_name});

    if ($product) {

        # Check for exact case sensitive match:
        if ($product->name eq $product_name) {
            ThrowUserError("product_name_already_in_use",
                           {'product' => $product->name});
        }

        # Next check for a case-insensitive match:
        if (lc($product->name) eq lc($product_name)) {
            ThrowUserError("product_name_diff_in_case",
                           {'product' => $product_name,
                            'existing_product' => $product->name}); 
        }
    }

    my $version = trim($cgi->param('version') || '');

    if ($version eq '') {
        ThrowUserError("product_must_have_version",
                       {'product' => $product_name});
    }

    my $description  = trim($cgi->param('description')  || '');

    if ($description eq '') {
        ThrowUserError('product_must_have_description',
                       {'product' => $product_name});
    }

    my $milestoneurl = trim($cgi->param('milestoneurl') || '');
    my $disallownew = $cgi->param('disallownew') ? 1 : 0;
    my $votesperuser = $cgi->param('votesperuser') || 0;
    my $maxvotesperbug = defined($cgi->param('maxvotesperbug')) ?
        $cgi->param('maxvotesperbug') : 10000;
    my $votestoconfirm = $cgi->param('votestoconfirm') || 0;
    my $defaultmilestone = $cgi->param('defaultmilestone') || "---";

    # The following variables are used in placeholders only.
    trick_taint($product_name);
    trick_taint($version);
    trick_taint($description);
    trick_taint($milestoneurl);
    trick_taint($defaultmilestone);
    detaint_natural($disallownew);
    detaint_natural($votesperuser);
    detaint_natural($maxvotesperbug);
    detaint_natural($votestoconfirm);

    # Add the new product.
    $dbh->do('INSERT INTO products
              (name, description, milestoneurl, disallownew, votesperuser,
               maxvotesperbug, votestoconfirm, defaultmilestone, classification_id)
              VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)',
             undef, ($product_name, $description, $milestoneurl, $disallownew,
             $votesperuser, $maxvotesperbug, $votestoconfirm, $defaultmilestone,
             $classification_id));

    $product = new Bugzilla::Product({name => $product_name});
    
    $dbh->do('INSERT INTO versions (value, product_id) VALUES (?, ?)',
             undef, ($version, $product->id));

    $dbh->do('INSERT INTO milestones (product_id, value) VALUES (?, ?)',
             undef, ($product->id, $defaultmilestone));

    # If we're using bug groups, then we need to create a group for this
    # product as well.  -JMR, 2/16/00
    if (Bugzilla->params->{"makeproductgroups"}) {
        # Next we insert into the groups table
        my $productgroup = $product->name;
        while (new Bugzilla::Group({name => $productgroup})) {
            $productgroup .= '_';
        }
        my $group_description = "Access to bugs in the " .
                                $product->name . " product";

        $dbh->do('INSERT INTO groups (name, description, isbuggroup)
                  VALUES (?, ?, ?)',
                  undef, ($productgroup, $group_description, 1));

        my $gid = $dbh->bz_last_key('groups', 'id');

        # If we created a new group, give the "admin" group privileges
        # initially.
        my $admin = Bugzilla::Group->new({name => 'admin'})->id();
        
        my $sth = $dbh->prepare('INSERT INTO group_group_map
                                 (member_id, grantor_id, grant_type)
                                 VALUES (?, ?, ?)');

        $sth->execute($admin, $gid, GROUP_MEMBERSHIP);
        $sth->execute($admin, $gid, GROUP_BLESS);
        $sth->execute($admin, $gid, GROUP_VISIBLE);

        # Associate the new group and new product.
        $dbh->do('INSERT INTO group_control_map
                  (group_id, product_id, entry, membercontrol,
                   othercontrol, canedit)
                  VALUES (?, ?, ?, ?, ?, ?)',
                 undef, ($gid, $product->id, 
                         Bugzilla->params->{'useentrygroupdefault'},
                 CONTROLMAPDEFAULT, CONTROLMAPNA, 0));
    }

    if ($cgi->param('createseries')) {
        # Insert default charting queries for this product.
        # If they aren't using charting, this won't do any harm.
        #
        # $open_name and $product are sqlquoted by the series code 
        # and never used again here, so we can trick_taint them.
        my $open_name = $cgi->param('open_name');
        trick_taint($open_name);
    
        my @series;
    
        # We do every status, every resolution, and an "opened" one as well.
        foreach my $bug_status (@{get_legal_field_values('bug_status')}) {
            push(@series, [$bug_status, 
                           "bug_status=" . url_quote($bug_status)]);
        }

        foreach my $resolution (@{get_legal_field_values('resolution')}) {
            next if !$resolution;
            push(@series, [$resolution, "resolution=" .url_quote($resolution)]);
        }

        # For localization reasons, we get the name of the "global" subcategory
        # and the title of the "open" query from the submitted form.
        my @openedstatuses = BUG_STATE_OPEN;
        my $query = 
               join("&", map { "bug_status=" . url_quote($_) } @openedstatuses);
        push(@series, [$open_name, $query]);
    
        foreach my $sdata (@series) {
            my $series = new Bugzilla::Series(undef, $product->name, 
                            scalar $cgi->param('subcategory'),
                            $sdata->[0], $whoid, 1,
                            $sdata->[1] . "&product=" .
                            url_quote($product->name), 1);
            $series->writeToDatabase();
        }
    }
    delete_token($token);

    $vars->{'product'} = $product;

    $template->process("admin/products/created.html.tmpl", $vars)
        || ThrowTemplateError($template->error());
    exit;
}

#
# action='del' -> ask if user really wants to delete
#
# (next action would be 'delete')
#

if ($action eq 'del') {
    my $product = $user->check_can_admin_product($product_name);

    if (Bugzilla->params->{'useclassification'}) {
        my $classification = 
            Bugzilla::Classification::check_classification($classification_name);
        if ($classification->id != $product->classification_id) {
            ThrowUserError('classification_doesnt_exist_for_product',
                           { product => $product->name,
                             classification => $classification->name });
        }
        $vars->{'classification'} = $classification;
    }

    $vars->{'product'} = $product;
    $vars->{'token'} = issue_session_token('delete_product');

    $template->process("admin/products/confirm-delete.html.tmpl", $vars)
        || ThrowTemplateError($template->error());
    exit;
}

#
# action='delete' -> really delete the product
#

if ($action eq 'delete') {
    my $product = $user->check_can_admin_product($product_name);
    check_token_data($token, 'delete_product');

    $vars->{'product'} = $product;

    if (Bugzilla->params->{'useclassification'}) {
        my $classification = 
            Bugzilla::Classification::check_classification($classification_name);
        if ($classification->id != $product->classification_id) {
            ThrowUserError('classification_doesnt_exist_for_product',
                           { product => $product->name,
                             classification => $classification->name });
        }
        $vars->{'classification'} = $classification;
    }

    if ($product->bug_count) {
        if (Bugzilla->params->{"allowbugdeletion"}) {
            foreach my $bug_id (@{$product->bug_ids}) {
                # Note that we allow the user to delete bugs he can't see,
                # which is okay, because he's deleting the whole Product.
                my $bug = new Bugzilla::Bug($bug_id);
                $bug->remove_from_db();
            }
        }
        else {
            ThrowUserError("product_has_bugs", 
                           { nb => $product->bug_count });
        }
    }

    $dbh->bz_lock_tables('products WRITE', 'components WRITE',
                         'versions WRITE', 'milestones WRITE',
                         'group_control_map WRITE', 'component_cc WRITE',
                         'flaginclusions WRITE', 'flagexclusions WRITE');

    my $comp_ids = $dbh->selectcol_arrayref('SELECT id FROM components
                                             WHERE product_id = ?',
                                             undef, $product->id);

    $dbh->do('DELETE FROM component_cc WHERE component_id IN
              (' . join(',', @$comp_ids) . ')') if scalar(@$comp_ids);

    $dbh->do("DELETE FROM components WHERE product_id = ?",
             undef, $product->id);

    $dbh->do("DELETE FROM versions WHERE product_id = ?",
             undef, $product->id);

    $dbh->do("DELETE FROM milestones WHERE product_id = ?",
             undef, $product->id);

    $dbh->do("DELETE FROM group_control_map WHERE product_id = ?",
             undef, $product->id);

    $dbh->do("DELETE FROM flaginclusions WHERE product_id = ?",
             undef, $product->id);
             
    $dbh->do("DELETE FROM flagexclusions WHERE product_id = ?",
             undef, $product->id);
             
    $dbh->do("DELETE FROM products WHERE id = ?",
             undef, $product->id);

    $dbh->bz_unlock_tables();

    delete_token($token);

    $template->process("admin/products/deleted.html.tmpl", $vars)
        || ThrowTemplateError($template->error());
    exit;
}

#
# action='edit' -> present the 'edit product' form
# If a product is given with no action associated with it, then edit it.
#
# (next action would be 'update')
#

if ($action eq 'edit' || (!$action && $product_name)) {
    my $product = $user->check_can_admin_product($product_name);

    if (Bugzilla->params->{'useclassification'}) {
        my $classification; 
        if (!$classification_name) {
            $classification = 
                new Bugzilla::Classification($product->classification_id);
        } else {
            $classification = 
                Bugzilla::Classification::check_classification($classification_name);
            if ($classification->id != $product->classification_id) {
                ThrowUserError('classification_doesnt_exist_for_product',
                               { product => $product->name,
                                 classification => $classification->name });
            }
        }
        $vars->{'classification'} = $classification;
    }
    my $group_controls = $product->group_controls;
        
    # Convert Group Controls(membercontrol and othercontrol) from 
    # integer to string to display Membercontrol/Othercontrol names
    # at the template. <gabriel@async.com.br>
    my $constants = {
        (CONTROLMAPNA) => 'NA',
        (CONTROLMAPSHOWN) => 'Shown',
        (CONTROLMAPDEFAULT) => 'Default',
        (CONTROLMAPMANDATORY) => 'Mandatory'};

    foreach my $group (keys(%$group_controls)) {
        foreach my $control ('membercontrol', 'othercontrol') {
            $group_controls->{$group}->{$control} = 
                $constants->{$group_controls->{$group}->{$control}};
        }
    }
    $vars->{'group_controls'} = $group_controls;
    $vars->{'product'} = $product;
    $vars->{'token'} = issue_session_token('edit_product');

    $template->process("admin/products/edit.html.tmpl", $vars)
        || ThrowTemplateError($template->error());

    exit;
}

#
# action='updategroupcontrols' -> update the product
#

if ($action eq 'updategroupcontrols') {
    my $product = $user->check_can_admin_product($product_name);
    check_token_data($token, 'edit_group_controls');

    my @now_na = ();
    my @now_mandatory = ();
    foreach my $f ($cgi->param()) {
        if ($f =~ /^membercontrol_(\d+)$/) {
            my $id = $1;
            if ($cgi->param($f) == CONTROLMAPNA) {
                push @now_na,$id;
            } elsif ($cgi->param($f) == CONTROLMAPMANDATORY) {
                push @now_mandatory,$id;
            }
        }
    }
    if (!defined $cgi->param('confirmed')) {
        my $na_groups;
        if (@now_na) {
            $na_groups = $dbh->selectall_arrayref(
                    'SELECT groups.name, COUNT(bugs.bug_id) AS count
                       FROM bugs
                 INNER JOIN bug_group_map
                         ON bug_group_map.bug_id = bugs.bug_id
                 INNER JOIN groups
                         ON bug_group_map.group_id = groups.id
                      WHERE groups.id IN (' . join(', ', @now_na) . ')
                        AND bugs.product_id = ? ' .
                       $dbh->sql_group_by('groups.name'),
                   {'Slice' => {}}, $product->id);
        }

#
# return the mandatory groups which need to have bug entries added to the bug_group_map
# and the corresponding bug count
#
        my $mandatory_groups;
        if (@now_mandatory) {
            $mandatory_groups = $dbh->selectall_arrayref(
                    'SELECT groups.name,
                           (SELECT COUNT(bugs.bug_id)
                              FROM bugs
                             WHERE bugs.product_id = ?
                               AND bugs.bug_id NOT IN
                                (SELECT bug_group_map.bug_id FROM bug_group_map
                                  WHERE bug_group_map.group_id = groups.id))
                           AS count
                      FROM groups
                     WHERE groups.id IN (' . join(', ', @now_mandatory) . ')
                     ORDER BY groups.name',
                   {'Slice' => {}}, $product->id);
            # remove zero counts
            @$mandatory_groups = grep { $_->{count} } @$mandatory_groups;

        }
        if (($na_groups && scalar(@$na_groups))
            || ($mandatory_groups && scalar(@$mandatory_groups)))
        {
            $vars->{'product'} = $product;
            $vars->{'na_groups'} = $na_groups;
            $vars->{'mandatory_groups'} = $mandatory_groups;
            $template->process("admin/products/groupcontrol/confirm-edit.html.tmpl", $vars)
                || ThrowTemplateError($template->error());
            exit;                
        }
    }

    my $groups = $dbh->selectall_arrayref('SELECT id, name FROM groups
                                           WHERE isbuggroup != 0
                                           AND isactive != 0');
    foreach my $group (@$groups) {
        my ($groupid, $groupname) = @$group;
        my $newmembercontrol = $cgi->param("membercontrol_$groupid") || 0;
        my $newothercontrol = $cgi->param("othercontrol_$groupid") || 0;
        #  Legality of control combination is a function of
        #  membercontrol\othercontrol
        #                 NA SH DE MA
        #              NA  +  -  -  -
        #              SH  +  +  +  +
        #              DE  +  -  +  +
        #              MA  -  -  -  +
        unless (($newmembercontrol == $newothercontrol)
              || ($newmembercontrol == CONTROLMAPSHOWN)
              || (($newmembercontrol == CONTROLMAPDEFAULT)
               && ($newothercontrol != CONTROLMAPSHOWN))) {
            ThrowUserError('illegal_group_control_combination',
                            {groupname => $groupname});
        }
    }
    $dbh->bz_lock_tables('groups READ',
                         'group_control_map WRITE',
                         'bugs WRITE',
                         'bugs_activity WRITE',
                         'bug_group_map WRITE',
                         'fielddefs READ');

    my $sth_Insert = $dbh->prepare('INSERT INTO group_control_map
                                    (group_id, product_id, entry, membercontrol,
                                     othercontrol, canedit, editcomponents,
                                     canconfirm, editbugs)
                                    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)');

    my $sth_Update = $dbh->prepare('UPDATE group_control_map
                                       SET entry = ?, membercontrol = ?,
                                           othercontrol = ?, canedit = ?,
                                           editcomponents = ?, canconfirm = ?,
                                           editbugs = ?
                                     WHERE group_id = ? AND product_id = ?');

    my $sth_Delete = $dbh->prepare('DELETE FROM group_control_map
                                     WHERE group_id = ? AND product_id = ?');

    $groups = $dbh->selectall_arrayref('SELECT id, name, entry, membercontrol,
                                               othercontrol, canedit,
                                               editcomponents, canconfirm, editbugs
                                          FROM groups
                                     LEFT JOIN group_control_map
                                            ON group_control_map.group_id = id
                                           AND product_id = ?
                                         WHERE isbuggroup != 0
                                           AND isactive != 0',
                                         undef, $product->id);

    foreach my $group (@$groups) {
        my ($groupid, $groupname, $entry, $membercontrol, $othercontrol,
            $canedit, $editcomponents, $canconfirm, $editbugs) = @$group;
        my $newentry = $cgi->param("entry_$groupid") || 0;
        my $newmembercontrol = $cgi->param("membercontrol_$groupid") || 0;
        my $newothercontrol = $cgi->param("othercontrol_$groupid") || 0;
        my $newcanedit = $cgi->param("canedit_$groupid") || 0;
        my $new_editcomponents = $cgi->param("editcomponents_$groupid") || 0;
        my $new_canconfirm = $cgi->param("canconfirm_$groupid") || 0;
        my $new_editbugs = $cgi->param("editbugs_$groupid") || 0;

        my $oldentry = $entry;
        # Set undefined values to 0.
        $entry ||= 0;
        $membercontrol ||= 0;
        $othercontrol ||= 0;
        $canedit ||= 0;
        $editcomponents ||= 0;
        $canconfirm ||= 0;
        $editbugs ||= 0;

        # We use them in placeholders only. So it's safe to detaint them.
        detaint_natural($newentry);
        detaint_natural($newothercontrol);
        detaint_natural($newmembercontrol);
        detaint_natural($newcanedit);
        detaint_natural($new_editcomponents);
        detaint_natural($new_canconfirm);
        detaint_natural($new_editbugs);

        if (!defined($oldentry)
            && ($newentry || $newmembercontrol || $newcanedit
                || $new_editcomponents || $new_canconfirm || $new_editbugs))
        {
            $sth_Insert->execute($groupid, $product->id, $newentry,
                                 $newmembercontrol, $newothercontrol, $newcanedit,
                                 $new_editcomponents, $new_canconfirm, $new_editbugs);
        }
        elsif (($newentry != $entry)
               || ($newmembercontrol != $membercontrol)
               || ($newothercontrol != $othercontrol)
               || ($newcanedit != $canedit)
               || ($new_editcomponents != $editcomponents)
               || ($new_canconfirm != $canconfirm)
               || ($new_editbugs != $editbugs))
        {
            $sth_Update->execute($newentry, $newmembercontrol, $newothercontrol,
                                 $newcanedit, $new_editcomponents, $new_canconfirm,
                                 $new_editbugs, $groupid, $product->id);
        }

        if (!$newentry && !$newmembercontrol && !$newothercontrol
            && !$newcanedit && !$new_editcomponents && !$new_canconfirm
            && !$new_editbugs)
        {
            $sth_Delete->execute($groupid, $product->id);
        }
    }

    my $sth_Select = $dbh->prepare(
                     'SELECT bugs.bug_id,
                   CASE WHEN (lastdiffed >= delta_ts) THEN 1 ELSE 0 END
                        FROM bugs
                  INNER JOIN bug_group_map
                          ON bug_group_map.bug_id = bugs.bug_id
                       WHERE group_id = ?
                         AND bugs.product_id = ?
                    ORDER BY bugs.bug_id');

    my $sth_Select2 = $dbh->prepare('SELECT name, NOW() FROM groups WHERE id = ?');

    $sth_Update = $dbh->prepare('UPDATE bugs SET delta_ts = ? WHERE bug_id = ?');

    my $sth_Update2 = $dbh->prepare('UPDATE bugs SET delta_ts = ?, lastdiffed = ?
                                     WHERE bug_id = ?');

    $sth_Delete = $dbh->prepare('DELETE FROM bug_group_map
                                 WHERE bug_id = ? AND group_id = ?');

    my @removed_na;
    foreach my $groupid (@now_na) {
        my $count = 0;
        my $bugs = $dbh->selectall_arrayref($sth_Select, undef,
                                            ($groupid, $product->id));

        my ($removed, $timestamp) =
            $dbh->selectrow_array($sth_Select2, undef, $groupid);

        foreach my $bug (@$bugs) {
            my ($bugid, $mailiscurrent) = @$bug;
            $sth_Delete->execute($bugid, $groupid);

            LogActivityEntry($bugid, "bug_group", $removed, "",
                             $whoid, $timestamp);

            if ($mailiscurrent) {
                $sth_Update2->execute($timestamp, $timestamp, $bugid);
            }
            else {
                $sth_Update->execute($timestamp, $bugid);
            }
            $count++;
        }
        my %group = (name => $removed, bug_count => $count);

        push(@removed_na, \%group);
    }

    $sth_Select = $dbh->prepare(
                  'SELECT bugs.bug_id,
                CASE WHEN (lastdiffed >= delta_ts) THEN 1 ELSE 0 END
                     FROM bugs
                LEFT JOIN bug_group_map
                       ON bug_group_map.bug_id = bugs.bug_id
                      AND group_id = ?
                    WHERE bugs.product_id = ?
                      AND bug_group_map.bug_id IS NULL
                 ORDER BY bugs.bug_id');

    $sth_Insert = $dbh->prepare('INSERT INTO bug_group_map
                                 (bug_id, group_id) VALUES (?, ?)');

    my @added_mandatory;
    foreach my $groupid (@now_mandatory) {
        my $count = 0;
        my $bugs = $dbh->selectall_arrayref($sth_Select, undef,
                                            ($groupid, $product->id));

        my ($added, $timestamp) =
            $dbh->selectrow_array($sth_Select2, undef, $groupid);

        foreach my $bug (@$bugs) {
            my ($bugid, $mailiscurrent) = @$bug;
            $sth_Insert->execute($bugid, $groupid);

            LogActivityEntry($bugid, "bug_group", "", $added,
                             $whoid, $timestamp);

            if ($mailiscurrent) {
                $sth_Update2->execute($timestamp, $timestamp, $bugid);
            }
            else {
                $sth_Update->execute($timestamp, $bugid);
            }
            $count++;
        }
        my %group = (name => $added, bug_count => $count);

        push(@added_mandatory, \%group);
    }
    $dbh->bz_unlock_tables();

    delete_token($token);

    $vars->{'removed_na'} = \@removed_na;
    $vars->{'added_mandatory'} = \@added_mandatory;
    $vars->{'product'} = $product;

    $template->process("admin/products/groupcontrol/updated.html.tmpl", $vars)
        || ThrowTemplateError($template->error());
    exit;
}

#
# action='update' -> update the product
#
if ($action eq 'update') {
    check_token_data($token, 'edit_product');
    my $product_old_name    = trim($cgi->param('product_old_name')    || '');
    my $description         = trim($cgi->param('description')         || '');
    my $disallownew         = trim($cgi->param('disallownew')         || '');
    my $milestoneurl        = trim($cgi->param('milestoneurl')        || '');
    my $votesperuser        = trim($cgi->param('votesperuser')        || 0);
    my $maxvotesperbug      = trim($cgi->param('maxvotesperbug')      || 0);
    my $votestoconfirm      = trim($cgi->param('votestoconfirm')      || 0);
    my $defaultmilestone    = trim($cgi->param('defaultmilestone')    || '---');

    my $checkvotes = 0;

    my $product_old = $user->check_can_admin_product($product_old_name);

    if (Bugzilla->params->{'useclassification'}) {
        my $classification; 
        if (!$classification_name) {
            $classification = 
                new Bugzilla::Classification($product_old->classification_id);
        } else {
            $classification = 
                Bugzilla::Classification::check_classification($classification_name);
            if ($classification->id != $product_old->classification_id) {
                ThrowUserError('classification_doesnt_exist_for_product',
                               { product => $product_old->name,
                                 classification => $classification->name });
            }
        }
        $vars->{'classification'} = $classification;
    }

    unless ($product_name) {
        ThrowUserError('product_cant_delete_name',
                       {product => $product_old->name});
    }

    unless ($description) {
        ThrowUserError('product_cant_delete_description',
                       {product => $product_old->name});
    }

    my $stored_maxvotesperbug = $maxvotesperbug;
    if (!detaint_natural($maxvotesperbug)) {
        ThrowUserError('product_votes_per_bug_must_be_nonnegative',
                       {maxvotesperbug => $stored_maxvotesperbug});
    }

    my $stored_votesperuser = $votesperuser;
    if (!detaint_natural($votesperuser)) {
        ThrowUserError('product_votes_per_user_must_be_nonnegative',
                       {votesperuser => $stored_votesperuser});
    }

    my $stored_votestoconfirm = $votestoconfirm;
    if (!detaint_natural($votestoconfirm)) {
        ThrowUserError('product_votes_to_confirm_must_be_nonnegative',
                       {votestoconfirm => $stored_votestoconfirm});
    }

    $dbh->bz_lock_tables('products WRITE', 'milestones READ');

    my $testproduct = 
        new Bugzilla::Product({name => $product_name});
    if (lc($product_name) ne lc($product_old->name) &&
        $testproduct) {
        ThrowUserError('product_name_already_in_use',
                       {product => $product_name});
    }

    # Only update milestone related stuff if 'usetargetmilestone' is on.
    if (Bugzilla->params->{'usetargetmilestone'}) {
        my $milestone = new Bugzilla::Milestone(
            { product => $product_old, name => $defaultmilestone });

        unless ($milestone) {
            ThrowUserError('product_must_define_defaultmilestone',
                           {product          => $product_old->name,
                            defaultmilestone => $defaultmilestone,
                            classification   => $classification_name});
        }

        if ($milestoneurl ne $product_old->milestone_url) {
            trick_taint($milestoneurl);
            $dbh->do('UPDATE products SET milestoneurl = ? WHERE id = ?',
                     undef, ($milestoneurl, $product_old->id));
        }

        if ($milestone->name ne $product_old->default_milestone) {
            $dbh->do('UPDATE products SET defaultmilestone = ? WHERE id = ?',
                     undef, ($milestone->name, $product_old->id));
        }
    }

    $disallownew = $disallownew ? 1 : 0;
    if ($disallownew ne $product_old->disallow_new) {
        $dbh->do('UPDATE products SET disallownew = ? WHERE id = ?',
                 undef, ($disallownew, $product_old->id));
    }

    if ($description ne $product_old->description) {
        trick_taint($description);
        $dbh->do('UPDATE products SET description = ? WHERE id = ?',
                 undef, ($description, $product_old->id));
    }

    if ($votesperuser ne $product_old->votes_per_user) {
        $dbh->do('UPDATE products SET votesperuser = ? WHERE id = ?',
                 undef, ($votesperuser, $product_old->id));
        $checkvotes = 1;
    }

    if ($maxvotesperbug ne $product_old->max_votes_per_bug) {
        $dbh->do('UPDATE products SET maxvotesperbug = ? WHERE id = ?',
                 undef, ($maxvotesperbug, $product_old->id));
        $checkvotes = 1;
    }

    if ($votestoconfirm ne $product_old->votes_to_confirm) {
        $dbh->do('UPDATE products SET votestoconfirm = ? WHERE id = ?',
                 undef, ($votestoconfirm, $product_old->id));
        $checkvotes = 1;
    }

    if ($product_name ne $product_old->name) {
        trick_taint($product_name);
        $dbh->do('UPDATE products SET name = ? WHERE id = ?',
                 undef, ($product_name, $product_old->id));
    }

    $dbh->bz_unlock_tables();

    my $product = new Bugzilla::Product({name => $product_name});

    if ($checkvotes) {
        $vars->{'checkvotes'} = 1;

        # 1. too many votes for a single user on a single bug.
        my @toomanyvotes_list = ();
        if ($maxvotesperbug < $votesperuser) {
            my $votes = $dbh->selectall_arrayref(
                        'SELECT votes.who, votes.bug_id
                           FROM votes
                     INNER JOIN bugs
                             ON bugs.bug_id = votes.bug_id
                          WHERE bugs.product_id = ?
                            AND votes.vote_count > ?',
                         undef, ($product->id, $maxvotesperbug));

            foreach my $vote (@$votes) {
                my ($who, $id) = (@$vote);
                # If some votes are removed, RemoveVotes() returns a list
                # of messages to send to voters.
                my $msgs = RemoveVotes($id, $who, 'votes_too_many_per_bug');
                foreach my $msg (@$msgs) {
                    MessageToMTA($msg);
                }
                my $name = user_id_to_login($who);

                push(@toomanyvotes_list,
                     {id => $id, name => $name});
            }
        }
        $vars->{'toomanyvotes'} = \@toomanyvotes_list;

        # 2. too many total votes for a single user.
        # This part doesn't work in the general case because RemoveVotes
        # doesn't enforce votesperuser (except per-bug when it's less
        # than maxvotesperbug).  See Bugzilla::Bug::RemoveVotes().

        my $votes = $dbh->selectall_arrayref(
                    'SELECT votes.who, votes.vote_count
                       FROM votes
                 INNER JOIN bugs
                         ON bugs.bug_id = votes.bug_id
                      WHERE bugs.product_id = ?',
                     undef, $product->id);

        my %counts;
        foreach my $vote (@$votes) {
            my ($who, $count) = @$vote;
            if (!defined $counts{$who}) {
                $counts{$who} = $count;
            } else {
                $counts{$who} += $count;
            }
        }
        my @toomanytotalvotes_list = ();
        foreach my $who (keys(%counts)) {
            if ($counts{$who} > $votesperuser) {
                my $bug_ids = $dbh->selectcol_arrayref(
                              'SELECT votes.bug_id
                                 FROM votes
                           INNER JOIN bugs
                                   ON bugs.bug_id = votes.bug_id
                                WHERE bugs.product_id = ?
                                  AND votes.who = ?',
                               undef, ($product->id, $who));

                foreach my $bug_id (@$bug_ids) {
                    # RemoveVotes() returns a list of messages to send
                    # in case some voters had too many votes.
                    my $msgs = RemoveVotes($bug_id, $who, 'votes_too_many_per_user');
                    foreach my $msg (@$msgs) {
                        MessageToMTA($msg);
                    }
                    my $name = user_id_to_login($who);

                    push(@toomanytotalvotes_list,
                         {id => $bug_id, name => $name});
                }
            }
        }
        $vars->{'toomanytotalvotes'} = \@toomanytotalvotes_list;

        # 3. enough votes to confirm
        my $bug_list = $dbh->selectcol_arrayref(
                       "SELECT bug_id FROM bugs
                         WHERE product_id = ?
                           AND bug_status = 'UNCONFIRMED'
                           AND votes >= ?",
                        undef, ($product->id, $votestoconfirm));

        my @updated_bugs = ();
        foreach my $bug_id (@$bug_list) {
            my $confirmed = CheckIfVotedConfirmed($bug_id, $whoid);
            push (@updated_bugs, $bug_id) if $confirmed;
        }

        $vars->{'confirmedbugs'} = \@updated_bugs;
        $vars->{'changer'} = $user->login;
    }
    delete_token($token);

    $vars->{'old_product'} = $product_old;
    $vars->{'product'} = $product;

    $template->process("admin/products/updated.html.tmpl", $vars)
        || ThrowTemplateError($template->error());
    exit;
}

#
# action='editgroupcontrols' -> update product group controls
#

if ($action eq 'editgroupcontrols') {
    my $product = $user->check_can_admin_product($product_name);

    # Display a group if it is either enabled or has bugs for this product.
    my $groups = $dbh->selectall_arrayref(
        'SELECT id, name, entry, membercontrol, othercontrol, canedit,
                editcomponents, editbugs, canconfirm,
                isactive, COUNT(bugs.bug_id) AS bugcount
           FROM groups
      LEFT JOIN group_control_map
             ON group_control_map.group_id = groups.id
            AND group_control_map.product_id = ?
      LEFT JOIN bug_group_map
             ON bug_group_map.group_id = groups.id
      LEFT JOIN bugs
             ON bugs.bug_id = bug_group_map.bug_id
            AND bugs.product_id = ?
          WHERE isbuggroup != 0
            AND (isactive != 0 OR entry IS NOT NULL OR bugs.bug_id IS NOT NULL) ' .
           $dbh->sql_group_by('name', 'id, entry, membercontrol,
                              othercontrol, canedit, isactive,
                              editcomponents, canconfirm, editbugs'),
        {'Slice' => {}}, ($product->id, $product->id));

    $vars->{'product'} = $product;
    $vars->{'groups'} = $groups;
    $vars->{'token'} = issue_session_token('edit_group_controls');

    $vars->{'const'} = {
        'CONTROLMAPNA' => CONTROLMAPNA,
        'CONTROLMAPSHOWN' => CONTROLMAPSHOWN,
        'CONTROLMAPDEFAULT' => CONTROLMAPDEFAULT,
        'CONTROLMAPMANDATORY' => CONTROLMAPMANDATORY,
    };

    $template->process("admin/products/groupcontrol/edit.html.tmpl", $vars)
        || ThrowTemplateError($template->error());
    exit;                
}


#
# No valid action found
#

ThrowUserError('no_valid_action', {field => "product"});
