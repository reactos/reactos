

sub check_dir
{
	my $dir = shift;
	opendir(DIR, $dir);
	my @files = readdir(DIR);
	closedir(DIR);
	
	my %resrevs = {};
	my @subdirs = ();
	
	foreach my $file (@files)
	{
		if ((-f "$dir/$file") && ($file =~ /^[A-Za-z][A-Za-z]\.rc$/))
		{
			# invoke Subversion to figure out info on this file
			open(SVNINVOKE, "svn info $dir/$file |");
			my @props = <SVNINVOKE>;
			close(SVNINVOKE);

			# pick out the appropriate property
			my ($propline) = grep { $_ =~ /Last Changed Rev: / } @props;
			if ($propline =~ /Last Changed Rev: (\d+)/)
			{
				my $rev = $1;
				$resrevs{$file} = $rev;
			}
		}
		elsif ((-d "$dir/$file") && !($file =~ /^\./))
		{
			# found a subdirectory
			push @subdirs, "$dir/$file";
		}
	}
	
	# list all files older than En.rc
	my $en_rev = $resrevs{'En.rc'};
	if ($en_rev > 0)
	{
		foreach my $file (keys(%resrevs))
		{
			my $this_rev = $resrevs{$file};
			if (($this_rev < $en_rev) && ($this_rev > 0))
			{
				my $path = "$dir/$file";
				$path =~ s/^\.\///;

				my $url = "http://svn.reactos.org/viewcvs/reactos/trunk/reactos/$path";
				
				print "<tr>\n";
				print "<td><a href='$url'>$path</a></td>\n";
				print "<td>$this_rev</td>\n"; 
				print "<td>$en_rev</td>\n"; 
				print "</tr>\n";
			}
		}
	}
	
	# recurse into subdirectories
	foreach my $dir (@subdirs)
	{
		&check_dir($dir);
	}
}

sub generate_content
{
	print '<h1><a href="http://www.reactos.org/xhtml/en/dev.html">';
	print "ReactOS Development</a> &gt; Resource translation status</h1>\n";
	print "<h2>Resource translation status</h2>\n";

	open(SVNINVOKE, "svn info . |");
	my @props = <SVNINVOKE>;
	close(SVNINVOKE);
	
	print "<h3>Last update: ";
	system "date";
	# pick out the appropriate property
	my ($propline) = grep { $_ =~ /Last Changed Rev: / } @props;
	if ($propline =~ /Last Changed Rev: (\d+)/)
	{
		print " (revision $1)";
	}
	print "</h3>\n";
	
	print "<table border='1'>\n";
	
	print "<tr>\n";
	print "<td>File</td>\n";
	print "<td>Rev</td>\n";
	print "<td>En.rc Rev</td>\n";
	print "</tr>\n";
	
	check_dir('.');
}

open(TEMPLATE, '../../htdocs/xhtml/en/dev_template.html' );
while (<TEMPLATE>)
{
	s/Template title/Resource translation status/;
#  s/^\/\/extramessage$/$extramessage/;
	if ($_ =~ '<!-- Replace this by the real content -->')
	{
		generate_content();
	}
	else
	{
		print "$_";
	}
}
close(TEMPLATE);
