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
# Contributor(s): Terry Weissman <terry@mozilla.org>
#                 Dan Mosedale <dmose@mozilla.org>
#                 Jacob Steenhagen <jake@bugzilla.org>
#                 Bradley Baetz <bbaetz@student.usyd.edu.au>
#                 Christopher Aillon <christopher@aillon.com>
#                 Tobias Burnus <burnus@net-b.de>
#                 Myk Melez <myk@mozilla.org>
#                 Max Kanat-Alexander <mkanat@bugzilla.org>
#                 Frédéric Buclin <LpSolit@gmail.com>
#                 Greg Hendricks <ghendricks@novell.com>
#                 David D. Kilzer <ddkilzer@kilzer.net>


package Bugzilla::Template;

use strict;

use Bugzilla::Constants;
use Bugzilla::Install::Requirements;
use Bugzilla::Util;
use Bugzilla::User;
use Bugzilla::Error;
use MIME::Base64;
use Bugzilla::Bug;

use Cwd qw(abs_path);
# for time2str - replace by TT Date plugin??
use Date::Format ();
use File::Basename qw(dirname);
use File::Find;
use File::Path qw(rmtree mkpath);
use File::Spec;
use IO::Dir;

use base qw(Template);

# Convert the constants in the Bugzilla::Constants module into a hash we can
# pass to the template object for reflection into its "constants" namespace
# (which is like its "variables" namespace, but for constants).  To do so, we
# traverse the arrays of exported and exportable symbols, pulling out functions
# (which is how Perl implements constants) and ignoring the rest (which, if
# Constants.pm exports only constants, as it should, will be nothing else).
sub _load_constants {
    my %constants;
    foreach my $constant (@Bugzilla::Constants::EXPORT,
                          @Bugzilla::Constants::EXPORT_OK)
    {
        if (defined Bugzilla::Constants->$constant) {
            # Constants can be lists, and we can't know whether we're
            # getting a scalar or a list in advance, since they come to us
            # as the return value of a function call, so we have to
            # retrieve them all in list context into anonymous arrays,
            # then extract the scalar ones (i.e. the ones whose arrays
            # contain a single element) from their arrays.
            $constants{$constant} = [&{$Bugzilla::Constants::{$constant}}];
            if (scalar(@{$constants{$constant}}) == 1) {
                $constants{$constant} = @{$constants{$constant}}[0];
            }
        }
    }
    return \%constants;
}

# Make an ordered list out of a HTTP Accept-Language header see RFC 2616, 14.4
# We ignore '*' and <language-range>;q=0
# For languages with the same priority q the order remains unchanged.
sub sortAcceptLanguage {
    sub sortQvalue { $b->{'qvalue'} <=> $a->{'qvalue'} }
    my $accept_language = $_[0];

    # clean up string.
    $accept_language =~ s/[^A-Za-z;q=0-9\.\-,]//g;
    my @qlanguages;
    my @languages;
    foreach(split /,/, $accept_language) {
        if (m/([A-Za-z\-]+)(?:;q=(\d(?:\.\d+)))?/) {
            my $lang   = $1;
            my $qvalue = $2;
            $qvalue = 1 if not defined $qvalue;
            next if $qvalue == 0;
            $qvalue = 1 if $qvalue > 1;
            push(@qlanguages, {'qvalue' => $qvalue, 'language' => $lang});
        }
    }

    return map($_->{'language'}, (sort sortQvalue @qlanguages));
}

# Returns the path to the templates based on the Accept-Language
# settings of the user and of the available languages
# If no Accept-Language is present it uses the defined default
# Templates may also be found in the extensions/ tree
sub getTemplateIncludePath {
    my $lang = Bugzilla->request_cache->{'language'} || "";
    # Return cached value if available

    my $include_path = Bugzilla->request_cache->{"template_include_path_$lang"};
    return $include_path if $include_path;

    my $templatedir = bz_locations()->{'templatedir'};
    my $project     = bz_locations()->{'project'};

    my $languages = trim(Bugzilla->params->{'languages'});
    if (not ($languages =~ /,/)) {
       if ($project) {
           $include_path = [
               "$templatedir/$languages/$project",
               "$templatedir/$languages/custom",
               "$templatedir/$languages/default"
           ];
       } else {
           $include_path = [
               "$templatedir/$languages/custom",
               "$templatedir/$languages/default"
           ];
       }
    }
    my @languages       = sortAcceptLanguage($languages);
    # If $lang is specified, only consider this language.
    my @accept_language = ($lang) || sortAcceptLanguage($ENV{'HTTP_ACCEPT_LANGUAGE'} || "");
    my @usedlanguages;
    foreach my $language (@accept_language) {
        # Per RFC 1766 and RFC 2616 any language tag matches also its 
        # primary tag. That is 'en' (accept language)  matches 'en-us',
        # 'en-uk' etc. but not the otherway round. (This is unfortunately
        # not very clearly stated in those RFC; see comment just over 14.5
        # in http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.4)
        if(my @found = grep /^\Q$language\E(-.+)?$/i, @languages) {
            push (@usedlanguages, @found);
        }
    }
    push(@usedlanguages, Bugzilla->params->{'defaultlanguage'});
    if ($project) {
        $include_path = [
           map((
               "$templatedir/$_/$project",
               "$templatedir/$_/custom",
               "$templatedir/$_/default"
               ), @usedlanguages
            )
        ];
    } else {
        $include_path = [
           map((
               "$templatedir/$_/custom",
               "$templatedir/$_/default"
               ), @usedlanguages
            )
        ];
    }
    
    # add in extension template directories:
    my @extensions = glob(bz_locations()->{'extensionsdir'} . "/*");
    foreach my $extension (@extensions) {
        trick_taint($extension); # since this comes right from the filesystem
                                 # we have bigger issues if it is insecure
        push(@$include_path,
            map((
                $extension."/template/".$_),
               @usedlanguages));
    }
    
    # remove duplicates since they keep popping up:
    my @dirs;
    foreach my $dir (@$include_path) {
        push(@dirs, $dir) unless grep ($dir eq $_, @dirs);
    }
    Bugzilla->request_cache->{"template_include_path_$lang"} = \@dirs;
    
    return Bugzilla->request_cache->{"template_include_path_$lang"};
}

sub put_header {
    my $self = shift;
    my $vars = {};
    ($vars->{'title'}, $vars->{'h1'}, $vars->{'h2'}) = (@_);
     
    $self->process("global/header.html.tmpl", $vars)
      || ThrowTemplateError($self->error());
    $vars->{'header_done'} = 1;
}

sub put_footer {
    my $self = shift;
    $self->process("global/footer.html.tmpl")
      || ThrowTemplateError($self->error());
}

sub get_format {
    my $self = shift;
    my ($template, $format, $ctype) = @_;

    $ctype ||= 'html';
    $format ||= '';

    # Security - allow letters and a hyphen only
    $ctype =~ s/[^a-zA-Z\-]//g;
    $format =~ s/[^a-zA-Z\-]//g;
    trick_taint($ctype);
    trick_taint($format);

    $template .= ($format ? "-$format" : "");
    $template .= ".$ctype.tmpl";

    # Now check that the template actually exists. We only want to check
    # if the template exists; any other errors (eg parse errors) will
    # end up being detected later.
    eval {
        $self->context->template($template);
    };
    # This parsing may seem fragile, but it's OK:
    # http://lists.template-toolkit.org/pipermail/templates/2003-March/004370.html
    # Even if it is wrong, any sort of error is going to cause a failure
    # eventually, so the only issue would be an incorrect error message
    if ($@ && $@->info =~ /: not found$/) {
        ThrowUserError('format_not_found', {'format' => $format,
                                            'ctype'  => $ctype});
    }

    # Else, just return the info
    return
    {
        'template'    => $template,
        'extension'   => $ctype,
        'ctype'       => Bugzilla::Constants::contenttypes->{$ctype}
    };
}

# This routine quoteUrls contains inspirations from the HTML::FromText CPAN
# module by Gareth Rees <garethr@cre.canon.co.uk>.  It has been heavily hacked,
# all that is really recognizable from the original is bits of the regular
# expressions.
# This has been rewritten to be faster, mainly by substituting 'as we go'.
# If you want to modify this routine, read the comments carefully

sub quoteUrls {
    my ($text, $curr_bugid) = (@_);
    return $text unless $text;

    # We use /g for speed, but uris can have other things inside them
    # (http://foo/bug#3 for example). Filtering that out filters valid
    # bug refs out, so we have to do replacements.
    # mailto can't contain space or #, so we don't have to bother for that
    # Do this by escaping \0 to \1\0, and replacing matches with \0\0$count\0\0
    # \0 is used because it's unlikely to occur in the text, so the cost of
    # doing this should be very small
    # Also, \0 won't appear in the value_quote'd bug title, so we don't have
    # to worry about bogus substitutions from there

    # escape the 2nd escape char we're using
    my $chr1 = chr(1);
    $text =~ s/\0/$chr1\0/g;

    # However, note that adding the title (for buglinks) can affect things
    # In particular, attachment matches go before bug titles, so that titles
    # with 'attachment 1' don't double match.
    # Dupe checks go afterwards, because that uses ^ and \Z, which won't occur
    # if it was substituted as a bug title (since that always involve leading
    # and trailing text)

    # Because of entities, it's easier (and quicker) to do this before escaping

    my @things;
    my $count = 0;
    my $tmp;

    # Provide tooltips for full bug links (Bug 74355)
    my $urlbase_re = '(' . join('|',
        map { qr/$_/ } grep($_, Bugzilla->params->{'urlbase'}, 
                            Bugzilla->params->{'sslbase'})) . ')';
    $text =~ s~\b(${urlbase_re}\Qshow_bug.cgi?id=\E([0-9]+)(\#c([0-9]+))?)\b
              ~($things[$count++] = get_bug_link($3, $1, $5)) &&
               ("\0\0" . ($count-1) . "\0\0")
              ~egox;

    # non-mailto protocols
    my $safe_protocols = join('|', SAFE_PROTOCOLS);
    my $protocol_re = qr/($safe_protocols)/i;

    $text =~ s~\b(${protocol_re}:  # The protocol:
                  [^\s<>\"]+       # Any non-whitespace
                  [\w\/])          # so that we end in \w or /
              ~($tmp = html_quote($1)) &&
               ($things[$count++] = "<a href=\"$tmp\">$tmp</a>") &&
               ("\0\0" . ($count-1) . "\0\0")
              ~egox;

    # We have to quote now, otherwise the html itself is escaped
    # THIS MEANS THAT A LITERAL ", <, >, ' MUST BE ESCAPED FOR A MATCH

    $text = html_quote($text);

    # Color quoted text
    $text =~ s~^(&gt;.+)$~<span class="quote">$1</span >~mg;
    $text =~ s~</span >\n<span class="quote">~\n~g;

    # mailto:
    # Use |<nothing> so that $1 is defined regardless
    $text =~ s~\b(mailto:|)?([\w\.\-\+\=]+\@[\w\-]+(?:\.[\w\-]+)+)\b
              ~<a href=\"mailto:$2\">$1$2</a>~igx;

    # attachment links - handle both cases separately for simplicity
    $text =~ s~((?:^Created\ an\ |\b)attachment\s*\(id=(\d+)\)(\s\[edit\])?)
              ~($things[$count++] = get_attachment_link($2, $1)) &&
               ("\0\0" . ($count-1) . "\0\0")
              ~egmx;

    $text =~ s~\b(attachment\s*\#?\s*(\d+))
              ~($things[$count++] = get_attachment_link($2, $1)) &&
               ("\0\0" . ($count-1) . "\0\0")
              ~egmxi;

    # Current bug ID this comment belongs to
    my $current_bugurl = $curr_bugid ? "show_bug.cgi?id=$curr_bugid" : "";

    # This handles bug a, comment b type stuff. Because we're using /g
    # we have to do this in one pattern, and so this is semi-messy.
    # Also, we can't use $bug_re?$comment_re? because that will match the
    # empty string
    my $bug_word = get_text('term', { term => 'bug' });
    my $bug_re = qr/\Q$bug_word\E\s*\#?\s*(\d+)/i;
    my $comment_re = qr/comment\s*\#?\s*(\d+)/i;
    $text =~ s~\b($bug_re(?:\s*,?\s*$comment_re)?|$comment_re)
              ~ # We have several choices. $1 here is the link, and $2-4 are set
                # depending on which part matched
               (defined($2) ? get_bug_link($2,$1,$3) :
                              "<a href=\"$current_bugurl#c$4\">$1</a>")
              ~egox;

    # Old duplicate markers. These don't use $bug_word because they are old
    # and were never customizable.
    $text =~ s~(?<=^\*\*\*\ This\ bug\ has\ been\ marked\ as\ a\ duplicate\ of\ )
               (\d+)
               (?=\ \*\*\*\Z)
              ~get_bug_link($1, $1)
              ~egmx;

    # Now remove the encoding hacks
    $text =~ s/\0\0(\d+)\0\0/$things[$1]/eg;
    $text =~ s/$chr1\0/\0/g;

    return $text;
}

# Creates a link to an attachment, including its title.
sub get_attachment_link {
    my ($attachid, $link_text) = @_;
    my $dbh = Bugzilla->dbh;

    detaint_natural($attachid)
      || die "get_attachment_link() called with non-integer attachment number";

    my ($bugid, $isobsolete, $desc) =
        $dbh->selectrow_array('SELECT bug_id, isobsolete, description
                               FROM attachments WHERE attach_id = ?',
                               undef, $attachid);

    if ($bugid) {
        my $title = "";
        my $className = "";
        if (Bugzilla->user->can_see_bug($bugid)) {
            $title = $desc;
        }
        if ($isobsolete) {
            $className = "bz_obsolete";
        }
        # Prevent code injection in the title.
        $title = value_quote($title);

        $link_text =~ s/ \[details\]$//;
        my $linkval = "attachment.cgi?id=$attachid";
        # Whitespace matters here because these links are in <pre> tags.
        return qq|<span class="$className">|
               . qq|<a href="${linkval}" name="attach_${attachid}" title="$title">$link_text</a>|
               . qq| <a href="${linkval}&amp;action=edit" title="$title">[details]</a>|
               . qq|</span>|;
    }
    else {
        return qq{$link_text};
    }
}

# Creates a link to a bug, including its title.
# It takes either two or three parameters:
#  - The bug number
#  - The link text, to place between the <a>..</a>
#  - An optional comment number, for linking to a particular
#    comment in the bug

sub get_bug_link {
    my ($bug_num, $link_text, $comment_num) = @_;
    my $dbh = Bugzilla->dbh;

    if (!defined($bug_num) || ($bug_num eq "")) {
        return "&lt;missing bug number&gt;";
    }
    my $quote_bug_num = html_quote($bug_num);
    detaint_natural($bug_num) || return "&lt;invalid bug number: $quote_bug_num&gt;";

    my ($bug_state, $bug_res, $bug_desc) =
        $dbh->selectrow_array('SELECT bugs.bug_status, resolution, short_desc
                               FROM bugs WHERE bugs.bug_id = ?',
                               undef, $bug_num);

    if ($bug_state) {
        # Initialize these variables to be "" so that we don't get warnings
        # if we don't change them below (which is highly likely).
        my ($pre, $title, $post) = ("", "", "");

        $title = $bug_state;
        if ($bug_state eq 'UNCONFIRMED') {
            $pre = "<i>";
            $post = "</i>";
        }
        elsif (!is_open_state($bug_state)) {
            $pre = '<span class="bz_closed">';
            $title .= " $bug_res";
            $post = '</span>';
        }
        if (Bugzilla->user->can_see_bug($bug_num)) {
            $title .= " - $bug_desc";
        }
        # Prevent code injection in the title.
        $title = value_quote($title);

        my $linkval = "show_bug.cgi?id=$bug_num";
        if (defined $comment_num) {
            $linkval .= "#c$comment_num";
        }
        return qq{$pre<a href="$linkval" title="$title">$link_text</a>$post};
    }
    else {
        return qq{$link_text};
    }
}

###############################################################################
# Templatization Code

# The Template Toolkit throws an error if a loop iterates >1000 times.
# We want to raise that limit.
# NOTE: If you change this number, you MUST RE-RUN checksetup.pl!!!
# If you do not re-run checksetup.pl, the change you make will not apply
$Template::Directive::WHILE_MAX = 1000000;

# Use the Toolkit Template's Stash module to add utility pseudo-methods
# to template variables.
use Template::Stash;

# Add "contains***" methods to list variables that search for one or more 
# items in a list and return boolean values representing whether or not 
# one/all/any item(s) were found.
$Template::Stash::LIST_OPS->{ contains } =
  sub {
      my ($list, $item) = @_;
      return grep($_ eq $item, @$list);
  };

$Template::Stash::LIST_OPS->{ containsany } =
  sub {
      my ($list, $items) = @_;
      foreach my $item (@$items) { 
          return 1 if grep($_ eq $item, @$list);
      }
      return 0;
  };

# Allow us to still get the scalar if we use the list operation ".0" on it,
# as we often do for defaults in query.cgi and other places.
$Template::Stash::SCALAR_OPS->{ 0 } = 
  sub {
      return $_[0];
  };

# Add a "substr" method to the Template Toolkit's "scalar" object
# that returns a substring of a string.
$Template::Stash::SCALAR_OPS->{ substr } = 
  sub {
      my ($scalar, $offset, $length) = @_;
      return substr($scalar, $offset, $length);
  };

# Add a "truncate" method to the Template Toolkit's "scalar" object
# that truncates a string to a certain length.
$Template::Stash::SCALAR_OPS->{ truncate } = 
  sub {
      my ($string, $length, $ellipsis) = @_;
      $ellipsis ||= "";
      
      return $string if !$length || length($string) <= $length;
      
      my $strlen = $length - length($ellipsis);
      my $newstr = substr($string, 0, $strlen) . $ellipsis;
      return $newstr;
  };

# Create the template object that processes templates and specify
# configuration parameters that apply to all templates.

###############################################################################

# Construct the Template object

# Note that all of the failure cases here can't use templateable errors,
# since we won't have a template to use...

sub create {
    my $class = shift;
    my %opts = @_;

    # checksetup.pl will call us once for any template/lang directory.
    # We need a possibility to reset the cache, so that no files from
    # the previous language pollute the action.
    if ($opts{'clean_cache'}) {
        delete Bugzilla->request_cache->{template_include_path_};
    }

    # IMPORTANT - If you make any configuration changes here, make sure to
    # make them in t/004.template.t and checksetup.pl.

    return $class->new({
        # Colon-separated list of directories containing templates.
        INCLUDE_PATH => [\&getTemplateIncludePath],

        # Remove white-space before template directives (PRE_CHOMP) and at the
        # beginning and end of templates and template blocks (TRIM) for better
        # looking, more compact content.  Use the plus sign at the beginning
        # of directives to maintain white space (i.e. [%+ DIRECTIVE %]).
        PRE_CHOMP => 1,
        TRIM => 1,

        COMPILE_DIR => bz_locations()->{'datadir'} . "/template",

        # Initialize templates (f.e. by loading plugins like Hook).
        PRE_PROCESS => "global/initialize.none.tmpl",

        # Functions for processing text within templates in various ways.
        # IMPORTANT!  When adding a filter here that does not override a
        # built-in filter, please also add a stub filter to t/004template.t.
        FILTERS => {

            # Render text in required style.

            inactive => [
                sub {
                    my($context, $isinactive) = @_;
                    return sub {
                        return $isinactive ? '<span class="bz_inactive">'.$_[0].'</span>' : $_[0];
                    }
                }, 1
            ],

            closed => [
                sub {
                    my($context, $isclosed) = @_;
                    return sub {
                        return $isclosed ? '<span class="bz_closed">'.$_[0].'</span>' : $_[0];
                    }
                }, 1
            ],

            obsolete => [
                sub {
                    my($context, $isobsolete) = @_;
                    return sub {
                        return $isobsolete ? '<span class="bz_obsolete">'.$_[0].'</span>' : $_[0];
                    }
                }, 1
            ],

            # Returns the text with backslashes, single/double quotes,
            # and newlines/carriage returns escaped for use in JS strings.
            js => sub {
                my ($var) = @_;
                $var =~ s/([\\\'\"\/])/\\$1/g;
                $var =~ s/\n/\\n/g;
                $var =~ s/\r/\\r/g;
                $var =~ s/\@/\\x40/g; # anti-spam for email addresses
                return $var;
            },
            
            # Converts data to base64
            base64 => sub {
                my ($data) = @_;
                return encode_base64($data);
            },
            
            # HTML collapses newlines in element attributes to a single space,
            # so form elements which may have whitespace (ie comments) need
            # to be encoded using &#013;
            # See bugs 4928, 22983 and 32000 for more details
            html_linebreak => sub {
                my ($var) = @_;
                $var =~ s/\r\n/\&#013;/g;
                $var =~ s/\n\r/\&#013;/g;
                $var =~ s/\r/\&#013;/g;
                $var =~ s/\n/\&#013;/g;
                return $var;
            },

            # Prevents line break on hyphens and whitespaces.
            no_break => sub {
                my ($var) = @_;
                $var =~ s/ /\&nbsp;/g;
                $var =~ s/-/\&#8209;/g;
                return $var;
            },

            xml => \&Bugzilla::Util::xml_quote ,

            # This filter escapes characters in a variable or value string for
            # use in a query string.  It escapes all characters NOT in the
            # regex set: [a-zA-Z0-9_\-.].  The 'uri' filter should be used for
            # a full URL that may have characters that need encoding.
            url_quote => \&Bugzilla::Util::url_quote ,

            # This filter is similar to url_quote but used a \ instead of a %
            # as prefix. In addition it replaces a ' ' by a '_'.
            css_class_quote => \&Bugzilla::Util::css_class_quote ,

            quoteUrls => [ sub {
                               my ($context, $bug) = @_;
                               return sub {
                                   my $text = shift;
                                   return quoteUrls($text, $bug);
                               };
                           },
                           1
                         ],

            bug_link => [ sub {
                              my ($context, $bug) = @_;
                              return sub {
                                  my $text = shift;
                                  return get_bug_link($bug, $text);
                              };
                          },
                          1
                        ],

            bug_list_link => sub
            {
                my $buglist = shift;
                return join(", ", map(get_bug_link($_, $_), split(/ *, */, $buglist)));
            },

            # In CSV, quotes are doubled, and any value containing a quote or a
            # comma is enclosed in quotes.
            csv => sub
            {
                my ($var) = @_;
                $var =~ s/\"/\"\"/g;
                if ($var !~ /^-?(\d+\.)?\d*$/) {
                    $var = "\"$var\"";
                }
                return $var;
            } ,

            # Format a filesize in bytes to a human readable value
            unitconvert => sub
            {
                my ($data) = @_;
                my $retval = "";
                my %units = (
                    'KB' => 1024,
                    'MB' => 1024 * 1024,
                    'GB' => 1024 * 1024 * 1024,
                );

                if ($data < 1024) {
                    return "$data bytes";
                } 
                else {
                    my $u;
                    foreach $u ('GB', 'MB', 'KB') {
                        if ($data >= $units{$u}) {
                            return sprintf("%.2f %s", $data/$units{$u}, $u);
                        }
                    }
                }
            },

            # Format a time for display (more info in Bugzilla::Util)
            time => \&Bugzilla::Util::format_time,

            # Bug 120030: Override html filter to obscure the '@' in user
            #             visible strings.
            # Bug 319331: Handle BiDi disruptions.
            html => sub {
                my ($var) = Template::Filters::html_filter(@_);
                # Obscure '@'.
                $var =~ s/\@/\&#64;/g;
                if (Bugzilla->params->{'utf8'}) {
                    # Remove the following characters because they're
                    # influencing BiDi:
                    # --------------------------------------------------------
                    # |Code  |Name                      |UTF-8 representation|
                    # |------|--------------------------|--------------------|
                    # |U+202a|Left-To-Right Embedding   |0xe2 0x80 0xaa      |
                    # |U+202b|Right-To-Left Embedding   |0xe2 0x80 0xab      |
                    # |U+202c|Pop Directional Formatting|0xe2 0x80 0xac      |
                    # |U+202d|Left-To-Right Override    |0xe2 0x80 0xad      |
                    # |U+202e|Right-To-Left Override    |0xe2 0x80 0xae      |
                    # --------------------------------------------------------
                    #
                    # The following are characters influencing BiDi, too, but
                    # they can be spared from filtering because they don't
                    # influence more than one character right or left:
                    # --------------------------------------------------------
                    # |Code  |Name                      |UTF-8 representation|
                    # |------|--------------------------|--------------------|
                    # |U+200e|Left-To-Right Mark        |0xe2 0x80 0x8e      |
                    # |U+200f|Right-To-Left Mark        |0xe2 0x80 0x8f      |
                    # --------------------------------------------------------
                    #
                    # Do the replacing in a loop so that we don't get tricked
                    # by stuff like 0xe2 0xe2 0x80 0xae 0x80 0xae.
                    while ($var =~ s/\xe2\x80(\xaa|\xab|\xac|\xad|\xae)//g) {
                    }
                }
                return $var;
            },

            html_light => \&Bugzilla::Util::html_light_quote,

            # iCalendar contentline filter
            ics => [ sub {
                         my ($context, @args) = @_;
                         return sub {
                             my ($var) = shift;
                             my ($par) = shift @args;
                             my ($output) = "";

                             $var =~ s/[\r\n]/ /g;
                             $var =~ s/([;\\\",])/\\$1/g;

                             if ($par) {
                                 $output = sprintf("%s:%s", $par, $var);
                             } else {
                                 $output = $var;
                             }
                             
                             $output =~ s/(.{75,75})/$1\n /g;

                             return $output;
                         };
                     },
                     1
                     ],

            # Note that using this filter is even more dangerous than
            # using "none," and you should only use it when you're SURE
            # the output won't be displayed directly to a web browser.
            txt => sub {
                my ($var) = @_;
                # Trivial HTML tag remover
                $var =~ s/<[^>]*>//g;
                # And this basically reverses the html filter.
                $var =~ s/\&#64;/@/g;
                $var =~ s/\&lt;/</g;
                $var =~ s/\&gt;/>/g;
                $var =~ s/\&quot;/\"/g;
                $var =~ s/\&amp;/\&/g;
                return $var;
            },

            # Wrap a displayed comment to the appropriate length
            wrap_comment => \&Bugzilla::Util::wrap_comment,

            # We force filtering of every variable in key security-critical
            # places; we have a none filter for people to use when they 
            # really, really don't want a variable to be changed.
            none => sub { return $_[0]; } ,
        },

        PLUGIN_BASE => 'Bugzilla::Template::Plugin',

        CONSTANTS => _load_constants(),

        # Default variables for all templates
        VARIABLES => {
            # Function for retrieving global parameters.
            'Param' => sub { return Bugzilla->params->{$_[0]}; },

            # Function to create date strings
            'time2str' => \&Date::Format::time2str,

            # Generic linear search function
            'lsearch' => \&Bugzilla::Util::lsearch,

            # Currently logged in user, if any
            # If an sudo session is in progress, this is the user we're faking
            'user' => sub { return Bugzilla->user; },

            # If an sudo session is in progress, this is the user who
            # started the session.
            'sudoer' => sub { return Bugzilla->sudoer; },

            # SendBugMail - sends mail about a bug, using Bugzilla::BugMail.pm
            'SendBugMail' => sub {
                my ($id, $mailrecipients) = (@_);
                require Bugzilla::BugMail;
                Bugzilla::BugMail::Send($id, $mailrecipients);
            },

            # These don't work as normal constants.
            DB_MODULE        => \&Bugzilla::Constants::DB_MODULE,
            REQUIRED_MODULES => 
                \&Bugzilla::Install::Requirements::REQUIRED_MODULES,
            OPTIONAL_MODULES => sub {
                my @optional = @{OPTIONAL_MODULES()};
                @optional    = sort {$a->{feature} cmp $b->{feature}} 
                                    @optional;
                return \@optional;
            },
        },

   }) || die("Template creation failed: " . $class->error());
}

# Used as part of the two subroutines below.
our (%_templates_to_precompile, $_current_path);

sub precompile_templates {
    my ($output) = @_;

    # Remove the compiled templates.
    my $datadir = bz_locations()->{'datadir'};
    if (-e "$datadir/template") {
        print "Removing existing compiled templates ...\n" if $output;

        # XXX This frequently fails if the webserver made the files, because
        # then the webserver owns the directories. We could fix that by
        # doing a chmod/chown on all the directories here.
        rmtree("$datadir/template");

        # Check that the directory was really removed
        if(-e "$datadir/template") {
            print "\n\n";
            print "The directory '$datadir/template' could not be removed.\n";
            print "Please remove it manually and rerun checksetup.pl.\n\n";
            exit;
        }
    }

    print "Precompiling templates...\n" if $output;

    my $templatedir = bz_locations()->{'templatedir'};
    # Don't hang on templates which use the CGI library
    eval("use CGI qw(-no_debug)");
    
    my $dir_reader    = new IO::Dir($templatedir) || die "$templatedir: $!";
    my @language_dirs = grep { /^[a-z-]+$/i } $dir_reader->read;
    $dir_reader->close;

    foreach my $dir (@language_dirs) {
        next if ($dir eq 'CVS');
        -d "$templatedir/$dir/default" || -d "$templatedir/$dir/custom" 
            || next;
        local $ENV{'HTTP_ACCEPT_LANGUAGE'} = $dir;
        # We locally hack this parameter so that Bugzilla::Template
        # accepts this language no matter what.
        local Bugzilla->params->{'languages'} = "$dir,en";
        my $template = Bugzilla::Template->create(clean_cache => 1);

        # Precompile all the templates found in all the directories.
        %_templates_to_precompile = ();
        foreach my $subdir (qw(custom extension default), bz_locations()->{'project'}) {
            next unless $subdir; # If 'project' is empty.
            $_current_path = File::Spec->catdir($templatedir, $dir, $subdir);
            next unless -d $_current_path;
            # Traverse the template hierarchy.
            find({ wanted => \&_precompile_push, no_chdir => 1 }, $_current_path);
        }
        # The sort isn't totally necessary, but it makes debugging easier
        # by making the templates always be compiled in the same order.
        foreach my $file (sort keys %_templates_to_precompile) {
            # Compile the template but throw away the result. This has the side-
            # effect of writing the compiled version to disk.
            $template->context->template($file);
        }
    }

    # Under mod_perl, we look for templates using the absolute path of the
    # template directory, which causes Template Toolkit to look for their 
    # *compiled* versions using the full absolute path under the data/template
    # directory. (Like data/template/var/www/html/mod_perl/.) To avoid
    # re-compiling templates under mod_perl, we symlink to the
    # already-compiled templates. This doesn't work on Windows.
    if (!ON_WINDOWS) {
        my $abs_root = dirname(abs_path($templatedir));
        my $todir    = "$datadir/template$abs_root";
        mkpath($todir);
        # We use abs2rel so that the symlink will look like 
        # "../../../../template" which works, while just 
        # "data/template/template/" doesn't work.
        my $fromdir = File::Spec->abs2rel("$datadir/template/template", $todir);
        # We eval for systems that can't symlink at all, where "symlink" 
        # throws a fatal error.
        eval { symlink($fromdir, "$todir/template") 
                   or warn "Failed to symlink from $fromdir to $todir: $!" };
    }

    # If anything created a Template object before now, clear it out.
    delete Bugzilla->request_cache->{template};
    # This is the single variable used to precompile templates,
    # which needs to be cleared as well.
    delete Bugzilla->request_cache->{template_include_path_};
}

# Helper for precompile_templates
sub _precompile_push {
    my $name = $File::Find::name;
    return if (-d $name);
    return if ($name =~ /\/CVS\//);
    return if ($name !~ /\.tmpl$/);
   
    $name =~ s/\Q$_current_path\E\///;
    $_templates_to_precompile{$name} = 1;
}

1;

__END__

=head1 NAME

Bugzilla::Template - Wrapper around the Template Toolkit C<Template> object

=head1 SYNOPSIS

  my $template = Bugzilla::Template->create;

  $template->put_header($title, $h1, $h2);
  $template->put_footer();

  my $format = $template->get_format("foo/bar",
                                     scalar($cgi->param('format')),
                                     scalar($cgi->param('ctype')));

=head1 DESCRIPTION

This is basically a wrapper so that the correct arguments get passed into
the C<Template> constructor.

It should not be used directly by scripts or modules - instead, use
C<Bugzilla-E<gt>instance-E<gt>template> to get an already created module.

=head1 SUBROUTINES

=over

=item C<precompile_templates($output)>

Description: Compiles all of Bugzilla's templates in every language.
             Used mostly by F<checksetup.pl>.

Params:      C<$output> - C<true> if you want the function to print
               out information about what it's doing.

Returns:     nothing

=back

=head1 METHODS

=over

=item C<put_header($title, $h1, $h2)>

 Description: Display the header of the page for non yet templatized .cgi files.

 Params:      $title - Page title.
              $h1    - Main page header.
              $h2    - Page subheader.

 Returns:     nothing

=item C<put_footer()>

 Description: Display the footer of the page for non yet templatized .cgi files.

 Params:      none

 Returns:     nothing

=item C<get_format($file, $format, $ctype)>

 Description: Construct a format object from URL parameters.

 Params:      $file   - Name of the template to display.
              $format - When the template exists under several formats
                        (e.g. table or graph), specify the one to choose.
              $ctype  - Content type, see Bugzilla::Constants::contenttypes.

 Returns:     A format object.

=back

=head1 SEE ALSO

L<Bugzilla>, L<Template>
