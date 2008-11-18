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
# Contributor(s): Gervase Markham <gerv@gerv.net>
#                 Terry Weissman <terry@mozilla.org>
#                 Dan Mosedale <dmose@mozilla.org>
#                 Stephan Niemz <st.n@gmx.net>
#                 Andreas Franke <afranke@mathweb.org>
#                 Myk Melez <myk@mozilla.org>
#                 Michael Schindler <michael@compressconsult.com>
#                 Max Kanat-Alexander <mkanat@bugzilla.org>
#                 Joel Peshkin <bugreport@peshkin.net>
#                 Lance Larsh <lance.larsh@oracle.com>

use strict;

package Bugzilla::Search;
use base qw(Exporter);
@Bugzilla::Search::EXPORT = qw(IsValidQueryType);

use Bugzilla::Error;
use Bugzilla::Util;
use Bugzilla::Constants;
use Bugzilla::Group;
use Bugzilla::User;
use Bugzilla::Field;
use Bugzilla::Bug;
use Bugzilla::Keyword;

use Date::Format;
use Date::Parse;

# How much we should add to the relevance, for each word that matches
# in bugs.short_desc, during fulltext search. This is high because
# we want summary matches to be *very* relevant, by default.
use constant SUMMARY_RELEVANCE_FACTOR => 7;

# Some fields are not sorted on themselves, but on other fields. 
# We need to have a list of these fields and what they map to.
# Each field points to an array that contains the fields mapped 
# to, in order.
use constant SPECIAL_ORDER => {
    'bugs.target_milestone' => [ 'ms_order.sortkey','ms_order.value' ],
    'bugs.bug_status' => [ 'bug_status.sortkey','bug_status.value' ],
    'bugs.rep_platform' => [ 'rep_platform.sortkey','rep_platform.value' ],
    'bugs.priority' => [ 'priority.sortkey','priority.value' ],
    'bugs.op_sys' => [ 'op_sys.sortkey','op_sys.value' ],
    'bugs.resolution' => [ 'resolution.sortkey', 'resolution.value' ],
    'bugs.bug_severity' => [ 'bug_severity.sortkey','bug_severity.value' ]
};

# When we add certain fields to the ORDER BY, we need to then add a
# table join to the FROM statement. This hash maps input fields to 
# the join statements that need to be added.
use constant SPECIAL_ORDER_JOIN => {
    'bugs.target_milestone' => 'LEFT JOIN milestones AS ms_order ON ms_order.value = bugs.target_milestone AND ms_order.product_id = bugs.product_id',
    'bugs.bug_status' => 'LEFT JOIN bug_status ON bug_status.value = bugs.bug_status',
    'bugs.rep_platform' => 'LEFT JOIN rep_platform ON rep_platform.value = bugs.rep_platform',
    'bugs.priority' => 'LEFT JOIN priority ON priority.value = bugs.priority',
    'bugs.op_sys' => 'LEFT JOIN op_sys ON op_sys.value = bugs.op_sys',
    'bugs.resolution' => 'LEFT JOIN resolution ON resolution.value = bugs.resolution',
    'bugs.bug_severity' => 'LEFT JOIN bug_severity ON bug_severity.value = bugs.bug_severity'
};

# Create a new Search
# Note that the param argument may be modified by Bugzilla::Search
sub new {
    my $invocant = shift;
    my $class = ref($invocant) || $invocant;
  
    my $self = { @_ };
    bless($self, $class);

    $self->init();
 
    return $self;
}

sub init {
    my $self = shift;
    my $fieldsref = $self->{'fields'};
    my $params = $self->{'params'};
    my $user = $self->{'user'} || Bugzilla->user;

    my $orderref = $self->{'order'} || 0;
    my @inputorder;
    @inputorder = @$orderref if $orderref;
    my @orderby;

    my $debug = 0;
    my @debugdata;
    if ($params->param('debug')) { $debug = 1; }

    my @fields;
    my @supptables;
    my @wherepart;
    my @having;
    my @groupby;
    @fields = @$fieldsref if $fieldsref;
    my @specialchart;
    my @andlist;
    my %chartfields;

    my %special_order      = %{SPECIAL_ORDER()};
    my %special_order_join = %{SPECIAL_ORDER_JOIN()};

    my @select_fields = Bugzilla->get_fields({ type => FIELD_TYPE_SINGLE_SELECT,
                                               obsolete => 0 });
    foreach my $field (@select_fields) {
        my $name = $field->name;
        $special_order{"bugs.$name"} = [ "$name.sortkey", "$name.value" ],
        $special_order_join{"bugs.$name"} =
            "LEFT JOIN $name ON $name.value = bugs.$name";
    }

    my $dbh = Bugzilla->dbh;

    # First, deal with all the old hard-coded non-chart-based poop.
    if (grep(/map_assigned_to/, @$fieldsref)) {
        push @supptables, "INNER JOIN profiles AS map_assigned_to " .
                          "ON bugs.assigned_to = map_assigned_to.userid";
    }

    if (grep(/map_reporter/, @$fieldsref)) {
        push @supptables, "INNER JOIN profiles AS map_reporter " .
                          "ON bugs.reporter = map_reporter.userid";
    }

    if (grep(/map_qa_contact/, @$fieldsref)) {
        push @supptables, "LEFT JOIN profiles AS map_qa_contact " .
                          "ON bugs.qa_contact = map_qa_contact.userid";
    }

    if (lsearch($fieldsref, 'map_products.name') >= 0) {
        push @supptables, "INNER JOIN products AS map_products " .
                          "ON bugs.product_id = map_products.id";
    }

    if (lsearch($fieldsref, 'map_classifications.name') >= 0) {
        push @supptables, "INNER JOIN products AS map_products " .
                          "ON bugs.product_id = map_products.id";
        push @supptables,
                "INNER JOIN classifications AS map_classifications " .
                "ON map_products.classification_id = map_classifications.id";
    }

    if (lsearch($fieldsref, 'map_components.name') >= 0) {
        push @supptables, "INNER JOIN components AS map_components " .
                          "ON bugs.component_id = map_components.id";
    }
    
    if (grep($_ =~/AS (actual_time|percentage_complete)$/, @$fieldsref)) {
        push(@supptables, "LEFT JOIN longdescs AS ldtime " .
                          "ON ldtime.bug_id = bugs.bug_id");
    }

    my $minvotes;
    if (defined $params->param('votes')) {
        my $c = trim($params->param('votes'));
        if ($c ne "") {
            if ($c !~ /^[0-9]*$/) {
                ThrowUserError("illegal_at_least_x_votes",
                                  { value => $c });
            }
            push(@specialchart, ["votes", "greaterthan", $c - 1]);
        }
    }

    if ($params->param('bug_id')) {
        my $type = "anyexact";
        if ($params->param('bugidtype') && $params->param('bugidtype') eq 'exclude') {
            $type = "nowords";
        }
        push(@specialchart, ["bug_id", $type, join(',', $params->param('bug_id'))]);
    }

    # If the user has selected all of either status or resolution, change to
    # selecting none. This is functionally equivalent, but quite a lot faster.
    # Also, if the status is __open__ or __closed__, translate those
    # into their equivalent lists of open and closed statuses.
    if ($params->param('bug_status')) {
        my @bug_statuses = $params->param('bug_status');
        my @legal_statuses = @{get_legal_field_values('bug_status')};
        if (scalar(@bug_statuses) == scalar(@legal_statuses)
            || $bug_statuses[0] eq "__all__")
        {
            $params->delete('bug_status');
        }
        elsif ($bug_statuses[0] eq '__open__') {
            $params->param('bug_status', grep(is_open_state($_), 
                                              @legal_statuses));
        }
        elsif ($bug_statuses[0] eq "__closed__") {
            $params->param('bug_status', grep(!is_open_state($_), 
                                              @legal_statuses));
        }
    }
    
    if ($params->param('resolution')) {
        my @resolutions = $params->param('resolution');
        my $legal_resolutions = get_legal_field_values('resolution');
        if (scalar(@resolutions) == scalar(@$legal_resolutions)) {
            $params->delete('resolution');
        }
    }
    
    my @legal_fields = ("product", "version", "rep_platform", "op_sys",
                        "bug_status", "resolution", "priority", "bug_severity",
                        "assigned_to", "reporter", "component", "classification",
                        "target_milestone", "bug_group");

    # Include custom select fields.
    push(@legal_fields, map { $_->name } @select_fields);

    foreach my $field ($params->param()) {
        if (lsearch(\@legal_fields, $field) != -1) {
            push(@specialchart, [$field, "anyexact",
                                 join(',', $params->param($field))]);
        }
    }

    if ($params->param('keywords')) {
        my $t = $params->param('keywords_type');
        if (!$t || $t eq "or") {
            $t = "anywords";
        }
        push(@specialchart, ["keywords", $t, $params->param('keywords')]);
    }

    foreach my $id ("1", "2") {
        if (!defined ($params->param("email$id"))) {
            next;
        }
        my $email = trim($params->param("email$id"));
        if ($email eq "") {
            next;
        }
        my $type = $params->param("emailtype$id");
        if ($type eq "exact") {
            $type = "anyexact";
            foreach my $name (split(',', $email)) {
                $name = trim($name);
                if ($name) {
                    login_to_id($name, THROW_ERROR);
                }
            }
        }

        my @clist;
        foreach my $field ("assigned_to", "reporter", "cc", "qa_contact") {
            if ($params->param("email$field$id")) {
                push(@clist, $field, $type, $email);
            }
        }
        if ($params->param("emaillongdesc$id")) {
                push(@clist, "commenter", $type, $email);
        }
        if (@clist) {
            push(@specialchart, \@clist);
        } else {
            ThrowUserError("missing_email_type",
                           { email => $email });
        }
    }

    my $chfieldfrom = trim(lc($params->param('chfieldfrom'))) || '';
    my $chfieldto = trim(lc($params->param('chfieldto'))) || '';
    $chfieldfrom = '' if ($chfieldfrom eq 'now');
    $chfieldto = '' if ($chfieldto eq 'now');
    my @chfield = $params->param('chfield');
    my $chvalue = trim($params->param('chfieldvalue')) || '';

    # 2003-05-20: The 'changedin' field is no longer in the UI, but we continue
    # to process it because it will appear in stored queries and bookmarks.
    my $changedin = trim($params->param('changedin')) || '';
    if ($changedin) {
        if ($changedin !~ /^[0-9]*$/) {
            ThrowUserError("illegal_changed_in_last_x_days",
                              { value => $changedin });
        }

        if (!$chfieldfrom
            && !$chfieldto
            && scalar(@chfield) == 1
            && $chfield[0] eq "[Bug creation]")
        {
            # Deal with the special case where the query is using changedin
            # to get bugs created in the last n days by converting the value
            # into its equivalent for the chfieldfrom parameter.
            $chfieldfrom = "-" . ($changedin - 1) . "d";
        }
        else {
            # Oh boy, the general case.  Who knows why the user included
            # the changedin parameter, but do our best to comply.
            push(@specialchart, ["changedin", "lessthan", $changedin + 1]);
        }
    }

    if ($chfieldfrom ne '' || $chfieldto ne '') {
        my $sql_chfrom = $chfieldfrom ? $dbh->quote(SqlifyDate($chfieldfrom)):'';
        my $sql_chto   = $chfieldto   ? $dbh->quote(SqlifyDate($chfieldto))  :'';
        my $sql_chvalue = $chvalue ne '' ? $dbh->quote($chvalue) : '';
        trick_taint($sql_chvalue);
        if(!@chfield) {
            push(@wherepart, "bugs.delta_ts >= $sql_chfrom") if ($sql_chfrom);
            push(@wherepart, "bugs.delta_ts <= $sql_chto") if ($sql_chto);
        } else {
            my $bug_creation_clause;
            my @list;
            my @actlist;
            foreach my $f (@chfield) {
                if ($f eq "[Bug creation]") {
                    # Treat [Bug creation] differently because we need to look
                    # at bugs.creation_ts rather than the bugs_activity table.
                    my @l;
                    push(@l, "bugs.creation_ts >= $sql_chfrom") if($sql_chfrom);
                    push(@l, "bugs.creation_ts <= $sql_chto") if($sql_chto);
                    $bug_creation_clause = "(" . join(' AND ', @l) . ")";
                } else {
                    push(@actlist, get_field_id($f));
                }
            }

            # @actlist won't have any elements if the only field being searched
            # is [Bug creation] (in which case we don't need bugs_activity).
            if(@actlist) {
                my $extra = " actcheck.bug_id = bugs.bug_id";
                push(@list, "(actcheck.bug_when IS NOT NULL)");
                if($sql_chfrom) {
                    $extra .= " AND actcheck.bug_when >= $sql_chfrom";
                }
                if($sql_chto) {
                    $extra .= " AND actcheck.bug_when <= $sql_chto";
                }
                if($sql_chvalue) {
                    $extra .= " AND actcheck.added = $sql_chvalue";
                }
                push(@supptables, "LEFT JOIN bugs_activity AS actcheck " .
                                  "ON $extra AND actcheck.fieldid IN (" .
                                  join(",", @actlist) . ")");
            }

            # Now that we're done using @list to determine if there are any
            # regular fields to search (and thus we need bugs_activity),
            # add the [Bug creation] criterion to the list so we can OR it
            # together with the others.
            push(@list, $bug_creation_clause) if $bug_creation_clause;

            push(@wherepart, "(" . join(" OR ", @list) . ")");
        }
    }

    my $sql_deadlinefrom;
    my $sql_deadlineto;
    if (Bugzilla->user->in_group(Bugzilla->params->{'timetrackinggroup'})){
      my $deadlinefrom;
      my $deadlineto;
            
      if ($params->param('deadlinefrom')){
        $deadlinefrom = $params->param('deadlinefrom');
        validate_date($deadlinefrom)
          || ThrowUserError('illegal_date', {date => $deadlinefrom,
                                             format => 'YYYY-MM-DD'});
        $sql_deadlinefrom = $dbh->quote($deadlinefrom);
        trick_taint($sql_deadlinefrom);
        push(@wherepart, "bugs.deadline >= $sql_deadlinefrom");
      }
      
      if ($params->param('deadlineto')){
        $deadlineto = $params->param('deadlineto');
        validate_date($deadlineto)
          || ThrowUserError('illegal_date', {date => $deadlineto,
                                             format => 'YYYY-MM-DD'});
        $sql_deadlineto = $dbh->quote($deadlineto);
        trick_taint($sql_deadlineto);
        push(@wherepart, "bugs.deadline <= $sql_deadlineto");
      }
    }  

    foreach my $f ("short_desc", "long_desc", "bug_file_loc",
                   "status_whiteboard") {
        if (defined $params->param($f)) {
            my $s = trim($params->param($f));
            if ($s ne "") {
                my $n = $f;
                my $q = $dbh->quote($s);
                trick_taint($q);
                my $type = $params->param($f . "_type");
                push(@specialchart, [$f, $type, $s]);
            }
        }
    }

    if (defined $params->param('content')) {
        push(@specialchart, ['content', 'matches', $params->param('content')]);
    }

    my $chartid;
    my $sequence = 0;
    # $type_id is used by the code that queries for attachment flags.
    my $type_id = 0;
    my $f;
    my $ff;
    my $t;
    my $q;
    my $v;
    my $term;
    my %funcsbykey;
    my @funcdefs =
        (
         "^(?:assigned_to|reporter|qa_contact),(?:notequals|equals|anyexact),%group\\.([^%]+)%" => sub {
             my $group = $1;
             my $groupid = Bugzilla::Group::ValidateGroupName( $group, ($user));
             $groupid || ThrowUserError('invalid_group_name',{name => $group});
             my @childgroups = @{$user->flatten_group_membership($groupid)};
             my $table = "user_group_map_$chartid";
             push (@supptables, "LEFT JOIN user_group_map AS $table " .
                                "ON $table.user_id = bugs.$f " .
                                "AND $table.group_id IN(" .
                                join(',', @childgroups) . ") " .
                                "AND $table.isbless = 0 " .
                                "AND $table.grant_type IN(" .
                                GRANT_DIRECT . "," . GRANT_REGEXP . ")"
                  );
             if ($t =~ /^not/) {
                 $term = "$table.group_id IS NULL";
             } else {
                 $term = "$table.group_id IS NOT NULL";
             }
          },
         "^(?:assigned_to|reporter|qa_contact),(?:equals|anyexact),(%\\w+%)" => sub {
             $term = "bugs.$f = " . pronoun($1, $user);
          },
         "^(?:assigned_to|reporter|qa_contact),(?:notequals),(%\\w+%)" => sub {
             $term = "bugs.$f <> " . pronoun($1, $user);
          },
         "^(assigned_to|reporter),(?!changed)" => sub {
             my $list = $self->ListIDsForEmail($t, $v);
             if ($list) {
                 $term = "bugs.$f IN ($list)"; 
             } else {
                 push(@supptables, "INNER JOIN profiles AS map_$f " .
                                   "ON bugs.$f = map_$f.userid");
                 $f = "map_$f.login_name";
             }
         },
         "^qa_contact,(?!changed)" => sub {
             push(@supptables, "LEFT JOIN profiles AS map_qa_contact " .
                               "ON bugs.qa_contact = map_qa_contact.userid");
             $f = "COALESCE(map_$f.login_name,'')";
         },

         "^(?:cc),(?:notequals|equals|anyexact),%group\\.([^%]+)%" => sub {
             my $group = $1;
             my $groupid = Bugzilla::Group::ValidateGroupName( $group, ($user));
             $groupid || ThrowUserError('invalid_group_name',{name => $group});
             my @childgroups = @{$user->flatten_group_membership($groupid)};
             my $chartseq = $chartid;
             if ($chartid eq "") {
                 $chartseq = "CC$sequence";
                 $sequence++;
             }
             my $table = "user_group_map_$chartseq";
             push(@supptables, "LEFT JOIN cc AS cc_$chartseq " .
                               "ON bugs.bug_id = cc_$chartseq.bug_id");
             push(@supptables, "LEFT JOIN user_group_map AS $table " .
                                "ON $table.user_id = cc_$chartseq.who " .
                                "AND $table.group_id IN(" .
                                join(',', @childgroups) . ") " .
                                "AND $table.isbless = 0 " .
                                "AND $table.grant_type IN(" .
                                GRANT_DIRECT . "," . GRANT_REGEXP . ")"
                  );
             if ($t =~ /^not/) {
                 $term = "$table.group_id IS NULL";
             } else {
                 $term = "$table.group_id IS NOT NULL";
             }
          },

         "^cc,(?:equals|anyexact),(%\\w+%)" => sub {
             my $match = pronoun($1, $user);
             my $chartseq = $chartid;
             if ($chartid eq "") {
                 $chartseq = "CC$sequence";
                 $sequence++;
             }
             push(@supptables, "LEFT JOIN cc AS cc_$chartseq " .
                               "ON bugs.bug_id = cc_$chartseq.bug_id " .
                               "AND cc_$chartseq.who = $match");
             $term = "cc_$chartseq.who IS NOT NULL";
         },
         "^cc,(?:notequals),(%\\w+%)" => sub {
             my $match = pronoun($1, $user);
             my $chartseq = $chartid;
             if ($chartid eq "") {
                 $chartseq = "CC$sequence";
                 $sequence++;
             }
             push(@supptables, "LEFT JOIN cc AS cc_$chartseq " .
                               "ON bugs.bug_id = cc_$chartseq.bug_id " .
                               "AND cc_$chartseq.who = $match");
             $term = "cc_$chartseq.who IS NULL";
         },
         "^cc,(anyexact|substring|regexp)" => sub {
             my $list;
             $list = $self->ListIDsForEmail($t, $v);
             my $chartseq = $chartid;
             if ($chartid eq "") {
                 $chartseq = "CC$sequence";
                 $sequence++;
             }
             if ($list) {
                 push(@supptables, "LEFT JOIN cc AS cc_$chartseq " .
                                   "ON bugs.bug_id = cc_$chartseq.bug_id " .
                                   "AND cc_$chartseq.who IN($list)");
                 $term = "cc_$chartseq.who IS NOT NULL";
             } else {
                 push(@supptables, "LEFT JOIN cc AS cc_$chartseq " .
                                   "ON bugs.bug_id = cc_$chartseq.bug_id");
                 push(@supptables,
                            "LEFT JOIN profiles AS map_cc_$chartseq " .
                            "ON cc_$chartseq.who = map_cc_$chartseq.userid");

                 $ff = $f = "map_cc_$chartseq.login_name";
                 my $ref = $funcsbykey{",$t"};
                 &$ref;
             }
         },
         "^cc,(?!changed)" => sub {
             my $chartseq = $chartid;
             if ($chartid eq "") {
                 $chartseq = "CC$sequence";
                 $sequence++;
             }
            push(@supptables, "LEFT JOIN cc AS cc_$chartseq " .
                              "ON bugs.bug_id = cc_$chartseq.bug_id");

            $ff = $f = "map_cc_$chartseq.login_name";
            my $ref = $funcsbykey{",$t"};
            &$ref;
            push(@supptables, 
                        "LEFT JOIN profiles AS map_cc_$chartseq " .
                        "ON (cc_$chartseq.who = map_cc_$chartseq.userid " .
                        "AND ($term))"
                );
            $term = "$f IS NOT NULL";
         },

         "^long_?desc,changedby" => sub {
             my $table = "longdescs_$chartid";
             push(@supptables, "LEFT JOIN longdescs AS $table " .
                               "ON $table.bug_id = bugs.bug_id");
             my $id = login_to_id($v, THROW_ERROR);
             $term = "$table.who = $id";
         },
         "^long_?desc,changedbefore" => sub {
             my $table = "longdescs_$chartid";
             push(@supptables, "LEFT JOIN longdescs AS $table " .
                               "ON $table.bug_id = bugs.bug_id");
             $term = "$table.bug_when < " . $dbh->quote(SqlifyDate($v));
         },
         "^long_?desc,changedafter" => sub {
             my $table = "longdescs_$chartid";
             push(@supptables, "LEFT JOIN longdescs AS $table " .
                               "ON $table.bug_id = bugs.bug_id");
             $term = "$table.bug_when > " . $dbh->quote(SqlifyDate($v));
         },
         "^content,matches" => sub {
             # "content" is an alias for columns containing text for which we
             # can search a full-text index and retrieve results by relevance, 
             # currently just bug comments (and summaries to some degree).
             # There's only one way to search a full-text index, so we only
             # accept the "matches" operator, which is specific to full-text
             # index searches.

             # Add the longdescs table to the query so we can search comments.
             my $table = "longdescs_$chartid";
             my $extra = "";
             if (Bugzilla->params->{"insidergroup"} 
                 && !Bugzilla->user->in_group(Bugzilla->params->{"insidergroup"}))
             {
                 $extra = "AND $table.isprivate < 1";
             }
             push(@supptables, "LEFT JOIN longdescs AS $table " .
                               "ON bugs.bug_id = $table.bug_id $extra");

             # Create search terms to add to the SELECT and WHERE clauses.
             # $term1 searches comments.
             my $term1 = $dbh->sql_fulltext_search("${table}.thetext", $v);

             # short_desc searching for the WHERE clause
             my @words = _split_words_into_like('bugs.short_desc', $v);
             my $term2_where = join(' OR ', @words);

             # short_desc relevance
             my $factor = SUMMARY_RELEVANCE_FACTOR;
             my @s_words = map("CASE WHEN $_ THEN $factor ELSE 0 END", @words);
             my $term2_select = join(' + ', @s_words);

             # The term to use in the WHERE clause.
             $term = "$term1 > 0 OR ($term2_where)";

             # In order to sort by relevance (in case the user requests it),
             # we SELECT the relevance value and give it an alias so we can
             # add it to the SORT BY clause when we build it in buglist.cgi.
             #
             # Note: We should be calculating the relevance based on all
             # comments for a bug, not just matching comments, but that's hard
             # (see http://bugzilla.mozilla.org/show_bug.cgi?id=145588#c35).
             my $select_term = "(SUM($term1) + $term2_select) AS relevance";

             # add the column not used in aggregate function explicitly
             push(@groupby, 'bugs.short_desc');

             # Users can specify to display the relevance field, in which case
             # it'll show up in the list of fields being selected, and we need
             # to replace that occurrence with our select term.  Otherwise
             # we can just add the term to the list of fields being selected.
             if (grep($_ eq "relevance", @fields)) {
                 @fields = map($_ eq "relevance" ? $select_term : $_ , @fields);
             }
             else {
                 push(@fields, $select_term);
             }
         },
         "^content," => sub {
             ThrowUserError("search_content_without_matches");
         },
         "^(?:deadline|creation_ts|delta_ts),(?:lessthan|greaterthan|equals|notequals),(?:-|\\+)?(?:\\d+)(?:[dDwWmMyY])\$" => sub {
             $v = SqlifyDate($v);
             $q = $dbh->quote($v);
        },
         "^commenter,(?:equals|anyexact),(%\\w+%)" => sub {
             my $match = pronoun($1, $user);
             my $chartseq = $chartid;
             if ($chartid eq "") {
                 $chartseq = "LD$sequence";
                 $sequence++;
             }
             my $table = "longdescs_$chartseq";
             my $extra = "";
             if (Bugzilla->params->{"insidergroup"} 
                 && !Bugzilla->user->in_group(Bugzilla->params->{"insidergroup"})) 
             {
                 $extra = "AND $table.isprivate < 1";
             }
             push(@supptables, "LEFT JOIN longdescs AS $table " .
                               "ON $table.bug_id = bugs.bug_id $extra " .
                               "AND $table.who IN ($match)");
             $term = "$table.who IS NOT NULL";
         },
         "^commenter," => sub {
             my $chartseq = $chartid;
             my $list;
             $list = $self->ListIDsForEmail($t, $v);
             if ($chartid eq "") {
                 $chartseq = "LD$sequence";
                 $sequence++;
             }
             my $table = "longdescs_$chartseq";
             my $extra = "";
             if (Bugzilla->params->{"insidergroup"} 
                 && !Bugzilla->user->in_group(Bugzilla->params->{"insidergroup"})) 
             {
                 $extra = "AND $table.isprivate < 1";
             }
             if ($list) {
                 push(@supptables, "LEFT JOIN longdescs AS $table " .
                                   "ON $table.bug_id = bugs.bug_id $extra " .
                                   "AND $table.who IN ($list)");
                 $term = "$table.who IS NOT NULL";
             } else {
                 push(@supptables, "LEFT JOIN longdescs AS $table " .
                                   "ON $table.bug_id = bugs.bug_id $extra");
                 push(@supptables, "LEFT JOIN profiles AS map_$table " .
                                   "ON $table.who = map_$table.userid");
                 $ff = $f = "map_$table.login_name";
                 my $ref = $funcsbykey{",$t"};
                 &$ref;
             }
         },
         "^long_?desc," => sub {
             my $table = "longdescs_$chartid";
             my $extra = "";
             if (Bugzilla->params->{"insidergroup"} 
                 && !Bugzilla->user->in_group(Bugzilla->params->{"insidergroup"})) 
             {
                 $extra = "AND $table.isprivate < 1";
             }
             push(@supptables, "LEFT JOIN longdescs AS $table " .
                               "ON $table.bug_id = bugs.bug_id $extra");
             $f = "$table.thetext";
         },
         "^longdescs\.isprivate," => sub {
             my $table = "longdescs_$chartid";
             my $extra = "";
             if (Bugzilla->params->{"insidergroup"}
                 && !Bugzilla->user->in_group(Bugzilla->params->{"insidergroup"}))
             {
                 $extra = "AND $table.isprivate < 1";
             }
             push(@supptables, "LEFT JOIN longdescs AS $table " .
                               "ON $table.bug_id = bugs.bug_id $extra");
             $f = "$table.isprivate";
         },
         "^work_time,changedby" => sub {
             my $table = "longdescs_$chartid";
             push(@supptables, "LEFT JOIN longdescs AS $table " .
                               "ON $table.bug_id = bugs.bug_id");
             my $id = login_to_id($v, THROW_ERROR);
             $term = "(($table.who = $id";
             $term .= ") AND ($table.work_time <> 0))";
         },
         "^work_time,changedbefore" => sub {
             my $table = "longdescs_$chartid";
             push(@supptables, "LEFT JOIN longdescs AS $table " .
                               "ON $table.bug_id = bugs.bug_id");
             $term = "(($table.bug_when < " . $dbh->quote(SqlifyDate($v));
             $term .= ") AND ($table.work_time <> 0))";
         },
         "^work_time,changedafter" => sub {
             my $table = "longdescs_$chartid";
             push(@supptables, "LEFT JOIN longdescs AS $table " .
                               "ON $table.bug_id = bugs.bug_id");
             $term = "(($table.bug_when > " . $dbh->quote(SqlifyDate($v));
             $term .= ") AND ($table.work_time <> 0))";
         },
         "^work_time," => sub {
             my $table = "longdescs_$chartid";
             push(@supptables, "LEFT JOIN longdescs AS $table " .
                               "ON $table.bug_id = bugs.bug_id");
             $f = "$table.work_time";
         },
         "^percentage_complete," => sub {
             my $oper;
             if ($t eq "equals") {
                 $oper = "=";
             } elsif ($t eq "greaterthan") {
                 $oper = ">";
             } elsif ($t eq "lessthan") {
                 $oper = "<";
             } elsif ($t eq "notequal") {
                 $oper = "<>";
             } elsif ($t eq "regexp") {
                 # This is just a dummy to help catch bugs- $oper won't be used
                 # since "regexp" is treated as a special case below.  But
                 # leaving $oper uninitialized seems risky...
                 $oper = "sql_regexp";
             } elsif ($t eq "notregexp") {
                 # This is just a dummy to help catch bugs- $oper won't be used
                 # since "notregexp" is treated as a special case below.  But
                 # leaving $oper uninitialized seems risky...
                 $oper = "sql_not_regexp";
             } else {
                 $oper = "noop";
             }
             if ($oper ne "noop") {
                 my $table = "longdescs_$chartid";
                 if(lsearch(\@fields, "bugs.remaining_time") == -1) {
                     push(@fields, "bugs.remaining_time");                  
                 }
                 push(@supptables, "LEFT JOIN longdescs AS $table " .
                                   "ON $table.bug_id = bugs.bug_id");
                 my $expression = "(100 * ((SUM($table.work_time) *
                                             COUNT(DISTINCT $table.bug_when) /
                                             COUNT(bugs.bug_id)) /
                                            ((SUM($table.work_time) *
                                              COUNT(DISTINCT $table.bug_when) /
                                              COUNT(bugs.bug_id)) +
                                             bugs.remaining_time)))";
                 $q = $dbh->quote($v);
                 trick_taint($q);
                 if ($t eq "regexp") {
                     push(@having, $dbh->sql_regexp($expression, $q));
                 } elsif ($t eq "notregexp") {
                     push(@having, $dbh->sql_not_regexp($expression, $q));
                 } else {
                     push(@having, "$expression $oper " . $q);
                 }
                 push(@groupby, "bugs.remaining_time");
             }
             $term = "0=0";
         },
         "^bug_group,(?!changed)" => sub {
            push(@supptables,
                    "LEFT JOIN bug_group_map AS bug_group_map_$chartid " .
                    "ON bugs.bug_id = bug_group_map_$chartid.bug_id");
            $ff = $f = "groups_$chartid.name";
            my $ref = $funcsbykey{",$t"};
            &$ref;
            push(@supptables,
                    "LEFT JOIN groups AS groups_$chartid " .
                    "ON groups_$chartid.id = bug_group_map_$chartid.group_id " .
                    "AND $term");
            $term = "$ff IS NOT NULL";
         },
         "^attach_data\.thedata,changed" => sub {
            # Searches for attachment data's change must search
            # the creation timestamp of the attachment instead.
            $f = "attachments.whocares";
         },
         "^attach_data\.thedata," => sub {
             my $atable = "attachments_$chartid";
             my $dtable = "attachdata_$chartid";
             my $extra = "";
             if (Bugzilla->params->{"insidergroup"} 
                 && !Bugzilla->user->in_group(Bugzilla->params->{"insidergroup"})) 
             {
                 $extra = "AND $atable.isprivate = 0";
             }
             push(@supptables, "INNER JOIN attachments AS $atable " .
                               "ON bugs.bug_id = $atable.bug_id $extra");
             push(@supptables, "INNER JOIN attach_data AS $dtable " .
                               "ON $dtable.id = $atable.attach_id");
             $f = "$dtable.thedata";
         },
         "^attachments\.submitter," => sub {
             my $atable = "map_attachment_submitter_$chartid";
             my $extra = "";
             if (Bugzilla->params->{"insidergroup"}
                 && !Bugzilla->user->in_group(Bugzilla->params->{"insidergroup"}))
             {
                 $extra = "AND $atable.isprivate = 0";
             }
             push(@supptables, "INNER JOIN attachments AS $atable " .
                               "ON bugs.bug_id = $atable.bug_id $extra");
             push(@supptables, "LEFT JOIN profiles AS attachers_$chartid " .
                               "ON $atable.submitter_id = attachers_$chartid.userid");
             $f = "attachers_$chartid.login_name";
         },
         "^attachments\..*," => sub {
             my $table = "attachments_$chartid";
             my $extra = "";
             if (Bugzilla->params->{"insidergroup"} 
                 && !Bugzilla->user->in_group(Bugzilla->params->{"insidergroup"})) 
             {
                 $extra = "AND $table.isprivate = 0";
             }
             push(@supptables, "INNER JOIN attachments AS $table " .
                               "ON bugs.bug_id = $table.bug_id $extra");
             $f =~ m/^attachments\.(.*)$/;
             my $field = $1;
             if ($t eq "changedby") {
                 $v = login_to_id($v, THROW_ERROR);
                 $q = $dbh->quote($v);
                 $field = "submitter_id";
                 $t = "equals";
             } elsif ($t eq "changedbefore") {
                 $v = SqlifyDate($v);
                 $q = $dbh->quote($v);
                 $field = "creation_ts";
                 $t = "lessthan";
             } elsif ($t eq "changedafter") {
                 $v = SqlifyDate($v);
                 $q = $dbh->quote($v);
                 $field = "creation_ts";
                 $t = "greaterthan";
             }
             if ($field eq "ispatch" && $v ne "0" && $v ne "1") {
                 ThrowUserError("illegal_attachment_is_patch");
             }
             if ($field eq "isobsolete" && $v ne "0" && $v ne "1") {
                 ThrowUserError("illegal_is_obsolete");
             }
             $f = "$table.$field";
         },
         "^flagtypes.name," => sub {
             # Matches bugs by flag name/status.
             # Note that--for the purposes of querying--a flag comprises
             # its name plus its status (i.e. a flag named "review" 
             # with a status of "+" can be found by searching for "review+").
             
             # Don't do anything if this condition is about changes to flags,
             # as the generic change condition processors can handle those.
             return if ($t =~ m/^changed/);
             
             # Add the flags and flagtypes tables to the query.  We do 
             # a left join here so bugs without any flags still match 
             # negative conditions (f.e. "flag isn't review+").
             my $flags = "flags_$chartid";
             push(@supptables, "LEFT JOIN flags AS $flags " . 
                               "ON bugs.bug_id = $flags.bug_id ");
             my $flagtypes = "flagtypes_$chartid";
             push(@supptables, "LEFT JOIN flagtypes AS $flagtypes " . 
                               "ON $flags.type_id = $flagtypes.id");
             
             # Generate the condition by running the operator-specific
             # function. Afterwards the condition resides in the global $term
             # variable.
             $ff = $dbh->sql_string_concat("${flagtypes}.name",
                                           "$flags.status");
             &{$funcsbykey{",$t"}};
             
             # If this is a negative condition (f.e. flag isn't "review+"),
             # we only want bugs where all flags match the condition, not 
             # those where any flag matches, which needs special magic.
             # Instead of adding the condition to the WHERE clause, we select
             # the number of flags matching the condition and the total number
             # of flags on each bug, then compare them in a HAVING clause.
             # If the numbers are the same, all flags match the condition,
             # so this bug should be included.
             if ($t =~ m/not/) {
                push(@having,
                     "SUM(CASE WHEN $ff IS NOT NULL THEN 1 ELSE 0 END) = " .
                     "SUM(CASE WHEN $term THEN 1 ELSE 0 END)");
                $term = "0=0";
             }
         },
         "^requestees.login_name," => sub {
             my $flags = "flags_$chartid";
             push(@supptables, "LEFT JOIN flags AS $flags " .
                               "ON bugs.bug_id = $flags.bug_id ");
             push(@supptables, "LEFT JOIN profiles AS requestees_$chartid " .
                               "ON $flags.requestee_id = requestees_$chartid.userid");
             $f = "requestees_$chartid.login_name";
         },
         "^setters.login_name," => sub {
             my $flags = "flags_$chartid";
             push(@supptables, "LEFT JOIN flags AS $flags " .
                               "ON bugs.bug_id = $flags.bug_id ");
             push(@supptables, "LEFT JOIN profiles AS setters_$chartid " .
                               "ON $flags.setter_id = setters_$chartid.userid");
             $f = "setters_$chartid.login_name";
         },
         
         "^(changedin|days_elapsed)," => sub {
             $f = "(" . $dbh->sql_to_days('NOW()') . " - " .
                        $dbh->sql_to_days('bugs.delta_ts') . ")";
         },

         "^component,(?!changed)" => sub {
             $f = $ff = "components.name";
             $funcsbykey{",$t"}->();
             $term = build_subselect("bugs.component_id",
                                     "components.id",
                                     "components",
                                     $term);
         },

         "^product,(?!changed)" => sub {
             # Generate the restriction condition
             $f = $ff = "products.name";
             $funcsbykey{",$t"}->();
             $term = build_subselect("bugs.product_id",
                                     "products.id",
                                     "products",
                                     $term);
         },

         "^classification,(?!changed)" => sub {
             # Generate the restriction condition
             push @supptables, "INNER JOIN products AS map_products " .
                               "ON bugs.product_id = map_products.id";
             $f = $ff = "classifications.name";
             $funcsbykey{",$t"}->();
             $term = build_subselect("map_products.classification_id",
                                     "classifications.id",
                                     "classifications",
                                     $term);
         },

         "^keywords,(?!changed)" => sub {
             my @list;
             my $table = "keywords_$chartid";
             foreach my $value (split(/[\s,]+/, $v)) {
                 if ($value eq '') {
                     next;
                 }
                 my $keyword = new Bugzilla::Keyword({name => $value});
                 if ($keyword) {
                     push(@list, "$table.keywordid = " . $keyword->id);
                 }
                 else {
                     ThrowUserError("unknown_keyword",
                                    { keyword => $v });
                 }
             }
             my $haveawordterm;
             if (@list) {
                 $haveawordterm = "(" . join(' OR ', @list) . ")";
                 if ($t eq "anywords") {
                     $term = $haveawordterm;
                 } elsif ($t eq "allwords") {
                     my $ref = $funcsbykey{",$t"};
                     &$ref;
                     if ($term && $haveawordterm) {
                         $term = "(($term) AND $haveawordterm)";
                     }
                 }
             }
             if ($term) {
                 push(@supptables, "LEFT JOIN keywords AS $table " .
                                   "ON $table.bug_id = bugs.bug_id");
             }
         },

         "^dependson,(?!changed)" => sub {
                my $table = "dependson_" . $chartid;
                $ff = "$table.$f";
                my $ref = $funcsbykey{",$t"};
                &$ref;
                push(@supptables, "LEFT JOIN dependencies AS $table " .
                                  "ON $table.blocked = bugs.bug_id " .
                                  "AND ($term)");
                $term = "$ff IS NOT NULL";
         },

         "^blocked,(?!changed)" => sub {
                my $table = "blocked_" . $chartid;
                $ff = "$table.$f";
                my $ref = $funcsbykey{",$t"};
                &$ref;
                push(@supptables, "LEFT JOIN dependencies AS $table " .
                                  "ON $table.dependson = bugs.bug_id " .
                                  "AND ($term)");
                $term = "$ff IS NOT NULL";
         },

         "^alias,(?!changed)" => sub {
             $ff = "COALESCE(bugs.alias, '')";
             my $ref = $funcsbykey{",$t"};
             &$ref;
         },

         "^owner_idle_time,(greaterthan|lessthan)" => sub {
                my $table = "idle_" . $chartid;
                $v =~ /^(\d+)\s*([hHdDwWmMyY])?$/;
                my $quantity = $1;
                my $unit = lc $2;
                my $unitinterval = 'DAY';
                if ($unit eq 'h') {
                    $unitinterval = 'HOUR';
                } elsif ($unit eq 'w') {
                    $unitinterval = ' * 7 DAY';
                } elsif ($unit eq 'm') {
                    $unitinterval = 'MONTH';
                } elsif ($unit eq 'y') {
                    $unitinterval = 'YEAR';
                }
                my $cutoff = "NOW() - " .
                             $dbh->sql_interval($quantity, $unitinterval);
                my $assigned_fieldid = get_field_id('assigned_to');
                push(@supptables, "LEFT JOIN longdescs AS comment_$table " .
                                  "ON comment_$table.who = bugs.assigned_to " .
                                  "AND comment_$table.bug_id = bugs.bug_id " .
                                  "AND comment_$table.bug_when > $cutoff");
                push(@supptables, "LEFT JOIN bugs_activity AS activity_$table " .
                                  "ON (activity_$table.who = bugs.assigned_to " .
                                  "OR activity_$table.fieldid = $assigned_fieldid) " .
                                  "AND activity_$table.bug_id = bugs.bug_id " .
                                  "AND activity_$table.bug_when > $cutoff");
                if ($t =~ /greater/) {
                    push(@wherepart, "(comment_$table.who IS NULL " .
                                     "AND activity_$table.who IS NULL)");
                } else {
                    push(@wherepart, "(comment_$table.who IS NOT NULL " .
                                     "OR activity_$table.who IS NOT NULL)");
                }
                $term = "0=0";
         },

         ",equals" => sub {
             $term = "$ff = $q";
         },
         ",notequals" => sub {
             $term = "$ff != $q";
         },
         ",casesubstring" => sub {
             $term = $dbh->sql_position($q, $ff) . " > 0";
         },
         ",substring" => sub {
             $term = $dbh->sql_position(lc($q), "LOWER($ff)") . " > 0";
         },
         ",substr" => sub {
             $funcsbykey{",substring"}->();
         },
         ",notsubstring" => sub {
             $term = $dbh->sql_position(lc($q), "LOWER($ff)") . " = 0";
         },
         ",regexp" => sub {
             $term = $dbh->sql_regexp($ff, $q);
         },
         ",notregexp" => sub {
             $term = $dbh->sql_not_regexp($ff, $q);
         },
         ",lessthan" => sub {
             $term = "$ff < $q";
         },
         ",matches" => sub {
             ThrowUserError("search_content_without_matches");
         },
         ",greaterthan" => sub {
             $term = "$ff > $q";
         },
         ",anyexact" => sub {
             my @list;
             foreach my $w (split(/,/, $v)) {
                 if ($w eq "---" && $f =~ /resolution/) {
                     $w = "";
                 }
                 $q = $dbh->quote($w);
                 trick_taint($q);
                 push(@list, $q);
             }
             if (@list) {
                 $term = "$ff IN (" . join (',', @list) . ")";
             }
         },
         ",anywordssubstr" => sub {
             $term = join(" OR ", @{GetByWordListSubstr($ff, $v)});
         },
         ",allwordssubstr" => sub {
             $term = join(" AND ", @{GetByWordListSubstr($ff, $v)});
         },
         ",nowordssubstr" => sub {
             my @list = @{GetByWordListSubstr($ff, $v)};
             if (@list) {
                 $term = "NOT (" . join(" OR ", @list) . ")";
             }
         },
         ",anywords" => sub {
             $term = join(" OR ", @{GetByWordList($ff, $v)});
         },
         ",allwords" => sub {
             $term = join(" AND ", @{GetByWordList($ff, $v)});
         },
         ",nowords" => sub {
             my @list = @{GetByWordList($ff, $v)};
             if (@list) {
                 $term = "NOT (" . join(" OR ", @list) . ")";
             }
         },
         ",(changedbefore|changedafter)" => sub {
             my $operator = ($t =~ /before/) ? '<' : '>';
             my $table = "act_$chartid";
             my $fieldid = $chartfields{$f};
             if (!$fieldid) {
                 ThrowCodeError("invalid_field_name", {field => $f});
             }
             push(@supptables, "LEFT JOIN bugs_activity AS $table " .
                               "ON $table.bug_id = bugs.bug_id " .
                               "AND $table.fieldid = $fieldid " .
                               "AND $table.bug_when $operator " . 
                               $dbh->quote(SqlifyDate($v)) );
             $term = "($table.bug_when IS NOT NULL)";
         },
         ",(changedfrom|changedto)" => sub {
             my $operator = ($t =~ /from/) ? 'removed' : 'added';
             my $table = "act_$chartid";
             my $fieldid = $chartfields{$f};
             if (!$fieldid) {
                 ThrowCodeError("invalid_field_name", {field => $f});
             }
             push(@supptables, "LEFT JOIN bugs_activity AS $table " .
                               "ON $table.bug_id = bugs.bug_id " .
                               "AND $table.fieldid = $fieldid " .
                               "AND $table.$operator = $q");
             $term = "($table.bug_when IS NOT NULL)";
         },
         ",changedby" => sub {
             my $table = "act_$chartid";
             my $fieldid = $chartfields{$f};
             if (!$fieldid) {
                 ThrowCodeError("invalid_field_name", {field => $f});
             }
             my $id = login_to_id($v, THROW_ERROR);
             push(@supptables, "LEFT JOIN bugs_activity AS $table " .
                               "ON $table.bug_id = bugs.bug_id " .
                               "AND $table.fieldid = $fieldid " .
                               "AND $table.who = $id");
             $term = "($table.bug_when IS NOT NULL)";
         },
         );
    my @funcnames;
    while (@funcdefs) {
        my $key = shift(@funcdefs);
        my $value = shift(@funcdefs);
        if ($key =~ /^[^,]*$/) {
            die "All defs in %funcs must have a comma in their name: $key";
        }
        if (exists $funcsbykey{$key}) {
            die "Duplicate key in %funcs: $key";
        }
        $funcsbykey{$key} = $value;
        push(@funcnames, $key);
    }

    # first we delete any sign of "Chart #-1" from the HTML form hash
    # since we want to guarantee the user didn't hide something here
    my @badcharts = grep /^(field|type|value)-1-/, $params->param();
    foreach my $field (@badcharts) {
        $params->delete($field);
    }

    # now we take our special chart and stuff it into the form hash
    my $chart = -1;
    my $row = 0;
    foreach my $ref (@specialchart) {
        my $col = 0;
        while (@$ref) {
            $params->param("field$chart-$row-$col", shift(@$ref));
            $params->param("type$chart-$row-$col", shift(@$ref));
            $params->param("value$chart-$row-$col", shift(@$ref));
            if ($debug) {
                push(@debugdata, "$row-$col = " .
                               $params->param("field$chart-$row-$col") . ' | ' .
                               $params->param("type$chart-$row-$col") . ' | ' .
                               $params->param("value$chart-$row-$col") . ' *');
            }
            $col++;

        }
        $row++;
    }


# A boolean chart is a way of representing the terms in a logical
# expression.  Bugzilla builds SQL queries depending on how you enter
# terms into the boolean chart. Boolean charts are represented in
# urls as tree-tuples of (chart id, row, column). The query form
# (query.cgi) may contain an arbitrary number of boolean charts where
# each chart represents a clause in a SQL query.
#
# The query form starts out with one boolean chart containing one
# row and one column.  Extra rows can be created by pressing the
# AND button at the bottom of the chart.  Extra columns are created
# by pressing the OR button at the right end of the chart. Extra
# charts are created by pressing "Add another boolean chart".
#
# Each chart consists of an arbitrary number of rows and columns.
# The terms within a row are ORed together. The expressions represented
# by each row are ANDed together. The expressions represented by each
# chart are ANDed together.
#
#        ----------------------
#        | col2 | col2 | col3 |
# --------------|------|------|
# | row1 |  a1  |  a2  |      |
# |------|------|------|------|  => ((a1 OR a2) AND (b1 OR b2 OR b3) AND (c1))
# | row2 |  b1  |  b2  |  b3  |
# |------|------|------|------|
# | row3 |  c1  |      |      |
# -----------------------------
#
#        --------
#        | col2 |
# --------------|
# | row1 |  d1  | => (d1)
# ---------------
#
# Together, these two charts represent a SQL expression like this
# SELECT blah FROM blah WHERE ( (a1 OR a2)AND(b1 OR b2 OR b3)AND(c1)) AND (d1)
#
# The terms within a single row of a boolean chart are all constraints
# on a single piece of data.  If you're looking for a bug that has two
# different people cc'd on it, then you need to use two boolean charts.
# This will find bugs with one CC matching 'foo@blah.org' and and another
# CC matching 'bar@blah.org'.
#
# --------------------------------------------------------------
# CC    | equal to
# foo@blah.org
# --------------------------------------------------------------
# CC    | equal to
# bar@blah.org
#
# If you try to do this query by pressing the AND button in the
# original boolean chart then what you'll get is an expression that
# looks for a single CC where the login name is both "foo@blah.org",
# and "bar@blah.org". This is impossible.
#
# --------------------------------------------------------------
# CC    | equal to
# foo@blah.org
# AND
# CC    | equal to
# bar@blah.org
# --------------------------------------------------------------

# $chartid is the number of the current chart whose SQL we're constructing
# $row is the current row of the current chart

# names for table aliases are constructed using $chartid and $row
#   SELECT blah  FROM $table "$table_$chartid_$row" WHERE ....

# $f  = field of table in bug db (e.g. bug_id, reporter, etc)
# $ff = qualified field name (field name prefixed by table)
#       e.g. bugs_activity.bug_id
# $t  = type of query. e.g. "equal to", "changed after", case sensitive substr"
# $v  = value - value the user typed in to the form
# $q  = sanitized version of user input trick_taint(($dbh->quote($v)))
# @supptables = Tables and/or table aliases used in query
# %suppseen   = A hash used to store all the tables in supptables to weed
#               out duplicates.
# @supplist   = A list used to accumulate all the JOIN clauses for each
#               chart to merge the ON sections of each.
# $suppstring = String which is pasted into query containing all table names

    # get a list of field names to verify the user-submitted chart fields against
    %chartfields = @{$dbh->selectcol_arrayref(
        q{SELECT name, id FROM fielddefs}, { Columns=>[1,2] })};

    $row = 0;
    for ($chart=-1 ;
         $chart < 0 || $params->param("field$chart-0-0") ;
         $chart++) {
        $chartid = $chart >= 0 ? $chart : "";
        my @chartandlist = ();
        for ($row = 0 ;
             $params->param("field$chart-$row-0") ;
             $row++) {
            my @orlist;
            for (my $col = 0 ;
                 $params->param("field$chart-$row-$col") ;
                 $col++) {
                $f = $params->param("field$chart-$row-$col") || "noop";
                $t = $params->param("type$chart-$row-$col") || "noop";
                $v = $params->param("value$chart-$row-$col");
                $v = "" if !defined $v;
                $v = trim($v);
                if ($f eq "noop" || $t eq "noop" || $v eq "") {
                    next;
                }
                # chart -1 is generated by other code above, not from the user-
                # submitted form, so we'll blindly accept any values in chart -1
                if ((!$chartfields{$f}) && ($chart != -1)) {
                    ThrowCodeError("invalid_field_name", {field => $f});
                }

                # This is either from the internal chart (in which case we
                # already know about it), or it was in %chartfields, so it is
                # a valid field name, which means that it's ok.
                trick_taint($f);
                $q = $dbh->quote($v);
                trick_taint($q);
                my $rhs = $v;
                $rhs =~ tr/,//;
                my $func;
                $term = undef;
                foreach my $key (@funcnames) {
                    if ("$f,$t,$rhs" =~ m/$key/) {
                        my $ref = $funcsbykey{$key};
                        if ($debug) {
                            push(@debugdata, "$key ($f / $t / $rhs) =>");
                        }
                        $ff = $f;
                        if ($f !~ /\./) {
                            $ff = "bugs.$f";
                        }
                        &$ref;
                        if ($debug) {
                            push(@debugdata, "$f / $t / $v / " .
                                             ($term || "undef") . " *");
                        }
                        if ($term) {
                            last;
                        }
                    }
                }
                if ($term) {
                    push(@orlist, $term);
                }
                else {
                    # This field and this type don't work together.
                    ThrowCodeError("field_type_mismatch",
                                   { field => $params->param("field$chart-$row-$col"),
                                     type => $params->param("type$chart-$row-$col"),
                                   });
                }
            }
            if (@orlist) {
                @orlist = map("($_)", @orlist) if (scalar(@orlist) > 1);
                push(@chartandlist, "(" . join(" OR ", @orlist) . ")");
            }
        }
        if (@chartandlist) {
            if ($params->param("negate$chart")) {
                push(@andlist, "NOT(" . join(" AND ", @chartandlist) . ")");
            } else {
                push(@andlist, "(" . join(" AND ", @chartandlist) . ")");
            }
        }
    }

    # The ORDER BY clause goes last, but can require modifications
    # to other parts of the query, so we want to create it before we
    # write the FROM clause.
    foreach my $orderitem (@inputorder) {
        # Some fields have 'AS' aliases. The aliases go in the ORDER BY,
        # not the whole fields.
        # XXX - Ideally, we would get just the aliases in @inputorder,
        # and we'd never have to deal with this.
        if ($orderitem =~ /\s+AS\s+(.+)$/i) {
            $orderitem = $1;
        }
        BuildOrderBy(\%special_order, $orderitem, \@orderby);
    }
    # Now JOIN the correct tables in the FROM clause.
    # This is done separately from the above because it's
    # cleaner to do it this way.
    foreach my $orderitem (@inputorder) {
        # Grab the part without ASC or DESC.
        my @splitfield = split(/\s+/, $orderitem);
        if ($special_order_join{$splitfield[0]}) {
            push(@supptables, $special_order_join{$splitfield[0]});
        }
    }

    my %suppseen = ("bugs" => 1);
    my $suppstring = "bugs";
    my @supplist = (" ");
    foreach my $str (@supptables) {

        if ($str =~ /^(LEFT|INNER|RIGHT)\s+JOIN/i) {
            $str =~ /^(.*?)\s+ON\s+(.*)$/i;
            my ($leftside, $rightside) = ($1, $2);
            if (defined $suppseen{$leftside}) {
                $supplist[$suppseen{$leftside}] .= " AND ($rightside)";
            } else {
                $suppseen{$leftside} = scalar @supplist;
                push @supplist, " $leftside ON ($rightside)";
            }
        } else {
            # Do not accept implicit joins using comma operator
            # as they are not DB agnostic
            ThrowCodeError("comma_operator_deprecated");
        }
    }
    $suppstring .= join('', @supplist);
    
    # Make sure we create a legal SQL query.
    @andlist = ("1 = 1") if !@andlist;

    my $query = "SELECT " . join(', ', @fields) .
                " FROM $suppstring" .
                " LEFT JOIN bug_group_map " .
                " ON bug_group_map.bug_id = bugs.bug_id ";

    if ($user->id) {
        if (%{$user->groups}) {
            $query .= " AND bug_group_map.group_id NOT IN (" . join(',', values(%{$user->groups})) . ") ";
        }

        $query .= " LEFT JOIN cc ON cc.bug_id = bugs.bug_id AND cc.who = " . $user->id;
    }

    $query .= " WHERE " . join(' AND ', (@wherepart, @andlist)) .
              " AND bugs.creation_ts IS NOT NULL AND ((bug_group_map.group_id IS NULL)";

    if ($user->id) {
        my $userid = $user->id;
        $query .= "    OR (bugs.reporter_accessible = 1 AND bugs.reporter = $userid) " .
              "    OR (bugs.cclist_accessible = 1 AND cc.who IS NOT NULL) " .
              "    OR (bugs.assigned_to = $userid) ";
        if (Bugzilla->params->{'useqacontact'}) {
            $query .= "OR (bugs.qa_contact = $userid) ";
        }
    }

    foreach my $field (@fields, @orderby) {
        next if ($field =~ /(AVG|SUM|COUNT|MAX|MIN|VARIANCE)\s*\(/i ||
                 $field =~ /^\d+$/ || $field eq "bugs.bug_id" ||
                 $field =~ /^(relevance|actual_time|percentage_complete)/);
        # The structure of fields is of the form:
        # [foo AS] {bar | bar.baz} [ASC | DESC]
        # Only the mandatory part bar OR bar.baz is of interest
        if ($field =~ /(?:.*\s+AS\s+)?(\w+(\.\w+)?)(?:\s+(ASC|DESC))?$/i) {
            push(@groupby, $1) if !grep($_ eq $1, @groupby);
        }
    }
    $query .= ") " . $dbh->sql_group_by("bugs.bug_id", join(', ', @groupby));


    if (@having) {
        $query .= " HAVING " . join(" AND ", @having);
    }

    if (@orderby) {
        $query .= " ORDER BY " . join(',', @orderby);
    }

    $self->{'sql'} = $query;
    $self->{'debugdata'} = \@debugdata;
}

###############################################################################
# Helper functions for the init() method.
###############################################################################
sub SqlifyDate {
    my ($str) = @_;
    $str = "" if !defined $str;
    if ($str eq "") {
        my ($sec, $min, $hour, $mday, $month, $year, $wday) = localtime(time());
        return sprintf("%4d-%02d-%02d 00:00:00", $year+1900, $month+1, $mday);
    }


    if ($str =~ /^(-|\+)?(\d+)([hHdDwWmMyY])$/) {   # relative date
        my ($sign, $amount, $unit, $date) = ($1, $2, lc $3, time);
        my ($sec, $min, $hour, $mday, $month, $year, $wday)  = localtime($date);
        if ($sign && $sign eq '+') { $amount = -$amount; }
        if ($unit eq 'w') {                  # convert weeks to days
            $amount = 7*$amount + $wday;
            $unit = 'd';
        }
        if ($unit eq 'd') {
            $date -= $sec + 60*$min + 3600*$hour + 24*3600*$amount;
            return time2str("%Y-%m-%d %H:%M:%S", $date);
        }
        elsif ($unit eq 'y') {
            return sprintf("%4d-01-01 00:00:00", $year+1900-$amount);
        }
        elsif ($unit eq 'm') {
            $month -= $amount;
            while ($month<0) { $year--; $month += 12; }
            return sprintf("%4d-%02d-01 00:00:00", $year+1900, $month+1);
        }
        elsif ($unit eq 'h') {
            # Special case 0h for 'beginning of this hour'
            if ($amount == 0) {
                $date -= $sec + 60*$min;
            } else {
                $date -= 3600*$amount;
            }
            return time2str("%Y-%m-%d %H:%M:%S", $date);
        }
        return undef;                      # should not happen due to regexp at top
    }
    my $date = str2time($str);
    if (!defined($date)) {
        ThrowUserError("illegal_date", { date => $str });
    }
    return time2str("%Y-%m-%d %H:%M:%S", $date);
}

# ListIDsForEmail returns a string with a comma-joined list
# of userids matching email addresses
# according to the type specified.
# Currently, this only supports regexp, exact, anyexact, and substring matches.
# Matches will return up to 50 matching userids
# If a match type is unsupported or returns too many matches,
# ListIDsForEmail returns an undef.
sub ListIDsForEmail {
    my ($self, $type, $email) = (@_);
    my $old = $self->{"emailcache"}{"$type,$email"};
    return undef if ($old && $old eq "---");
    return $old if $old;
    
    my $dbh = Bugzilla->dbh;
    my @list = ();
    my $list = "---";
    if ($type eq 'anyexact') {
        foreach my $w (split(/,/, $email)) {
            $w = trim($w);
            my $id = login_to_id($w);
            if ($id > 0) {
                push(@list,$id)
            }
        }
        $list = join(',', @list);
    } elsif ($type eq 'substring') {
        my $sql_email = $dbh->quote($email);
        trick_taint($sql_email);
        my $result = $dbh->selectcol_arrayref(
                    q{SELECT userid FROM profiles WHERE } .
                    $dbh->sql_position(lc($sql_email), q{LOWER(login_name)}) .
                    q{ > 0 } . $dbh->sql_limit(51));
        @list = @{$result};
        if (scalar(@list) < 50) {
            $list = join(',', @list);
        }
    } elsif ($type eq 'regexp') {
        my $sql_email = $dbh->quote($email);
        trick_taint($sql_email);
        my $result = $dbh->selectcol_arrayref(
                        qq{SELECT userid FROM profiles WHERE } .
                        $dbh->sql_regexp("login_name", $sql_email) .
                        q{ } . $dbh->sql_limit(51));
        @list = @{$result};
        if (scalar(@list) < 50) {
            $list = join(',', @list);
        }
    }
    $self->{"emailcache"}{"$type,$email"} = $list;
    return undef if ($list eq "---");
    return $list;
}

sub build_subselect {
    my ($outer, $inner, $table, $cond) = @_;
    my $q = "SELECT $inner FROM $table WHERE $cond";
    #return "$outer IN ($q)";
    my $dbh = Bugzilla->dbh;
    my $list = $dbh->selectcol_arrayref($q);
    return "1=2" unless @$list; # Could use boolean type on dbs which support it
    return "$outer IN (" . join(',', @$list) . ")";
}

sub GetByWordList {
    my ($field, $strs) = (@_);
    my @list;
    my $dbh = Bugzilla->dbh;

    foreach my $w (split(/[\s,]+/, $strs)) {
        my $word = $w;
        if ($word ne "") {
            $word =~ tr/A-Z/a-z/;
            $word = $dbh->quote(quotemeta($word));
            trick_taint($word);
            $word =~ s/^'//;
            $word =~ s/'$//;
            $word = '(^|[^a-z0-9])' . $word . '($|[^a-z0-9])';
            push(@list, $dbh->sql_regexp($field, "'$word'"));
        }
    }

    return \@list;
}

# Support for "any/all/nowordssubstr" comparison type ("words as substrings")
sub GetByWordListSubstr {
    my ($field, $strs) = (@_);
    my @list;
    my $dbh = Bugzilla->dbh;
    my $sql_word;

    foreach my $word (split(/[\s,]+/, $strs)) {
        if ($word ne "") {
            $sql_word = $dbh->quote($word);
            trick_taint($sql_word);
            push(@list, $dbh->sql_position(lc($sql_word),
                                           "LOWER($field)") . " > 0");
        }
    }

    return \@list;
}

sub getSQL {
    my $self = shift;
    return $self->{'sql'};
}

sub getDebugData {
    my $self = shift;
    return $self->{'debugdata'};
}

sub pronoun {
    my ($noun, $user) = (@_);
    if ($noun eq "%user%") {
        if ($user->id) {
            return $user->id;
        } else {
            ThrowUserError('login_required_for_pronoun');
        }
    }
    if ($noun eq "%reporter%") {
        return "bugs.reporter";
    }
    if ($noun eq "%assignee%") {
        return "bugs.assigned_to";
    }
    if ($noun eq "%qacontact%") {
        return "bugs.qa_contact";
    }
    return 0;
}

# Validate that the query type is one we can deal with
sub IsValidQueryType
{
    my ($queryType) = @_;
    if (grep { $_ eq $queryType } qw(specific advanced)) {
        return 1;
    }
    return 0;
}

# BuildOrderBy - Private Subroutine
# This function converts the input order to an "output" order,
# suitable for concatenation to form an ORDER BY clause. Basically,
# it just handles fields that have non-standard sort orders from
# %specialorder.
# Arguments:
#  $orderitem - A string. The next value to append to the ORDER BY clause,
#      in the format of an item in the 'order' parameter to
#      Bugzilla::Search.
#  $stringlist - A reference to the list of strings that will be join()'ed
#      to make ORDER BY. This is what the subroutine modifies.
#  $reverseorder - (Optional) A boolean. TRUE if we should reverse the order
#      of the field that we are given (from ASC to DESC or vice-versa).
#
# Explanation of $reverseorder
# ----------------------------
# The role of $reverseorder is to handle things like sorting by
# "target_milestone DESC".
# Let's say that we had a field "A" that normally translates to a sort 
# order of "B ASC, C DESC". If we sort by "A DESC", what we really then
# mean is "B DESC, C ASC". So $reverseorder is only used if we call 
# BuildOrderBy recursively, to let it know that we're "reversing" the 
# order. That is, that we wanted "A DESC", not "A".
sub BuildOrderBy {
    my ($special_order, $orderitem, $stringlist, $reverseorder) = (@_);

    my @twopart = split(/\s+/, $orderitem);
    my $orderfield = $twopart[0];
    my $orderdirection = $twopart[1] || "";

    if ($reverseorder) {
        # If orderdirection is empty or ASC...
        if (!$orderdirection || $orderdirection =~ m/asc/i) {
            $orderdirection = "DESC";
        } else {
            # This has the minor side-effect of making any reversed invalid
            # direction into ASC.
            $orderdirection = "ASC";
        }
    }

    # Handle fields that have non-standard sort orders, from $specialorder.
    if ($special_order->{$orderfield}) {
        foreach my $subitem (@{$special_order->{$orderfield}}) {
            # DESC on a field with non-standard sort order means
            # "reverse the normal order for each field that we map to."
            BuildOrderBy($special_order, $subitem, $stringlist,
                         $orderdirection =~ m/desc/i);
        }
        return;
    }

    push(@$stringlist, trim($orderfield . ' ' . $orderdirection));
}

# This is a helper for fulltext search
sub _split_words_into_like {
    my ($field, $text) = @_;
    my $dbh = Bugzilla->dbh;
    # This code is very similar to Bugzilla::DB::sql_fulltext_search,
    # so you can look there if you'd like an explanation of what it's
    # doing.
    my $lower_text = lc($text);
    my @words = split(/\s+/, $lower_text);
    @words = map($dbh->quote("%$_%"), @words);
    map(trick_taint($_), @words);
    @words = map($dbh->sql_istrcmp($field, $_, 'LIKE'), @words);
    return @words;
}
1;
