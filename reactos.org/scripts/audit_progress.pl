#!/usr/bin/perl

# This script runs on a machine with access to a local ReactOS repositry
# Issues with the SVN perl bindings made this method easier.
# Also requires svn in your path.

use strict;
use warnings;

my $in_path  = "/reactos";              # enter the local path of the main reactos folder
my $out_file = "/progress_bar.html";    # enter the html output path
my $log_file = "/audit_progress.log";   # enter the log file path
my $locked_files = "/locked_files.log"; # enter the locked files log path
my $total_width = 200;
my $unclean_files = 0;
my (@locked_files, $total_files, $todo_width, $done_width, $done_pct);

open HTML, "> $out_file"
    or die "couldn't open $out_file for writing: $!\n";

my @files = `svn ls -R $in_path`;

foreach (@files) {
    next if /\/$/;
    $total_files++;
    if(`svn propget svn:needs-lock -R $_`) {
        $unclean_files++;
        push @locked_files, "$_";
    }
}

if ($unclean_files != 0) {
    $todo_width = ($unclean_files / $total_files) * $total_width;
    $done_width  = (($total_files - $unclean_files) / $total_files) * $total_width;
    $done_pct = (($total_files - $unclean_files) / $total_files) * 100;
} else {
    $todo_width = 0;
    $done_width = $total_width;
    $done_pct = 100;
}

output_html();

open LOCK, "> $locked_files"
    or die "couldn't open $locked_files for writing: $!\n";
print LOCK "Number of locked files: $unclean_files\n\n";
print LOCK @locked_files;

open LOG, ">> $log_file"
    or die "couldn't open $log_file for writing: $!\n";
my $date = localtime;
print LOG "$date  :  total files = $total_files\tfiles unaudited = $unclean_files\tcompleted $done_pct%\n";



sub output_html {
    print HTML  "<center>\n";
    print HTML  "  <img border=\"0\" src=\"http://www.reactos.org/images/progress-end.gif\" width=\"1\" height=\"20\">";
    printf HTML "<img border=\"0\" src=\"http://www.reactos.org/images/progress-done.gif\" width=\"%d\" height=\"20\">", $done_width;
    printf HTML "<img border=\"0\" src=\"http://www.reactos.org/images/progress-todo.gif\" width=\"%d\" height=\"20\">", $todo_width;
    print HTML  "<img border=\"0\" src=\"http://www.reactos.org/images/progress-end.gif\" width=\"1\" height=\"20\">\n";
    print HTML  "  <br>\n";
    printf HTML "  <font size=\"2\" face=\"Verdana, Arial, Helvetica, sans-serif\">%.1f%% complete</font>\n", $done_pct;
    print HTML  "</center>\n";
}

