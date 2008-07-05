#!/usr/bin/perl
# Since we use a linear search trought the block and the license and
# the warranty are quite big, we leave them at the end of the help file,
# the index will be consulted quite frequently, so we put it at the beginning. 

@help_file = <>;

foreach $line (@help_file){
    if ($line =~ /\x4\[(.*)\]/ && $line !~ /\x4\[main\]/){
	$nodes[$node_count++] = $1;
	$line =~ s/(\x4\[) */$1/;
    }
}

print "\x4[Contents]\nTopics:\n\n";
foreach $node (@nodes){
    if (length $node){
	$node =~ m/^( *)(.*)$/;
	printf ("  %s\x1 %s \x2%s\x3", $1, $2, $2);
    }
    print "\n";
}
#foreach $line (@help_file){
#    $line =~ s/%NEW_NODE%/\004/g;
#}
print @help_file;
