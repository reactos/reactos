#!perl

=head1 NAME

Htmlcln - Sanitizes HTML and related files before publication

=head1 SYNOPSIS

perl htmlcln.pl [-t [html|js|css]] [-DVAR[=value] ...] [-v]
-o I<outfile> I<infile>

=head1 DESCRIPTION

The Htmlcln program preprocesses text files such as HTML, JS, or CSS
files and cleans them up.

=over 8
Comments are removed.

Blank lines are removed.

Sections marked "debug" are removed in the retail build.

=back

=head1 OPTIONS

=over 8

=item B<-t> [html|js|css]

Htmlcln normally tries to guess what kind of file it is processing from
the filename extension.  You can explicitly override the guess with the
B<-t> command line switch.

=item B<-D>VAR[=value] ...

Command line definitions are supported in the same manner as the C compiler.
The only command-line variable we pay attention to is the -DDBG flag, which
indicates that this is a debug build.

=item B<-o> I<outfile>

Specifies the name of the output file.

=item I<srcfile>

Specifies the name of the source file.

=back

=cut

use strict qw(vars refs subs);

##############################################################################
#
#   Element -  A class that parses HTML elements
#
#   Instance variables:
#
#       raw         = the raw text
#       attr        = hash of attributes
#       tag         = name of tag, including slash
#
#       If the value of an attribute hash is undef, it means that the
#       attribute is present but with no value.
#

package Element;

#
#   Constructor:  $elem = new Element("<TABLE BORDER>");
#
sub new {
    my ($class, $raw) = @_;
    my $attr = { };
    my $self = { raw => $raw, attr => $attr };
    my $tag;

    if ($raw =~ s/^<([^\s>]*)//) {
        $self->{tag} = uc $1;
        if ($self->{tag} =~ /^[A-Z]/) {
            $raw =~ s/>$//;
            for (;;) {
                if ($raw =~ s/^\s*([-A-Za-z]+)="([^"]*)"// ||
                    $raw =~ s/^\s*([-A-Za-z]+)='([^']*)'// ||
                    $raw =~ s/^\s*([-A-Za-z]+)=(\S*)//) {
                    $attr->{uc $1} = $2;
                } elsif ($raw =~ s/^\s*([A-Za-z]+)//) {
                    $attr->{uc $1} = undef;
                } else {
                    last;
                }
            }
        }
    } else {
        warn "Can't parse \"$raw\"";
    }

    bless $self, $class;

}

#
#   Element::Tag
#
#   Returns the tag.
#
sub Tag {
    my $self = shift;
    $self->{tag};
}

#
#   Element::Attr
#
#   Returns the value of the attribute.
#
sub Attr {
    my ($self, $attr) = @_;
    $self->{attr}{uc $attr};
}

#
#   Element::Exists
#
#   Returns the presence of the attribute.
#
sub Exists {
    my ($self, $attr) = @_;
    exists $self->{attr}{uc $attr};
}


##############################################################################
#
#   Filter base class
#
#   Basic stuff to save people some hassle.
#
#   Per perl tradition, an object is a ref to an anonymous hash where the
#   state is kept.
#
#   Instance variables:
#
#       sink        = reference to filter sink
#

package Filter;

sub new {
    my($class) = @_;
    bless { }, $class;
}

sub SetSink {
    my ($self, $sink) = @_;
    $self->{sink} = $sink;
}

sub Add {
    my $self = shift;
    $self->{sink}->Add(@_);
}

sub Flush { }

sub Close {
    my $self = shift;
    $self->{sink}->Close(@_);
}

sub SinkAdd {
    my $self = shift;
    $self->{sink}->Add(@_);
}

##############################################################################
#
#   TokenFilter filter package
#
#   Does not modify the stream, but merely chops them into tokens, as
#   recognized by NextToken and processed by EachToken.
#
#   Instance data:
#
#       buf         = unprocessed text
#
package TokenFilter;
@TokenFilter::ISA = qw(Filter);

#
#   Append the incoming text to the buffer, then suck out entire tokens.
#
sub Add {
    my($self, $text) = @_;
    my $tok;

    $self->{buf} .= $text;

    while ($self->{buf} ne '' && defined($tok = $self->NextToken))
    {
        $self->EachToken($tok);
    }
}

sub Flush {
    my $self = shift;
    $self->EachToken($self->{buf});
}

#
#   By default, we just sink tokens to the next layer.
#
sub EachToken {
    my($self, $tok) = @_;
    $self->SinkAdd($tok);
}


##############################################################################
#
#   LineFilter filter package
#
#   Tokenizer that recognizes lines.
#
#   Instance data:
#
#       buf         = unprocessed text
#
package LineFilter;
@LineFilter::ISA = qw(TokenFilter);

#
#   Recognize lines.
#
sub NextToken {
    my($self) = shift;

    if ($self->{buf} =~ s/([^\n]*\n)//) {
        $1;
    } else {
        undef;
    }
}

##############################################################################
#
#   WhitespaceFilter filter package
#
#   Removes blank lines and removes leading and trailing whitespace.
#
#   Someday: Collapse multiple whitespace outside of quotation marks.
#
package WhitespaceFilter;
@WhitespaceFilter::ISA = qw(LineFilter);

sub EachToken {
    my($self, $line) = @_;
    $line =~ s/^[ \t]+//;
    $line =~ s/[ \t]+$//;
    $self->SinkAdd($line) unless $line =~ /^$/;
}

##############################################################################
#
#   OutFile filter package
#
#   Writes its output to a file.
#
#   Instance data:
#
#       fh          = name of file handle
#
#

package OutFile;
@OutFile::ISA = qw(Filter);
no strict 'refs';           # Our filename globs aren't very strict

#
#   Custom method:  SetOutput.  Opens an output file.
#

my $seq = 0;

sub SetOutput {
    my($self, $file) = @_;
    $self->{fh} = "OutFile" . $seq++;
    open($self->{fh}, ">$file") || die "Unable to open $file for writing ($!)\n";
}

sub Add {
    my $self = shift;
    print { $self->{fh} }  @_;
}

sub Close {
    my $self = shift;
    close($self->{fh});
}

##############################################################################
#
#   DebugFilter filter package
#
#   Filters out ;debug and ;begin_debug blocks if building retail.
#
#   Instance data:
#
#       skip        = nonzero if we are inside an ignored ;begin_debug block
#       buf         = unprocessed text
#

package DebugFilter;
@DebugFilter::ISA = qw(LineFilter);
no strict 'refs';           # Our filename globs aren't very strict

#
#   See if the line contains a debug marker.
#   If applicable, send the line down the chain.
#
sub EachToken {
    my($self, $line) = @_;

    # ;begin_debug means start skipping if retail
    if ($line =~ s/;begin_debug//) {
        $self->{skip} = $::RetailVersion;
    }

    # If we were skipping, then ;end_debug ends skipping and we should eat it
    if ($line =~ s/;end_debug// && $self->{skip}) {
        $self->{skip} = 0;
    } elsif ($line =~ s/;debug// && $::RetailVersion) {
        # A one-shot debug line in retail - skip it
    } elsif (!$self->{skip}) {
        $self->SinkAdd($line);          # send it down the chain
    }
}

##############################################################################
#
#   CPP filter package
#
#   The CPP filter performs the following operations:
#
#       Removes C and C++-style comments.
#
#       Filters whitespace.
#
#   Instance data:
#
#       buf         = unprocessed text
#       wsf         = child WhitespaceFilter
#       script      = current script sink
#       ultSink     = the ultimate sink

package CPP;
@CPP::ISA = qw(TokenFilter);

sub new {
    my($class) = shift;
    my $self = new Filter;
    $self->{wsf} = new WhitespaceFilter;    # sink into a whitespace filter
    $self->{sink} = $self->{wsf};           # initially use this script
    bless $self, $class;
}

#
#   Recognize tokens, which are lines or /* ... */ comments.
#
sub NextToken {
    my($self) = shift;

    if ($self->{buf} =~ s/^([^\/]+)//) {    # eat up to a slash
        $1;
    } elsif ($self->{buf} =~ s/^\/\/.*?\n//) { # eat // to end of line
        "\n";
    } elsif ($self->{buf} =~ s/^\/\*[^\0]*?\*\///) { # eat /* .. */
        '';
    } elsif ($self->{buf} =~ s/^(\/)(?=[^\/\*])//) { # eat / not followed by / or *
        $1;
    } else {                                    # incomplete fragment - stop
        undef;
    }
}

#
#   SetSink
#
#   The sink we get is really the whitespace filter's sink, and we sink
#   into the whitespace filter.
#
sub SetSink {
    my ($self, $sink) = @_;
    $self->{wsf}->SetSink($sink);
}

##############################################################################
#
#
#   JS - comments are // or /* ... */, invoked via <SCRIPT>...
#   CSS - comments are /* ... */, invoked via <STYLE TYPE="text/css">
#
#   They are both just CPP thingies.  Both should someday remove whitespace

package JS;
@JS::ISA = qw(CPP);

package CSS;
@CSS::ISA = qw(CPP);

##############################################################################
#
#   HTML filter package
#
#   The HTML filter performs the following operations:
#
#       Send the final output through a whitespace filter.
#
#       Remove comments.
#
#   Someday it will also...
#
#       Recognize embedded stylesheets and scripts and generate a subfilter
#       to handle them.
#
#       Compress spaces outside quotation marks.
#
#   Instance data:
#
#       buf         = unprocessed text
#       wsf         = child WhitespaceFilter
#       script      = current script sink
#       endScript   = sub that recognizes end of script
#       ultSink     = the ultimate sink

package HTML;
@HTML::ISA = qw(TokenFilter);

sub new {
    my($class) = shift;
    my $self = new Filter;
    $self->{wsf} = new WhitespaceFilter;
    $self->{sink} = $self->{wsf};           # initially use this script
    bless $self, $class;
}

#
#   SetSink
#
#   The sink we get is really the whitespace filter's sink, and we sink
#   into the whitespace filter.
#
sub SetSink {
    my ($self, $sink) = @_;
    $self->{ultSink} = $sink;
    $self->{wsf}->SetSink($sink);
}

#
#   NextHTMLToken
#
#   An HTML token is one of the following:
#
#   -   A hunk of boring text.
#   -   A comment (thrown away).
#   -   A matched <...> thingie.

sub NextHTMLToken {
    my($self) = shift;

    #
    #   Any string of non "<" counts as a boring text token.
    #
    #   Be careful not to mistake <!DOCTYPE...> as a comment.
    #
    if ($self->{buf} =~ s/^([^<]+)//) {
        $1;
    } elsif ($self->{buf} =~ s/^(<!--[^\0]*?-->)//) {  # Eat full comments
        '';
    } elsif ($self->{buf} =~ s/^(<![^-][^>]*>)//) { # <!DOCTYPE ...>
        $1;
    } elsif ($self->{buf} =~ s/^(<[^!][^>]*>)//) { # <something else>
        $1;
    } else {                                    # incomplete fragment - stop
        undef;
    }
}

#
#   NextScriptToken
#
#   A script token is anything that isn't the word </SCRIPT>.
#

sub NextScriptToken
{
    my($self) = shift;
    if ($self->{buf} =~ s,^(</SCRIPT>),,i) {
        $1;
    } elsif ($self->{buf} =~ s,^(.*?)</SCRIPT>,,i) {
        $1;
    } else {
        my $tok = $self->{buf};
        $self->{buf} = '';
        $tok;
    }
}

#
#   NextToken
#
#   Returns either an HTML token or a script token.
#
sub NextToken {
    my($self) = shift;
    if (defined $self->{script}) {
        $self->NextScriptToken();
    } else {
        $self->NextHTMLToken();
    }
}

#
#   _Redirect - Private method that redirects parsing to a script language.
#
#       $self->_Redirect($scr, $end);
#
#       $scr = script object to hook in
#       $end = sub that recognizes the end of the script
#
#
sub _Redirect {
    my ($self, $scr, $end) = @_;
    $self->{script} = $self->{sink} = $scr;
    $scr->SetSink($self->{ultSink});
    $self->{endScript} = $end;
}

sub EachToken {
    my($self, $tok) = @_;

    if ($tok =~ /^<SCRIPT/i) {
        $self->{inScript} = 1; # BUGBUG create a script sink
        my $elem = new Element($tok);
        my $lang = lc $elem->Attr("LANGUAGE");
        my $scr;
        # No language implies JScript
        if (!defined($lang) || $lang eq 'jscript' || $lang eq 'javascript') {
            $scr = new CPP;
        } else {
            warn "Unknown script language [$lang]";
            # Just use the whitespace filter as the unknown script filter
            $scr = new WhitespaceFilter;
        }
        $self->_Redirect($scr, sub { m,^</SCRIPT>,i });

    } elsif ($tok =~ /<STYLE/i) {
        $self->_Redirect(new CSS, sub { m,^</STYLE>,i });

    } elsif (defined($self->{endScript}) && &{$self->{endScript}}($tok)) {
        delete $self->{endScript};
        $self->{script}->Flush();
        delete $self->{script};
        $self->{sink} = $self->{wsf};
    }
    $self->SinkAdd($tok);
}

##############################################################################
#
#   Main package
#

package main;

#
#   Set up some defaults.
#
my $force_type = undef;                 # do not force file type
$::RetailVersion = 1;                   # not the debugging version
my $outfile = undef;                    # output file not known yet
my %VAR = ();                           # No variables defined yet
my $verbose = undef;                    # not verbose mode

##############################################################################
#
#   CreateTypeFilter - Create a filter for the specified type.
#

my $types = {
    html    => sub { new HTML },        # HTML
    htm     => sub { new HTML },
    htx     => sub { new HTML },
    js      => sub { new JS },          # Javascript
    jsx     => sub { new JS },
    css     => sub { new CSS },         # Cascading style sheet
    csx     => sub { new CSS },
};

sub CreateTypeFilter {
    my $sub = $types->{lc shift};
    &$sub;
}

##############################################################################
#
#   Command line parsing
#

sub Usage {
    die "Usage: htmlcln [-t [html|js|css]] [-DVAR[=value]...] [-v] -o outfile infile\n";
}

#
#   AddDefine - Handle a -D command line option.
#
sub AddDefine {
    my $line = shift;
    if ($line =~ /=/) {
        $VAR{$`} = $';
    } else {
        $VAR{$line} = 1;
    }
}

sub ParseCommandLine {

    #
    #   Scream through the command line arguments.
    #

    while ($#ARGV >= 0 && $ARGV[0] =~ /^-(.)(.*)/) {
        # $1 - command
        # $2 - optional argument

        my($cmd, $val) = ($1, $2);

        shift(@ARGV);

        if ($cmd eq 't') {
            $val = shift(@ARGV) if $val eq '';
            $force_type = $val;
        } elsif ($cmd eq 'D') {
            AddDefine($val);
        } elsif ($cmd eq 'o') {
            $val = shift(@ARGV) if $val eq '';
            $outfile = $val;
        } elsif ($cmd eq 'v') {
            $verbose = 1;
        } else {
            Usage();
        }
    }

    #
    #   What's left should be a filename, and there should be an output file.
    #

    my $infile = shift(@ARGV);
    Usage() unless defined $infile && defined $outfile && $#ARGV == -1;

    #
    #   If the filetype is not being overridden, then take it from the filename.
    #
    if (!defined $force_type) {
        ($force_type) = $infile =~ /\.(.*)/;
    }

    #
    #   Include debug goo only if building DBG=1 and FULL_DEBUG is set in the
    #   environment.
    #
    $::RetailVersion = 0 if defined($VAR{"DBG"}) && defined($ENV{"FULL_DEBUG"});

    $infile;
}

##############################################################################
#
#   File processing
#

sub ProcessFile {
    my $infile = shift;

    #
    #   Create the final sink.
    #
    my $sink = new OutFile;
    $sink->SetOutput($outfile);

    #
    #   Set up the default filter based on the file type.
    #
    my $Type = CreateTypeFilter($force_type);
    $Type->SetSink($sink);

    #
    #   Create the DebugFilter which sits at the top of the chain.
    #
    my $Filter = new DebugFilter;
    $Filter->SetSink($Type);

    #
    #   All the plumbing is ready - start pumping data.
    #
    open(I, $infile) || die "Cannot open $infile for reading ($!)\n";

    while (<I>) {
        $Filter->Add($_);
    }
    $Filter->Flush();
    $Filter->Close();
}

##############################################################################
#
#   Main program
#

{
    my $infile = ParseCommandLine();
    ProcessFile($infile);
}
