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
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): Dawn Endico <endico@mozilla.org>
#                 Gregary Hendricks <ghendricks@novell.com>
#                 Vance Baarda <vrb@novell.com>
#                 Guzman Braso <gbn@hqso.net>
#                 Erik Purins <epurins@day1studios.com>

# This script reads in xml bug data from standard input and inserts
# a new bug into bugzilla. Everything before the beginning <?xml line
# is removed so you can pipe in email messages.

use strict;

#####################################################################
#
# This script is used to import bugs from another installation of bugzilla.
# It can be used in two ways.
# First using the move function of bugzilla
# on another system will send mail to an alias provided by
# the administrator of the target installation (you). Set up an alias
# similar to the one given below so this mail will be automatically
# run by this script and imported into your database.  Run 'newaliases'
# after adding this alias to your aliases file. Make sure your sendmail
# installation is configured to allow mail aliases to execute code.
#
# bugzilla-import: "|/usr/bin/perl /opt/bugzilla/importxml.pl"
#
# Second it can be run from the command line with any xml file from
# STDIN that conforms to the bugzilla DTD. In this case you can pass
# an argument to set whether you want to send the
# mail that will be sent to the exporter and maintainer normally.
#
# importxml.pl bugsfile.xml
#
#####################################################################

# figure out which path this script lives in. Set the current path to
# this and add it to @INC so this will work when run as part of mail
# alias by the mailer daemon
# since "use lib" is run at compile time, we need to enclose the
# $::path declaration in a BEGIN block so that it is executed before
# the rest of the file is compiled.
BEGIN {
    $::path = $0;
    $::path =~ m#(.*)/[^/]+#;
    $::path = $1;
    $::path ||= '.';    # $0 is empty at compile time.  This line will
                        # have no effect on this script at runtime.
}

chdir $::path;
use lib ($::path);
# Data dumber is used for debugging, I got tired of copying it back in 
# and then removing it. 
#use Data::Dumper;


use Bugzilla;
use Bugzilla::Bug;
use Bugzilla::Product;
use Bugzilla::Version;
use Bugzilla::Component;
use Bugzilla::Milestone;
use Bugzilla::FlagType;
use Bugzilla::BugMail;
use Bugzilla::Mailer;
use Bugzilla::User;
use Bugzilla::Util;
use Bugzilla::Constants;
use Bugzilla::Keyword;
use Bugzilla::Field;

use MIME::Base64;
use MIME::Parser;
use Date::Format;
use Getopt::Long;
use Pod::Usage;
use XML::Twig;

# We want to capture errors and handle them here rather than have the Template
# code barf all over the place.
Bugzilla->usage_mode(Bugzilla::Constants::USAGE_MODE_CMDLINE);

my $debug = 0;
my $mail  = '';
my $attach_path = '';
my $help  = 0;

my $result = GetOptions(
    "verbose|debug+" => \$debug,
    "mail|sendmail!" => \$mail,
    "attach_path=s"  => \$attach_path,
    "help|?"         => \$help
);

pod2usage(0) if $help;

use constant OK_LEVEL    => 3;
use constant DEBUG_LEVEL => 2;
use constant ERR_LEVEL   => 1;

our @logs;
our @attachments;
our $bugtotal;
my $xml;
my $dbh = Bugzilla->dbh;
my $params = Bugzilla->params;
my ($timestamp) = $dbh->selectrow_array("SELECT NOW()");

###############################################################################
# Helper sub routines                                                         #
###############################################################################

sub MailMessage {
    return unless ($mail);
    my $subject    = shift;
    my $message    = shift;
    my @recipients = @_;
    my $from   = $params->{"moved-from-address"};
    $from =~ s/@/\@/g;

    foreach my $to (@recipients){
        my $header = "To: $to\n";
        $header .= "From: Bugzilla <$from>\n";
        $header .= "Subject: $subject\n\n";
        my $sendmessage = $header . $message . "\n";
        MessageToMTA($sendmessage);
    }

}

sub Debug {
    return unless ($debug);
    my ( $message, $level ) = (@_);
    print STDERR "OK: $message \n" if ( $level == OK_LEVEL );
    print STDERR "ERR: $message \n" if ( $level == ERR_LEVEL );
    print STDERR "$message\n"
      if ( ( $debug == $level ) && ( $level == DEBUG_LEVEL ) );
}

sub Error {
    my ( $reason, $errtype, $exporter ) = @_;
    my $subject = "Bug import error: $reason";
    my $message = "Cannot import these bugs because $reason ";
    $message .= "\n\nPlease re-open the original bug.\n" if ($errtype);
    $message .= "For more info, contact " . $params->{"maintainer"} . ".\n";
    my @to = ( $params->{"maintainer"}, $exporter);
    Debug( $message, ERR_LEVEL );
    MailMessage( $subject, $message, @to );
    exit;
}

# This subroutine handles flags for process_bug. It is generic in that
# it can handle both attachment flags and bug flags.
sub flag_handler {
    my (
        $name,            $status,      $setter_login,
        $requestee_login, $exporterid,  $bugid,
        $productid,       $componentid, $attachid
      )
      = @_;

    my $type         = ($attachid) ? "attachment" : "bug";
    my $err          = '';
    my $setter       = new Bugzilla::User({ name => $setter_login });
    my $requestee;
    my $requestee_id;

    unless ($setter) {
        $err = "Invalid setter $setter_login on $type flag $name\n";
        $err .= "   Dropping flag $name\n";
        return $err;
    }
    if ( !$setter->can_see_bug($bugid) ) {
        $err .= "Setter is not a member of bug group\n";
        $err .= "   Dropping flag $name\n";
        return $err;
    }
    my $setter_id = $setter->id;
    if ( defined($requestee_login) ) {
        $requestee = new Bugzilla::User({ name => $requestee_login });
        if ( $requestee ) {
            if ( !$requestee->can_see_bug($bugid) ) {
                $err .= "Requestee is not a member of bug group\n";
                $err .= "   Requesting from the wind\n";
            }    
            else{
                $requestee_id = $requestee->id;
            }
        }
        else {
            $err = "Invalid requestee $requestee_login on $type flag $name\n";
            $err .= "   Requesting from the wind.\n";
        }
        
    }
    my $flag_types;

    # If this is an attachment flag we need to do some dirty work to look
    # up the flagtype ID
    if ($attachid) {
        $flag_types = Bugzilla::FlagType::match(
            {
                'target_type'  => 'attachment',
                'product_id'   => $productid,
                'component_id' => $componentid
            } );
    }
    else {
        my $bug = new Bugzilla::Bug($bugid);
        $flag_types = $bug->flag_types;
    }
    unless ($flag_types){
        $err  = "No flag types defined for this bug\n";
        $err .= "   Dropping flag $name\n";
        return $err;
    }

    # We need to see if the imported flag is in the list of known flags
    # It is possible for two flags on the same bug have the same name
    # If this is the case, we will only match the first one.
    my $ftype;
    foreach my $f ( @{$flag_types} ) {
        if ( $f->name eq $name) {
            $ftype = $f;
            last;
        }
    }

    if ($ftype) {    # We found the flag in the list
        my $grant_group = $ftype->grant_group;
        if (( $status eq '+' || $status eq '-' ) 
            && $grant_group && !$setter->in_group_id($grant_group->id)) {
            $err = "Setter $setter_login on $type flag $name ";
            $err .= "is not in the Grant Group\n";
            $err .= "   Dropping flag $name\n";
            return $err;
        }
        my $request_group = $ftype->request_group;
        if ($request_group
            && $status eq '?' && !$setter->in_group_id($request_group->id)) {
            $err = "Setter $setter_login on $type flag $name ";
            $err .= "is not in the Request Group\n";
            $err .= "   Dropping flag $name\n";
            return $err;
        }

        # Take the first flag_type that matches
        unless ($ftype->is_active) {
            $err = "Flag $name is not active in this database\n";
            $err .= "   Dropping flag $name\n";
            return $err;
        }

        $dbh->do("INSERT INTO flags 
                 (type_id, status, bug_id, attach_id, creation_date, 
                  setter_id, requestee_id)
                  VALUES (?, ?, ?, ?, ?, ?, ?)", undef,
            ($ftype->id, $status, $bugid, $attachid, $timestamp,
            $setter_id, $requestee_id));
    }
    else {
        $err = "Dropping unknown $type flag: $name\n";
        return $err;
    }
    return $err;
}

###############################################################################
# XML Handlers                                                                #
###############################################################################

# This subroutine gets called only once - as soon as the <bugzilla> opening
# tag is parsed. It simply checks to see that the all important exporter
# maintainer and URL base are set.
#
#    exporter:   email address of the person moving the bugs
#    maintainer: the maintainer of the bugzilla installation
#                as set in the parameters file
#    urlbase:    The urlbase parameter of the installation
#                bugs are being moved from
#
sub init() {
    my ( $twig, $bugzilla ) = @_;
    my $root       = $twig->root;
    my $maintainer = $root->{'att'}->{'maintainer'};
    my $exporter   = $root->{'att'}->{'exporter'};
    my $urlbase    = $root->{'att'}->{'urlbase'};
    my $xmlversion = $root->{'att'}->{'version'};

    if ($xmlversion ne BUGZILLA_VERSION) {
            my $log = "Possible version conflict!\n";
            $log .= "   XML was exported from Bugzilla version $xmlversion\n";
            $log .= "   But this installation uses ";
            $log .= BUGZILLA_VERSION . "\n";
            Debug($log, OK_LEVEL);
            push(@logs, $log);
    }
    Error( "no maintainer", "REOPEN", $exporter ) unless ($maintainer);
    Error( "no exporter",   "REOPEN", $exporter ) unless ($exporter);
    Error( "bug importing is disabled here", undef, $exporter ) unless ( $params->{"move-enabled"} );
    Error( "invalid exporter: $exporter", "REOPEN", $exporter ) if ( !login_to_id($exporter) );
    Error( "no urlbase set", "REOPEN", $exporter ) unless ($urlbase);
    my $def_product =
        new Bugzilla::Product( { name => $params->{"moved-default-product"} } )
        || Error("Cannot import these bugs because an invalid default 
                  product was defined for the target db."
                  . $params->{"maintainer"} . " needs to fix the definitions of
                  moved-default-product. \n", "REOPEN", $exporter);
    my $def_component = new Bugzilla::Component(
        {
            product => $def_product,
            name    => $params->{"moved-default-component"}
        })
    || Error("Cannot import these bugs because an invalid default 
              component was defined for the target db."
              . $params->{"maintainer"} . " needs to fix the definitions of
              moved-default-component.\n", "REOPEN", $exporter);
}
    

# Parse attachments.
#
# This subroutine is called once for each attachment in the xml file.
# It is called as soon as the closing </attachment> tag is parsed.
# Since attachments have the potential to be very large, and
# since each attachment will be inside <bug>..</bug> tags we shove
# the attachment onto an array which will be processed by process_bug
# and then disposed of. The attachment array will then contain only
# one bugs' attachments at a time.
# The cycle will then repeat for the next <bug>
#
# The attach_id is ignored since mysql generates a new one for us.
# The submitter_id gets filled in with $exporterid.

sub process_attachment() {
    my ( $twig, $attach ) = @_;
    Debug( "Parsing attachments", DEBUG_LEVEL );
    my %attachment;

    $attachment{'date'} =
        format_time( $attach->field('date'), "%Y-%m-%d %R" ) || $timestamp;
    $attachment{'desc'}       = $attach->field('desc');
    $attachment{'ctype'}      = $attach->field('type') || "unknown/unknown";
    $attachment{'attachid'}   = $attach->field('attachid');
    $attachment{'ispatch'}    = $attach->{'att'}->{'ispatch'} || 0;
    $attachment{'isobsolete'} = $attach->{'att'}->{'isobsolete'} || 0;
    $attachment{'isprivate'}  = $attach->{'att'}->{'isprivate'} || 0;
    $attachment{'filename'}   = $attach->field('filename') || "file";
    # Attachment data is not exported in versions 2.20 and older.
    if (defined $attach->first_child('data') &&
            defined $attach->first_child('data')->{'att'}->{'encoding'}) {
        my $encoding = $attach->first_child('data')->{'att'}->{'encoding'};
        if ($encoding =~ /base64/) {
            # decode the base64
            my $data   = $attach->field('data');
            my $output = decode_base64($data);
            $attachment{'data'} = $output;
        }
        elsif ($encoding =~ /filename/) {
            # read the attachment file
            Error("attach_path is required", undef) unless ($attach_path);
            my $attach_filename = $attach_path . "/" . $attach->field('data');
            open(ATTACH_FH, $attach_filename) or
                Error("cannot open $attach_filename", undef);
            $attachment{'data'} = do { local $/; <ATTACH_FH> };
            close ATTACH_FH;
        }
    }
    else {
        $attachment{'data'} = $attach->field('data');
    }

    # attachment flags
    my @aflags;
    foreach my $aflag ( $attach->children('flag') ) {
        my %aflag;
        $aflag{'name'}      = $aflag->{'att'}->{'name'};
        $aflag{'status'}    = $aflag->{'att'}->{'status'};
        $aflag{'setter'}    = $aflag->{'att'}->{'setter'};
        $aflag{'requestee'} = $aflag->{'att'}->{'requestee'};
        push @aflags, \%aflag;
    }
    $attachment{'flags'} = \@aflags if (@aflags);

    # free up the memory for use by the rest of the script
    $attach->delete;
    if ($attachment{'attachid'}) {
        push @attachments, \%attachment;
    }
    else {
        push @attachments, "err";
    }
}

# This subroutine will be called once for each <bug> in the xml file.
# It is called as soon as the closing </bug> tag is parsed.
# If this bug had any <attachment> tags, they will have been processed
# before we get to this point and their data will be in the @attachments
# array.
# As each bug is processed, it is inserted into the database and then
# purged from memory to free it up for later bugs.

sub process_bug {
    my ( $twig, $bug ) = @_;
    my $root             = $twig->root;
    my $maintainer       = $root->{'att'}->{'maintainer'};
    my $exporter_login   = $root->{'att'}->{'exporter'};
    my $exporter         = new Bugzilla::User({ name => $exporter_login });
    my $urlbase          = $root->{'att'}->{'urlbase'};

    # We will store output information in this variable.
    my $log = "";
    if ( defined $bug->{'att'}->{'error'} ) {
        $log .= "\nError in bug " . $bug->field('bug_id') . "\@$urlbase: ";
        $log .= $bug->{'att'}->{'error'} . "\n";
        if ( $bug->{'att'}->{'error'} =~ /NotFound/ ) {
            $log .= "$exporter_login tried to move bug " . $bug->field('bug_id');
            $log .= " here, but $urlbase reports that this bug";
            $log .= " does not exist.\n";
        }
        elsif ( $bug->{'att'}->{'error'} =~ /NotPermitted/ ) {
            $log .= "$exporter_login tried to move bug " . $bug->field('bug_id');
            $log .= " here, but $urlbase reports that $exporter_login does ";
            $log .= " not have access to that bug.\n";
        }
        return;
    }
    $bugtotal++;

    # This list contains all other bug fields that we want to process.
    # If it is not in this list it will not be included.
    my %all_fields;
    foreach my $field ( 
        qw(long_desc attachment flag group), Bugzilla::Bug::fields() )
    {
        $all_fields{$field} = 1;
    }
    
    my %bug_fields;
    my $err = "";

   # Loop through all the xml tags inside a <bug> and compare them to the
   # lists of fields. If they match throw them into the hash. Otherwise
   # append it to the log, which will go into the comments when we are done.
    foreach my $bugchild ( $bug->children() ) {
        Debug( "Parsing field: " . $bugchild->name, DEBUG_LEVEL );
        if ( defined $all_fields{ $bugchild->name } ) {
              $bug_fields{ $bugchild->name } =
                  join( " ", $bug->children_text( $bugchild->name ) );
        }
        else {
            $err .= "Unknown bug field \"" . $bugchild->name . "\"";
            $err .= " encountered while moving bug\n";
            $err .= "   <" . $bugchild->name . ">";
            if ( $bugchild->children_count > 1 ) {
                $err .= "\n";
                foreach my $subchild ( $bugchild->children() ) {
                    $err .= "     <" . $subchild->name . ">";
                    $err .= $subchild->field;
                    $err .= "</" . $subchild->name . ">\n";
                }
            }
            else {
                $err .= $bugchild->field;
            }
            $err .= "</" . $bugchild->name . ">\n";
        }
    }

    my @long_descs;
    my $private = 0;

    # Parse long descriptions
    foreach my $comment ( $bug->children('long_desc') ) {
        Debug( "Parsing Long Description", DEBUG_LEVEL );
        my %long_desc;
        $long_desc{'who'}       = $comment->field('who');
        $long_desc{'bug_when'}  = $comment->field('bug_when');
        $long_desc{'isprivate'} = $comment->{'att'}->{'isprivate'} || 0;

        # if one of the comments is private we need to set this flag
        if ( $long_desc{'isprivate'} && $exporter->in_group($params->{'insidergroup'})) {
            $private = 1;
        }
        my $data = $comment->field('thetext');
        if ( defined $comment->first_child('thetext')->{'att'}->{'encoding'}
            && $comment->first_child('thetext')->{'att'}->{'encoding'} =~
            /base64/ )
        {
            $data = decode_base64($data);
        }

        # If we leave the attachment ID in the comment it will be made a link
        # to the wrong attachment. Since the new attachment ID is unknown yet
        # let's strip it out for now. We will make a comment with the right ID
        # later
        $data =~ s/Created an attachment \(id=\d+\)/Created an attachment/g;

        # Same goes for bug #'s Since we don't know if the referenced bug
        # is also being moved, lets make sure they know it means a different
        # bugzilla.
        my $url = $urlbase . "show_bug.cgi?id=";
        $data =~ s/([Bb]ugs?\s*\#?\s*(\d+))/$url$2/g;

        $long_desc{'thetext'} = $data;
        push @long_descs, \%long_desc;
    }

    # instead of giving each comment its own item in the longdescs
    # table like it should have, lets cat them all into one big
    # comment otherwise we would have to lie often about who
    # authored the comment since commenters in one bugzilla probably
    # don't have accounts in the other one.
    # If one of the comments is private the whole comment will be
    # private since we don't want to expose these unnecessarily
    sub by_date { my @a; my @b; $a->{'bug_when'} cmp $b->{'bug_when'}; }
    my @sorted_descs     = sort by_date @long_descs;
    my $long_description = "";
    for ( my $z = 0 ; $z <= $#sorted_descs ; $z++ ) {
        if ( $z == 0 ) {
            $long_description .= "\n\n\n---- Reported by ";
        }
        else {
            $long_description .= "\n\n\n---- Additional Comments From ";
        }
        $long_description .= "$sorted_descs[$z]->{'who'} ";
        $long_description .= "$sorted_descs[$z]->{'bug_when'}";
        $long_description .= " ----";
        $long_description .= "\n\n";
        $long_description .= "THIS COMMENT IS PRIVATE \n"
          if ( $sorted_descs[$z]->{'isprivate'} );
        $long_description .= $sorted_descs[$z]->{'thetext'};
        $long_description .= "\n";
    }

    my $comments;

    $comments .= "\n\n--- Bug imported by $exporter_login ";
    $comments .= time2str( "%Y-%m-%d %H:%M", time ) . " ";
    $comments .= $params->{'timezone'};
    $comments .= " ---\n\n";
    $comments .= "This bug was previously known as _bug_ $bug_fields{'bug_id'} at ";
    $comments .= $urlbase . "show_bug.cgi?id=" . $bug_fields{'bug_id'} . "\n";
    if ( defined $bug_fields{'dependson'} ) {
        $comments .= "This bug depended on bug(s) $bug_fields{'dependson'}.\n";
    }
    if ( defined $bug_fields{'blocked'} ) {
        $comments .= "This bug blocked bug(s) $bug_fields{'blocked'}.\n";
    }

    # Now we process each of the fields in turn and make sure they contain
    # valid data. We will create two parallel arrays, one for the query
    # and one for the values. For every field we need to push an entry onto
    # each array.
    my @query  = ();
    my @values = ();

    # Each of these fields we will check for newlines and shove onto the array
    foreach my $field (qw(status_whiteboard bug_file_loc short_desc)) {
        if (( defined $bug_fields{$field} ) && ( $bug_fields{$field} )) {
            $bug_fields{$field} = clean_text( $bug_fields{$field} );
            push( @query,  $field );
            push( @values, $bug_fields{$field} );
        }
    }

    # Alias
    if ( $bug_fields{'alias'} ) {
        my ($alias) = $dbh->selectrow_array("SELECT COUNT(*) FROM bugs 
                                                WHERE alias = ?", undef,
                                                $bug_fields{'alias'} );
        if ($alias) {
            $err .= "Dropping conflicting bug alias ";
            $err .= $bug_fields{'alias'} . "\n";
        }
        else {
            $alias = $bug_fields{'alias'};
            push @query,  'alias';
            push @values, $alias;
        }
    }

    # Timestamps
    push( @query, "creation_ts" );
    push( @values,
        format_time( $bug_fields{'creation_ts'}, "%Y-%m-%d %X" )
          || $timestamp );

    push( @query, "delta_ts" );
    push( @values,
        format_time( $bug_fields{'delta_ts'}, "%Y-%m-%d %X" )
          || $timestamp );

    # Bug Access
    push( @query,  "cclist_accessible" );
    push( @values, $bug_fields{'cclist_accessible'} ? 1 : 0 );

    push( @query,  "reporter_accessible" );
    push( @values, $bug_fields{'reporter_accessible'} ? 1 : 0 );

    # Product and Component if there is no valid default product and
    # component defined in the parameters, we wouldn't be here
    my $def_product =
      new Bugzilla::Product( { name => $params->{"moved-default-product"} } );
    my $def_component = new Bugzilla::Component(
        {
            product => $def_product,
            name    => $params->{"moved-default-component"}
        }
    );
    my $product;
    my $component;

    if ( defined $bug_fields{'product'} ) {
        $product = new Bugzilla::Product( { name => $bug_fields{'product'} } );
        unless ($product) {
            $product = $def_product;
            $err .= "Unknown Product " . $bug_fields{'product'} . "\n";
            $err .= "   Using default product set in Parameters \n";
        }
    }
    else {
        $product = $def_product;
    }
    if ( defined $bug_fields{'component'} ) {
        $component = new Bugzilla::Component(
            {
                product => $product,
                name    => $bug_fields{'component'}
            }
        );
        unless ($component) {
            $component = $def_component;
            $product   = $def_product;
            $err .= "Unknown Component " . $bug_fields{'component'} . "\n";
            $err .= "   Using default product and component set ";
            $err .= "in Parameters \n";
        }
    }
    else {
        $component = $def_component;
        $product   = $def_product;
    }

    my $prod_id = $product->id;
    my $comp_id = $component->id;

    push( @query,  "product_id" );
    push( @values, $prod_id );
    push( @query,  "component_id" );
    push( @values, $comp_id );

    # Since there is no default version for a product, we check that the one
    # coming over is valid. If not we will use the first one in @versions
    # and warn them.
    my $version = new Bugzilla::Version(
          { product => $product, name => $bug_fields{'version'} });

    push( @query, "version" );
    if ($version) {
        push( @values, $version->name );
    }
    else {
        my @versions = @{ $product->versions };
        my $v        = $versions[0];
        push( @values, $v->name );
        $err .= "Unknown version \"";
        $err .= ( defined $bug_fields{'version'} )
            ? $bug_fields{'version'}
            : "unknown";
        $err .= " in product " . $product->name . ". \n";
        $err .= "   Setting version to \"" . $v->name . "\".\n";
    }

    # Milestone
    if ( $params->{"usetargetmilestone"} ) {
        my $milestone;
        if (defined $bug_fields{'target_milestone'}
            && $bug_fields{'target_milestone'} ne "") {

            $milestone = new Bugzilla::Milestone(
                { product => $product, name => $bug_fields{'target_milestone'} });
        }
        if ($milestone) {
            push( @values, $milestone->name );
        }
        else {
            push( @values, $product->default_milestone );
            $err .= "Unknown milestone \"";
            $err .= ( defined $bug_fields{'target_milestone'} )
                ? $bug_fields{'target_milestone'}
                : "unknown";
            $err .= " in product " . $product->name . ". \n";
            $err .= "   Setting to default milestone for this product, ";
            $err .= "\"" . $product->default_milestone . "\".\n";
        }
        push( @query, "target_milestone" );
    }

    # For priority, severity, opsys and platform we check that the one being
    # imported is valid. If it is not we use the defaults set in the parameters.
    if (defined( $bug_fields{'bug_severity'} )
        && check_field('bug_severity', scalar $bug_fields{'bug_severity'},
                       undef, ERR_LEVEL) )
    {
        push( @values, $bug_fields{'bug_severity'} );
    }
    else {
        push( @values, $params->{'defaultseverity'} );
        $err .= "Unknown severity ";
        $err .= ( defined $bug_fields{'bug_severity'} )
          ? $bug_fields{'bug_severity'}
          : "unknown";
        $err .= ". Setting to default severity \"";
        $err .= $params->{'defaultseverity'} . "\".\n";
    }
    push( @query, "bug_severity" );

    if (defined( $bug_fields{'priority'} )
        && check_field('priority', scalar $bug_fields{'priority'},
                       undef, ERR_LEVEL ) )
    {
        push( @values, $bug_fields{'priority'} );
    }
    else {
        push( @values, $params->{'defaultpriority'} );
        $err .= "Unknown priority ";
        $err .= ( defined $bug_fields{'priority'} )
          ? $bug_fields{'priority'}
          : "unknown";
        $err .= ". Setting to default priority \"";
        $err .= $params->{'defaultpriority'} . "\".\n";
    }
    push( @query, "priority" );

    if (defined( $bug_fields{'rep_platform'} )
        && check_field('rep_platform', scalar $bug_fields{'rep_platform'},
                       undef, ERR_LEVEL ) )
    {
        push( @values, $bug_fields{'rep_platform'} );
    }
    else {
        push( @values, $params->{'defaultplatform'} );
        $err .= "Unknown platform ";
        $err .= ( defined $bug_fields{'rep_platform'} )
          ? $bug_fields{'rep_platform'}
          : "unknown";
        $err .=". Setting to default platform \"";
        $err .= $params->{'defaultplatform'} . "\".\n";
    }
    push( @query, "rep_platform" );

    if (defined( $bug_fields{'op_sys'} )
        && check_field('op_sys',  scalar $bug_fields{'op_sys'},
                       undef, ERR_LEVEL ) )
    {
        push( @values, $bug_fields{'op_sys'} );
    }
    else {
        push( @values, $params->{'defaultopsys'} );
        $err .= "Unknown operating system ";
        $err .= ( defined $bug_fields{'op_sys'} )
          ? $bug_fields{'op_sys'}
          : "unknown";
        $err .= ". Setting to default OS \"" . $params->{'defaultopsys'} . "\".\n";
    }
    push( @query, "op_sys" );

    # Process time fields
    if ( $params->{"timetrackinggroup"} ) {
        my $date = format_time( $bug_fields{'deadline'}, "%Y-%m-%d" )
          || undef;
        push( @values, $date );
        push( @query,  "deadline" );
        if ( defined $bug_fields{'estimated_time'} ) {
            eval {
                Bugzilla::Bug::ValidateTime($bug_fields{'estimated_time'}, "e");
            };
            if (!$@){
                push( @values, $bug_fields{'estimated_time'} );
                push( @query,  "estimated_time" );
            }
        }
        if ( defined $bug_fields{'remaining_time'} ) {
            eval {
                Bugzilla::Bug::ValidateTime($bug_fields{'remaining_time'}, "r");
            };
            if (!$@){
                push( @values, $bug_fields{'remaining_time'} );
                push( @query,  "remaining_time" );
            }
        }
        if ( defined $bug_fields{'actual_time'} ) {
            eval {
                Bugzilla::Bug::ValidateTime($bug_fields{'actual_time'}, "a");
            };
            if ($@){
                $bug_fields{'actual_time'} = 0.0;
                $err .= "Invalid Actual Time. Setting to 0.0\n";
            }
        }
        else {
            $bug_fields{'actual_time'} = 0.0;
            $err .= "Actual time not defined. Setting to 0.0\n";
        }
    }

    # Reporter Assignee QA Contact
    my $exporterid = $exporter->id;
    my $reporterid = login_to_id( $bug_fields{'reporter'} )
      if $bug_fields{'reporter'};
    push( @query, "reporter" );
    if ( ( $bug_fields{'reporter'} ) && ($reporterid) ) {
        push( @values, $reporterid );
    }
    else {
        push( @values, $exporterid );
        $err .= "The original reporter of this bug does not have\n";
        $err .= "   an account here. Reassigning to the person who moved\n";
        $err .= "   it here: $exporter_login.\n";
        if ( $bug_fields{'reporter'} ) {
            $err .= "   Previous reporter was $bug_fields{'reporter'}.\n";
        }
        else {
            $err .= "   Previous reporter is unknown.\n";
        }
    }

    my $changed_owner = 0;
    my $owner;
    push( @query, "assigned_to" );
    if ( ( $bug_fields{'assigned_to'} )
        && ( $owner = login_to_id( $bug_fields{'assigned_to'} )) ) {
        push( @values, $owner );
    }
    else {
        push( @values, $component->default_assignee->id );
        $changed_owner = 1;
        $err .= "The original assignee of this bug does not have\n";
        $err .= "   an account here. Reassigning to the default assignee\n";
        $err .= "   for the component, ". $component->default_assignee->login .".\n";
        if ( $bug_fields{'assigned_to'} ) {
            $err .= "   Previous assignee was $bug_fields{'assigned_to'}.\n";
        }
        else {
            $err .= "   Previous assignee is unknown.\n";
        }
    }

    if ( $params->{"useqacontact"} ) {
        my $qa_contact;
        push( @query, "qa_contact" );
        if ( ( defined $bug_fields{'qa_contact'})
            && ( $qa_contact = login_to_id( $bug_fields{'qa_contact'} ) ) ) {
            push( @values, $qa_contact );
        }
        else {
            push( @values, $component->default_qa_contact->id || undef );
            if ($component->default_qa_contact->id){
                $err .= "Setting qa contact to the default for this product.\n";
                $err .= "   This bug either had no qa contact or an invalid one.\n";
            }
        }
    }

    # Status & Resolution
    my $has_res = defined($bug_fields{'resolution'});
    my $has_status = defined($bug_fields{'bug_status'});
    my $valid_res = check_field('resolution',  
                                  scalar $bug_fields{'resolution'}, 
                                  undef, ERR_LEVEL );
    my $valid_status = check_field('bug_status',  
                                  scalar $bug_fields{'bug_status'}, 
                                  undef, ERR_LEVEL );
    my $is_open = is_open_state($bug_fields{'bug_status'}); 
    my $status = $bug_fields{'bug_status'} || undef;
    my $resolution = $bug_fields{'resolution'} || undef;
    
    # Check everconfirmed 
    my $everconfirmed;
    if ($product->votes_to_confirm) {
        $everconfirmed = $bug_fields{'everconfirmed'} || 0;
    }
    else {
        $everconfirmed = 1;
    }
    push (@query,  "everconfirmed");
    push (@values, $everconfirmed);

    # Sanity check will complain about having bugs marked duplicate but no
    # entry in the dup table. Since we can't tell the bug ID of bugs
    # that might not yet be in the database we have no way of populating
    # this table. Change the resolution instead.
    if ( $valid_res  && ( $bug_fields{'resolution'} eq "DUPLICATE" ) ) {
        $resolution = "MOVED";
        $err .= "This bug was marked DUPLICATE in the database ";
        $err .= "it was moved from.\n    Changing resolution to \"MOVED\"\n";
    } 
    
    if($has_status){
        if($valid_status){
            if($is_open){
                if($has_res){
                    $err .= "Resolution set on an open status.\n";
                    $err .= "   Dropping resolution $resolution\n";
                    $resolution = undef;
                }
                if($changed_owner){
                    if($everconfirmed){  
                        $status = "NEW";
                    }
                    else{
                        $status = "UNCONFIRMED";
                    }
                    if ($status ne $bug_fields{'bug_status'}){
                        $err .= "Bug reassigned, setting status to \"$status\".\n";
                        $err .= "   Previous status was \"";
                        $err .=  $bug_fields{'bug_status'} . "\".\n";
                    }
                }
                if($everconfirmed){
                    if($status eq "UNCONFIRMED"){
                        $err .= "Bug Status was UNCONFIRMED but everconfirmed was true\n";
                        $err .= "   Setting status to NEW\n";
                        $err .= "Resetting votes to 0\n" if ( $bug_fields{'votes'} );
                        $status = "NEW";
                    }
                }
                else{ # $everconfirmed is false
                    if($status ne "UNCONFIRMED"){
                        $err .= "Bug Status was $status but everconfirmed was false\n";
                        $err .= "   Setting status to UNCONFIRMED\n";
                        $status = "UNCONFIRMED";
                    }
                }
            }
            else{ # $is_open is false
               if(!$has_res){
                   $err .= "Missing Resolution. Setting status to ";
                   if($everconfirmed){
                       $status = "NEW";
                       $err .= "NEW\n";
                   }
                   else{
                       $status = "UNCONFIRMED";
                       $err .= "UNCONFIRMED\n";
                   }
               }
               if(!$valid_res){
                   $err .= "Unknown resolution \"$resolution\".\n";
                   $err .= "   Setting resolution to MOVED\n";
                   $resolution = "MOVED";
               }
            }   
        }
        else{ # $valid_status is false
            if($everconfirmed){  
                $status = "NEW";
            }
            else{
                $status = "UNCONFIRMED";
            }        
            $err .= "Bug has invalid status, setting status to \"$status\".\n";
            $err .= "   Previous status was \"";
            $err .=  $bug_fields{'bug_status'} . "\".\n";
            $resolution = undef;
        }
                
    }
    else{ #has_status is false
        if($everconfirmed){  
            $status = "NEW";
        }
        else{
            $status = "UNCONFIRMED";
        }        
        $err .= "Bug has no status, setting status to \"$status\".\n";
        $err .= "   Previous status was unknown\n";
        $resolution = undef;
    }
                                 
    if (defined $resolution){
        push( @query,  "resolution" );
        push( @values, $resolution );
    }
    
    # Bug status
    push( @query,  "bug_status" );
    push( @values, $status );

    # Custom fields
    foreach my $custom_field (Bugzilla->custom_field_names) {
        next unless defined($bug_fields{$custom_field});
        my $field = new Bugzilla::Field({name => $custom_field});
        if ($field->type == FIELD_TYPE_FREETEXT) {
            push(@query, $custom_field);
            push(@values, clean_text($bug_fields{$custom_field}));
        } elsif ($field->type == FIELD_TYPE_SINGLE_SELECT) {
            my $is_well_formed = check_field($custom_field, scalar $bug_fields{$custom_field},
                                             undef, ERR_LEVEL);
            if ($is_well_formed) {
                push(@query, $custom_field);
                push(@values, $bug_fields{$custom_field});
            } else {
                $err .= "Skipping illegal value \"$bug_fields{$custom_field}\" in $custom_field.\n" ;
            }
        } else {
            $err .= "Type of custom field $custom_field is an unhandled FIELD_TYPE: " .
                    $field->type . "\n";
        }
    }

    # For the sake of sanitycheck.cgi we do this.
    # Update lastdiffed if you do not want to have mail sent
    unless ($mail) {
        push @query,  "lastdiffed";
        push @values, $timestamp;
    }

    # INSERT the bug
    my $query = "INSERT INTO bugs (" . join( ", ", @query ) . ") VALUES (";
       $query .= '?,' foreach (@values);
    chop($query);    # Remove the last comma.
       $query .= ")";

    $dbh->do( $query, undef, @values );
    my $id = $dbh->bz_last_key( 'bugs', 'bug_id' );

    # We are almost certain to get some uninitialized warnings
    # Since this is just for debugging the query, let's shut them up
    eval {
        no warnings 'uninitialized';
        Debug(
            "Bug Query: INSERT INTO bugs (\n"
              . join( ",\n", @query )
              . "\n) VALUES (\n"
              . join( ",\n", @values ),
            DEBUG_LEVEL
        );
    };

    # Handle CC's
    if ( defined $bug_fields{'cc'} ) {
        my %ccseen;
        my $sth_cc = $dbh->prepare("INSERT INTO cc (bug_id, who) VALUES (?,?)");
        foreach my $person ( split( /[\s,]+/, $bug_fields{'cc'} ) ) {
            next unless $person;
            my $uid;
            if ($uid = login_to_id($person)) {
                if ( !$ccseen{$uid} ) {
                    $sth_cc->execute( $id, $uid );
                    $ccseen{$uid} = 1;
                }
            }
            else {
                $err .= "CC member $person does not have an account here\n";
            }
        }
    }

    # Handle keywords
    if ( defined( $bug_fields{'keywords'} ) ) {
        my %keywordseen;
        my $key_sth = $dbh->prepare(
            "INSERT INTO keywords 
                      (bug_id, keywordid) VALUES (?,?)"
        );
        foreach my $keyword ( split( /[\s,]+/, $bug_fields{'keywords'} )) {
            next unless $keyword;
            my $keyword_obj = new Bugzilla::Keyword({name => $keyword});
            if (!$keyword_obj) {
                $err .= "Skipping unknown keyword: $keyword.\n";
                next;
            }
            if (!$keywordseen{$keyword_obj->id}) {
                $key_sth->execute($id, $keyword_obj->id);
                $keywordseen{$keyword_obj->id} = 1;
            }
        }
        my ($keywordarray) = $dbh->selectcol_arrayref(
            "SELECT d.name FROM keyworddefs d
                    INNER JOIN keywords k 
                    ON d.id = k.keywordid 
                    WHERE k.bug_id = ? 
                    ORDER BY d.name", undef, $id);
        my $keywordstring = join( ", ", @{$keywordarray} );
        $dbh->do( "UPDATE bugs SET keywords = ? WHERE bug_id = ?",
            undef, $keywordstring, $id )
    }

    # Parse bug flags
    foreach my $bflag ( $bug->children('flag')) {
        next unless ( defined($bflag) );
        $err .= flag_handler(
            $bflag->{'att'}->{'name'},   $bflag->{'att'}->{'status'},
            $bflag->{'att'}->{'setter'}, $bflag->{'att'}->{'requestee'},
            $exporterid,                 $id,
            $comp_id,                    $prod_id,
            undef
        );
    }

    # Insert Attachments for the bug
    foreach my $att (@attachments) {
        if ($att eq "err"){
            $err .= "No attachment ID specified, dropping attachment\n";
            next;
        }
        if (!$exporter->in_group($params->{'insidergroup'}) && $att->{'isprivate'}){
            $err .= "Exporter not in insidergroup and attachment marked private.\n";
            $err .= "   Marking attachment public\n";
            $att->{'isprivate'} = 0;
        }
        $dbh->do("INSERT INTO attachments 
                 (bug_id, creation_ts, filename, description, mimetype, 
                 ispatch, isprivate, isobsolete, submitter_id) 
                 VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
            undef, $id, $att->{'date'}, $att->{'filename'},
            $att->{'desc'}, $att->{'ctype'}, $att->{'ispatch'},
            $att->{'isprivate'}, $att->{'isobsolete'}, $exporterid);
        my $att_id   = $dbh->bz_last_key( 'attachments', 'attach_id' );
        my $att_data = $att->{'data'};
        my $sth = $dbh->prepare("INSERT INTO attach_data (id, thedata) 
                                 VALUES ($att_id, ?)" );
        trick_taint($att_data);
        $sth->bind_param( 1, $att_data, $dbh->BLOB_TYPE );
        $sth->execute();
        $comments .= "Imported an attachment (id=$att_id)\n";

        # Process attachment flags
        foreach my $aflag (@{ $att->{'flags'} }) {
            next unless defined($aflag) ;
            $err .= flag_handler(
                $aflag->{'name'},   $aflag->{'status'},
                $aflag->{'setter'}, $aflag->{'requestee'},
                $exporterid,        $id,
                $comp_id,           $prod_id,
                $att_id
            );
        }
    }

    # Clear the attachments array for the next bug
    @attachments = ();

    # Insert longdesc and append any errors
    my $worktime = $bug_fields{'actual_time'} || 0.0;
    $worktime = 0.0 if (!$exporter->in_group($params->{'timetrackinggroup'}));
    $long_description .= "\n" . $comments;
    if ($err) {
        $long_description .= "\n$err\n";
    }
    trick_taint($long_description);
    $dbh->do("INSERT INTO longdescs 
                     (bug_id, who, bug_when, work_time, isprivate, thetext) 
                     VALUES (?,?,?,?,?,?)", undef,
        $id, $exporterid, $timestamp, $worktime, $private, $long_description
    );

    # Add this bug to each group of which its product is a member.
    my $sth_group = $dbh->prepare("INSERT INTO bug_group_map (bug_id, group_id) 
                         VALUES (?, ?)");
    foreach my $group_id ( keys %{ $product->group_controls } ) {
        if ($product->group_controls->{$group_id}->{'membercontrol'} != CONTROLMAPNA
            && $product->group_controls->{$group_id}->{'othercontrol'} != CONTROLMAPNA){
            $sth_group->execute( $id, $group_id );
        }
    }

    $log .= "Bug ${urlbase}show_bug.cgi?id=$bug_fields{'bug_id'} ";
    $log .= "imported as bug $id.\n";
    $log .= $params->{"urlbase"} . "show_bug.cgi?id=$id\n\n";
    if ($err) {
        $log .= "The following problems were encountered while creating bug $id.\n";
        $log .= $err;
        $log .= "You may have to set certain fields in the new bug by hand.\n\n";
    }
    Debug( $log, OK_LEVEL );
    push(@logs, $log);
    Bugzilla::BugMail::Send( $id, { 'changer' => $exporter_login } ) if ($mail);

    # done with the xml data. Lets clear it from memory
    $twig->purge;

}

Debug( "Reading xml", DEBUG_LEVEL );

# Read STDIN in slurp mode. VERY dangerous, but we live on the wild side ;-)
local ($/);
$xml = <>;

# If there's anything except whitespace before <?xml then we guess it's a mail
# and MIME::Parser should parse it. Else don't.
if ($xml =~ m/\S.*<\?xml/s ) {

    # If the email was encoded (Mailer::MessageToMTA() does it when using UTF-8),
    # we have to decode it first, else the XML parsing will fail.
    my $parser = MIME::Parser->new;
    $parser->output_to_core(1);
    $parser->tmp_to_core(1);
    my $entity = $parser->parse_data($xml);
    my $bodyhandle = $entity->bodyhandle;
    $xml = $bodyhandle->as_string;

}

# remove everything in file before xml header
$xml =~ s/^.+(<\?xml version.+)$/$1/s;

Debug( "Parsing tree", DEBUG_LEVEL );
my $twig = XML::Twig->new(
    twig_handlers => {
        bug        => \&process_bug,
        attachment => \&process_attachment
    },
    start_tag_handlers => { bugzilla => \&init }
);
$twig->parse($xml);
my $root       = $twig->root;
my $maintainer = $root->{'att'}->{'maintainer'};
my $exporter   = $root->{'att'}->{'exporter'};
my $urlbase    = $root->{'att'}->{'urlbase'};

# It is time to email the result of the import.
my $log = join("\n\n", @logs);
$log .=  "\n\nImported $bugtotal bug(s) from $urlbase,\n  sent by $exporter.\n";
my $subject =  "$bugtotal Bug(s) successfully moved from $urlbase to " 
   . $params->{"urlbase"};
my @to = ($exporter, $maintainer);
MailMessage( $subject, $log, @to );

__END__

=head1 NAME

importxml - Import bugzilla bug data from xml.

=head1 SYNOPSIS

    importxml.pl [options] [file ...]

 Options:
       -? --help        brief help message
       -v --verbose     print error and debug information. 
                        Mulltiple -v increases verbosity
       -m --sendmail    send mail to recipients with log of bugs imported
       --attach_path    The path to the attachment files.
                        (Required if encoding="filename" is used for attachments.)

=head1 OPTIONS

=over 8

=item B<-?>

    Print a brief help message and exits.

=item B<-v>

    Print error and debug information. Mulltiple -v increases verbosity

=item B<-m>

    Send mail to exporter with a log of bugs imported and any errors.

=back

=head1 DESCRIPTION

     This script is used to import bugs from another installation of bugzilla.
     It can be used in two ways.
     First using the move function of bugzilla
     on another system will send mail to an alias provided by
     the administrator of the target installation (you). Set up an alias
     similar to the one given below so this mail will be automatically 
     run by this script and imported into your database.  Run 'newaliases'
     after adding this alias to your aliases file. Make sure your sendmail
     installation is configured to allow mail aliases to execute code. 

     bugzilla-import: "|/usr/bin/perl /opt/bugzilla/importxml.pl --mail"

     Second it can be run from the command line with any xml file from 
     STDIN that conforms to the bugzilla DTD. In this case you can pass 
     an argument to set whether you want to send the
     mail that will be sent to the exporter and maintainer normally.

     importxml.pl [options] bugsfile.xml

=cut

