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
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are Copyright (C) 1998
# Netscape Communications Corporation. All Rights Reserved.
# 
# Contributor(s): Terry Weissman <terry@mozilla.org>
#                 Dave Miller <justdave@syndicomm.com>
#                 Joe Robins <jmrobins@tgix.com>
#                 Gervase Markham <gerv@gerv.net>
#                 Shane H. W. Travis <travis@sedsystems.ca>

##############################################################################
#
# enter_bug.cgi
# -------------
# Displays bug entry form. Bug fields are specified through popup menus, 
# drop-down lists, or text fields. Default for these values can be 
# passed in as parameters to the cgi.
#
##############################################################################

use strict;

use lib qw(.);

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::Bug;
use Bugzilla::User;
use Bugzilla::Hook;
use Bugzilla::Product;
use Bugzilla::Classification;
use Bugzilla::Keyword;
use Bugzilla::Token;
use Bugzilla::Field;

my $user = Bugzilla->login(LOGIN_REQUIRED);

my $cloned_bug;
my $cloned_bug_id;

my $cgi = Bugzilla->cgi;
my $dbh = Bugzilla->dbh;
my $template = Bugzilla->template;
my $vars = {};

my $product_name = trim($cgi->param('product') || '');
# Will contain the product object the bug is created in.
my $product;

if ($product_name eq '') {
    # If the user cannot enter bugs in any product, stop here.
    my @enterable_products = @{$user->get_enterable_products};
    ThrowUserError('no_products') unless scalar(@enterable_products);

    my $classification = Bugzilla->params->{'useclassification'} ?
        scalar($cgi->param('classification')) : '__all';

    # Unless a real classification name is given, we sort products
    # by classification.
    my @classifications;

    unless ($classification && $classification ne '__all') {
        if (Bugzilla->params->{'useclassification'}) {
            my $class;
            # Get all classifications with at least one enterable product.
            foreach my $product (@enterable_products) {
                $class->{$product->classification_id}->{'object'} ||=
                    new Bugzilla::Classification($product->classification_id);
                # Nice way to group products per classification, without querying
                # the DB again.
                push(@{$class->{$product->classification_id}->{'products'}}, $product);
            }
            @classifications = sort {$a->{'object'}->sortkey <=> $b->{'object'}->sortkey
                                     || lc($a->{'object'}->name) cmp lc($b->{'object'}->name)}
                                    (values %$class);
        }
        else {
            @classifications = ({object => undef, products => \@enterable_products});
        }
    }

    unless ($classification) {
        # We know there is at least one classification available,
        # else we would have stopped earlier.
        if (scalar(@classifications) > 1) {
            # We only need classification objects.
            $vars->{'classifications'} = [map {$_->{'object'}} @classifications];

            $vars->{'target'} = "enter_bug.cgi";
            $vars->{'format'} = $cgi->param('format');
            $vars->{'cloned_bug_id'} = $cgi->param('cloned_bug_id');

            print $cgi->header();
            $template->process("global/choose-classification.html.tmpl", $vars)
               || ThrowTemplateError($template->error());
            exit;
        }
        # If we come here, then there is only one classification available.
        $classification = $classifications[0]->{'object'}->name;
    }

    # Keep only enterable products which are in the specified classification.
    if ($classification ne "__all") {
        my $class = new Bugzilla::Classification({'name' => $classification});
        # If the classification doesn't exist, then there is no product in it.
        if ($class) {
            @enterable_products
              = grep {$_->classification_id == $class->id} @enterable_products;
            @classifications = ({object => $class, products => \@enterable_products});
        }
        else {
            @enterable_products = ();
        }
    }

    if (scalar(@enterable_products) == 0) {
        ThrowUserError('no_products');
    }
    elsif (scalar(@enterable_products) > 1) {
        $vars->{'classifications'} = \@classifications;
        $vars->{'target'} = "enter_bug.cgi";
        $vars->{'format'} = $cgi->param('format');
        $vars->{'cloned_bug_id'} = $cgi->param('cloned_bug_id');

        print $cgi->header();
        $template->process("global/choose-product.html.tmpl", $vars)
          || ThrowTemplateError($template->error());
        exit;
    } else {
        # Only one product exists.
        $product = $enterable_products[0];
    }
}
else {
    # Do not use Bugzilla::Product::check_product() here, else the user
    # could know whether the product doesn't exist or is not accessible.
    $product = new Bugzilla::Product({'name' => $product_name});
}

# We need to check and make sure that the user has permission
# to enter a bug against this product.
$user->can_enter_product($product ? $product->name : $product_name, THROW_ERROR);

##############################################################################
# Useful Subroutines
##############################################################################
sub formvalue {
    my ($name, $default) = (@_);
    return Bugzilla->cgi->param($name) || $default || "";
}

# Takes the name of a field and a list of possible values for that 
# field. Returns the first value in the list that is actually a 
# valid value for that field.
# The field should be named after its DB table.
# Returns undef if none of the platforms match.
sub pick_valid_field_value (@) {
    my ($field, @values) = @_;
    my $dbh = Bugzilla->dbh;

    foreach my $value (@values) {
        return $value if $dbh->selectrow_array(
            "SELECT 1 FROM $field WHERE value = ?", undef, $value); 
    }
    return undef;
}

sub pickplatform {
    return formvalue("rep_platform") if formvalue("rep_platform");

    my @platform;

    if (Bugzilla->params->{'defaultplatform'}) {
        @platform = Bugzilla->params->{'defaultplatform'};
    } else {
        # If @platform is a list, this function will return the first
        # item in the list that is a valid platform choice. If
        # no choice is valid, we return "Other".
        for ($ENV{'HTTP_USER_AGENT'}) {
        #PowerPC
            /\(.*PowerPC.*\)/i && do {@platform = "Macintosh"; last;};
            /\(.*PPC.*\)/ && do {@platform = "Macintosh"; last;};
            /\(.*AIX.*\)/ && do {@platform = "Macintosh"; last;};
        #Intel x86
            /\(.*Intel.*\)/ && do {@platform = "PC"; last;};
            /\(.*[ix0-9]86.*\)/ && do {@platform = "PC"; last;};
        #Versions of Windows that only run on Intel x86
            /\(.*Win(?:dows |)[39M].*\)/ && do {@platform = "PC"; last};
            /\(.*Win(?:dows |)16.*\)/ && do {@platform = "PC"; last;};
        #Sparc
            /\(.*sparc.*\)/ && do {@platform = "Sun"; last;};
            /\(.*sun4.*\)/ && do {@platform = "Sun"; last;};
        #Alpha
            /\(.*AXP.*\)/i && do {@platform = "DEC"; last;};
            /\(.*[ _]Alpha.\D/i && do {@platform = "DEC"; last;};
            /\(.*[ _]Alpha\)/i && do {@platform = "DEC"; last;};
        #MIPS
            /\(.*IRIX.*\)/i && do {@platform = "SGI"; last;};
            /\(.*MIPS.*\)/i && do {@platform = "SGI"; last;};
        #68k
            /\(.*68K.*\)/ && do {@platform = "Macintosh"; last;};
            /\(.*680[x0]0.*\)/ && do {@platform = "Macintosh"; last;};
        #HP
            /\(.*9000.*\)/ && do {@platform = "HP"; last;};
        #ARM
#            /\(.*ARM.*\) && do {$platform = "ARM";};
        #Stereotypical and broken
            /\(.*Macintosh.*\)/ && do {@platform = "Macintosh"; last;};
            /\(.*Mac OS [89].*\)/ && do {@platform = "Macintosh"; last;};
            /\(Win.*\)/ && do {@platform = "PC"; last;};
            /\(.*Win(?:dows[ -])NT.*\)/ && do {@platform = "PC"; last;};
            /\(.*OSF.*\)/ && do {@platform = "DEC"; last;};
            /\(.*HP-?UX.*\)/i && do {@platform = "HP"; last;};
            /\(.*IRIX.*\)/i && do {@platform = "SGI"; last;};
            /\(.*(SunOS|Solaris).*\)/ && do {@platform = "Sun"; last;};
        #Braindead old browsers who didn't follow convention:
            /Amiga/ && do {@platform = "Macintosh"; last;};
            /WinMosaic/ && do {@platform = "PC"; last;};
        }
    }

    return pick_valid_field_value('rep_platform', @platform) || "Other";
}

sub pickos {
    if (formvalue('op_sys') ne "") {
        return formvalue('op_sys');
    }

    my @os = ();

    if (Bugzilla->params->{'defaultopsys'}) {
        @os = Bugzilla->params->{'defaultopsys'};
    } else {
        # This function will return the first
        # item in @os that is a valid platform choice. If
        # no choice is valid, we return "Other".
        for ($ENV{'HTTP_USER_AGENT'}) {
            /\(.*IRIX.*\)/ && do {push @os, "IRIX"; };
            /\(.*OSF.*\)/ && do {push @os, "OSF/1";};
            /\(.*Linux.*\)/ && do {push @os, "Linux";};
            /\(.*Solaris.*\)/ && do {push @os, "Solaris";};
            /\(.*SunOS 5.*\)/ && do {push @os, "Solaris";};
            /\(.*SunOS.*sun4u.*\)/ && do {push @os, "Solaris";};
            /\(.*SunOS.*\)/ && do {push @os, "SunOS";};
            /\(.*HP-?UX.*\)/ && do {push @os, "HP-UX";};
            /\(.*BSD\/(?:OS|386).*\)/ && do {push @os, "BSDI";};
            /\(.*FreeBSD.*\)/ && do {push @os, "FreeBSD";};
            /\(.*OpenBSD.*\)/ && do {push @os, "OpenBSD";};
            /\(.*NetBSD.*\)/ && do {push @os, "NetBSD";};
            /\(.*BeOS.*\)/ && do {push @os, "BeOS";};
            /\(.*AIX.*\)/ && do {push @os, "AIX";};
            /\(.*OS\/2.*\)/ && do {push @os, "OS/2";};
            /\(.*QNX.*\)/ && do {push @os, "Neutrino";};
            /\(.*VMS.*\)/ && do {push @os, "OpenVMS";};
            /\(.*Windows XP.*\)/ && do {push @os, "Windows XP";};
            /\(.*Windows NT 6\.0.*\)/ && do {push @os, "Windows Vista";};
            /\(.*Windows NT 5\.2.*\)/ && do {push @os, "Windows Server 2003";};
            /\(.*Windows NT 5\.1.*\)/ && do {push @os, "Windows XP";};
            /\(.*Windows 2000.*\)/ && do {push @os, "Windows 2000";};
            /\(.*Windows NT 5.*\)/ && do {push @os, "Windows 2000";};
            /\(.*Win.*9[8x].*4\.9.*\)/ && do {push @os, "Windows ME";};
            /\(.*Win(?:dows |)M[Ee].*\)/ && do {push @os, "Windows ME";};
            /\(.*Win(?:dows |)98.*\)/ && do {push @os, "Windows 98";};
            /\(.*Win(?:dows |)95.*\)/ && do {push @os, "Windows 95";};
            /\(.*Win(?:dows |)16.*\)/ && do {push @os, "Windows 3.1";};
            /\(.*Win(?:dows[ -]|)NT.*\)/ && do {push @os, "Windows NT";};
            /\(.*Windows.*NT.*\)/ && do {push @os, "Windows NT";};
            /\(.*32bit.*\)/ && do {push @os, "Windows 95";};
            /\(.*16bit.*\)/ && do {push @os, "Windows 3.1";};
            /\(.*Mac OS 9.*\)/ && do {push @os, "Mac System 9.x";};
            /\(.*Mac OS 8\.6.*\)/ && do {push @os, "Mac System 8.6";};
            /\(.*Mac OS 8\.5.*\)/ && do {push @os, "Mac System 8.5";};
        # Bugzilla doesn't have an entry for 8.1
            /\(.*Mac OS 8\.1.*\)/ && do {push @os, "Mac System 8.0";};
            /\(.*Mac OS 8\.0.*\)/ && do {push @os, "Mac System 8.0";};
            /\(.*Mac OS 8[^.].*\)/ && do {push @os, "Mac System 8.0";};
            /\(.*Mac OS 8.*\)/ && do {push @os, "Mac System 8.6";};
            /\(.*Intel.*Mac OS X.*\)/ && do {push @os, "Mac OS X 10.4";};
            /\(.*Mac OS X.*\)/ && do {push @os, ("Mac OS X 10.3", "Mac OS X 10.0");};
            /\(.*Darwin.*\)/ && do {push @os, "Mac OS X 10.0";};
        # Silly
            /\(.*Mac.*PowerPC.*\)/ && do {push @os, "Mac System 9.x";};
            /\(.*Mac.*PPC.*\)/ && do {push @os, "Mac System 9.x";};
            /\(.*Mac.*68k.*\)/ && do {push @os, "Mac System 8.0";};
        # Evil
            /Amiga/i && do {push @os, "Other";};
            /WinMosaic/ && do {push @os, "Windows 95";};
            /\(.*PowerPC.*\)/ && do {push @os, "Mac System 9.x";};
            /\(.*PPC.*\)/ && do {push @os, "Mac System 9.x";};
            /\(.*68K.*\)/ && do {push @os, "Mac System 8.0";};
        }
    }

    push(@os, "Windows") if grep(/^Windows /, @os);
    push(@os, "Mac OS") if grep(/^Mac /, @os);

    return pick_valid_field_value('op_sys', @os) || "Other";
}
##############################################################################
# End of subroutines
##############################################################################

my $has_editbugs = $user->in_group('editbugs', $product->id);
my $has_canconfirm = $user->in_group('canconfirm', $product->id);

# If a user is trying to clone a bug
#   Check that the user has authorization to view the parent bug
#   Create an instance of Bug that holds the info from the parent
$cloned_bug_id = $cgi->param('cloned_bug_id');

if ($cloned_bug_id) {
    ValidateBugID($cloned_bug_id);
    $cloned_bug = new Bugzilla::Bug($cloned_bug_id);
}

if (scalar(@{$product->components}) == 1) {
    # Only one component; just pick it.
    $cgi->param('component', $product->components->[0]->name);
}

my %default;

$vars->{'product'}               = $product;

$vars->{'priority'}              = get_legal_field_values('priority');
$vars->{'bug_severity'}          = get_legal_field_values('bug_severity');
$vars->{'rep_platform'}          = get_legal_field_values('rep_platform');
$vars->{'op_sys'}                = get_legal_field_values('op_sys');

$vars->{'use_keywords'}          = 1 if Bugzilla::Keyword::keyword_count();

$vars->{'assigned_to'}           = formvalue('assigned_to');
$vars->{'assigned_to_disabled'}  = !$has_editbugs;
$vars->{'cc_disabled'}           = 0;

$vars->{'qa_contact'}           = formvalue('qa_contact');
$vars->{'qa_contact_disabled'}  = !$has_editbugs;

$vars->{'cloned_bug_id'}         = $cloned_bug_id;

$vars->{'token'}             = issue_session_token('createbug:');


my @enter_bug_fields = Bugzilla->get_fields({ custom => 1, obsolete => 0, 
                                              enter_bug => 1 });
foreach my $field (@enter_bug_fields) {
    $vars->{$field->name} = formvalue($field->name);
}

if ($cloned_bug_id) {

    $default{'component_'}    = $cloned_bug->component;
    $default{'priority'}      = $cloned_bug->{'priority'};
    $default{'bug_severity'}  = $cloned_bug->{'bug_severity'};
    $default{'rep_platform'}  = $cloned_bug->{'rep_platform'};
    $default{'op_sys'}        = $cloned_bug->{'op_sys'};

    $vars->{'short_desc'}     = $cloned_bug->{'short_desc'};
    $vars->{'bug_file_loc'}   = $cloned_bug->{'bug_file_loc'};
    $vars->{'keywords'}       = $cloned_bug->keywords;
    $vars->{'dependson'}      = $cloned_bug_id;
    $vars->{'blocked'}        = "";
    $vars->{'deadline'}       = $cloned_bug->{'deadline'};

    if (defined $cloned_bug->cc) {
        $vars->{'cc'}         = join (" ", @{$cloned_bug->cc});
    } else {
        $vars->{'cc'}         = formvalue('cc');
    }

    foreach my $field (@enter_bug_fields) {
        $vars->{$field->name} = $cloned_bug->{$field->name};
    }

# We need to ensure that we respect the 'insider' status of
# the first comment, if it has one. Either way, make a note
# that this bug was cloned from another bug.

    $cloned_bug->longdescs();
    my $isprivate             = $cloned_bug->{'longdescs'}->[0]->{'isprivate'};

    $vars->{'comment'}        = "";
    $vars->{'commentprivacy'} = 0;

    if ( !($isprivate) ||
         ( ( Bugzilla->params->{"insidergroup"} ) && 
           ( Bugzilla->user->in_group(Bugzilla->params->{"insidergroup"}) ) ) 
       ) {
        $vars->{'comment'}        = $cloned_bug->{'longdescs'}->[0]->{'body'};
        $vars->{'commentprivacy'} = $isprivate;
    }

# Ensure that the groupset information is set up for later use.
    $cloned_bug->groups();

} # end of cloned bug entry form

else {

    $default{'component_'}    = formvalue('component');
    $default{'priority'}      = formvalue('priority', Bugzilla->params->{'defaultpriority'});
    $default{'bug_severity'}  = formvalue('bug_severity', Bugzilla->params->{'defaultseverity'});
    $default{'rep_platform'}  = pickplatform();
    $default{'op_sys'}        = pickos();

    $vars->{'short_desc'}     = formvalue('short_desc');
    $vars->{'bug_file_loc'}   = formvalue('bug_file_loc', "http://");
    $vars->{'keywords'}       = formvalue('keywords');
    $vars->{'dependson'}      = formvalue('dependson');
    $vars->{'blocked'}        = formvalue('blocked');
    $vars->{'deadline'}       = formvalue('deadline');

    $vars->{'cc'}             = join(', ', $cgi->param('cc'));

    $vars->{'comment'}        = formvalue('comment');
    $vars->{'commentprivacy'} = formvalue('commentprivacy');

} # end of normal/bookmarked entry form


# IF this is a cloned bug,
# AND the clone's product is the same as the parent's
#   THEN use the version from the parent bug
# ELSE IF a version is supplied in the URL
#   THEN use it
# ELSE IF there is a version in the cookie
#   THEN use it (Posting a bug sets a cookie for the current version.)
# ELSE
#   The default version is the last one in the list (which, it is
#   hoped, will be the most recent one).
#
# Eventually maybe each product should have a "current version"
# parameter.
$vars->{'version'} = [map($_->name, @{$product->versions})];

if ( ($cloned_bug_id) &&
     ($product->name eq $cloned_bug->product ) ) {
    $default{'version'} = $cloned_bug->{'version'};
} elsif (formvalue('version')) {
    $default{'version'} = formvalue('version');
} elsif (defined $cgi->cookie("VERSION-" . $product->name) &&
    lsearch($vars->{'version'}, $cgi->cookie("VERSION-" . $product->name)) != -1) {
    $default{'version'} = $cgi->cookie("VERSION-" . $product->name);
} else {
    $default{'version'} = $vars->{'version'}->[$#{$vars->{'version'}}];
}

# Get list of milestones.
if ( Bugzilla->params->{'usetargetmilestone'} ) {
    $vars->{'target_milestone'} = [map($_->name, @{$product->milestones})];
    if (formvalue('target_milestone')) {
       $default{'target_milestone'} = formvalue('target_milestone');
    } else {
       $default{'target_milestone'} = $product->default_milestone;
    }
}

# Construct the list of allowable statuses.
#
# * If the product requires votes to confirm:
#   users with privs   : NEW + ASSI + UNCO
#   users with no privs: UNCO
#
# * If the product doesn't require votes to confirm:
#   users with privs   : NEW + ASSI
#   users with no privs: NEW (as these users cannot reassign
#                             bugs to them, it doesn't make sense
#                             to let them mark bugs as ASSIGNED)

my @status;
@status = ('UNCONFIRMED');

$vars->{'bug_status'} = \@status; 

# Get the default from a template value if it is legitimate.
# Otherwise, set the default to the first item on the list.

if (formvalue('bug_status') && (lsearch(\@status, formvalue('bug_status')) >= 0)) {
    $default{'bug_status'} = formvalue('bug_status');
} else {
    $default{'bug_status'} = $status[0];
}
 
my $grouplist = $dbh->selectall_arrayref(
                  q{SELECT DISTINCT groups.id, groups.name, groups.description,
                                    membercontrol, othercontrol
                      FROM groups
                 LEFT JOIN group_control_map
                        ON group_id = id AND product_id = ?
                     WHERE isbuggroup != 0 AND isactive != 0
                  ORDER BY description}, undef, $product->id);

my @groups;

foreach my $row (@$grouplist) {
    my ($id, $groupname, $description, $membercontrol, $othercontrol) = @$row;
    # Only include groups if the entering user will have an option.
    next if ((!$membercontrol) 
               || ($membercontrol == CONTROLMAPNA) 
               || ($membercontrol == CONTROLMAPMANDATORY)
               || (($othercontrol != CONTROLMAPSHOWN) 
                    && ($othercontrol != CONTROLMAPDEFAULT)
                    && (!Bugzilla->user->in_group($groupname)))
             );
    my $check;

    # If this is a cloned bug, 
    # AND the product for this bug is the same as for the original
    #   THEN set a group's checkbox if the original also had it on
    # ELSE IF this is a bookmarked template
    #   THEN set a group's checkbox if was set in the bookmark
    # ELSE
    #   set a groups's checkbox based on the group control map
    #
    if ( ($cloned_bug_id) &&
         ($product->name eq $cloned_bug->product ) ) {
        foreach my $i (0..(@{$cloned_bug->{'groups'}}-1) ) {
            if ($cloned_bug->{'groups'}->[$i]->{'bit'} == $id) {
                $check = $cloned_bug->{'groups'}->[$i]->{'ison'};
            }
        }
    }
    elsif(formvalue("maketemplate") ne "") {
        $check = formvalue("bit-$id", 0);
    }
    else {
        # Checkbox is checked by default if $control is a default state.
        $check = (($membercontrol == CONTROLMAPDEFAULT)
                 || (($othercontrol == CONTROLMAPDEFAULT)
                      && (!Bugzilla->user->in_group($groupname))));
    }

    my $group = 
    {
        'bit' => $id , 
        'checked' => $check , 
        'description' => $description 
    };

    push @groups, $group;        
}

$vars->{'group'} = \@groups;

Bugzilla::Hook::process("enter_bug-entrydefaultvars", { vars => $vars });

$vars->{'default'} = \%default;

my $format = $template->get_format("bug/create/create",
                                   scalar $cgi->param('format'), 
                                   scalar $cgi->param('ctype'));

print $cgi->header($format->{'ctype'});
$template->process($format->{'template'}, $vars)
  || ThrowTemplateError($template->error());          

