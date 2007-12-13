# Class to handle Elf images
# Placed under GNU Public License by Ken Yap, December 2000

package Elf;

use strict;
use IO::Seekable;

use constant;
use constant TFTPBLOCKSIZE => 512;
# ELF magic header in first 4 bytes
use constant MAGIC => "\x7FELF";
# This is defined by the bootrom layout
use constant HEADERSIZE => 512;
# Size of ELF header
use constant ELF_HDR_LEN => 52;
# Type code
use constant ELFCLASS32 => 1;
# Byte order
use constant ELFDATA2LSB => 1;
# ELF version
use constant EV_CURRENT => 1;
# File type
use constant ET_EXEC => 2;
# Machine type
use constant EM_386 => 3;
# Size of each program header
use constant PROG_HDR_LEN => 32;
# Type of header
use constant PT_LOAD => 1;
use constant PT_NOTE => 4;
# Size of each section header (there is just one)
use constant SECT_HDR_LEN => 40;
# Note types
use constant EIN_PROGRAM_NAME     => 0x00000001;
use constant EIN_PROGRAM_VERSION  => 0x00000002;
use constant EIN_PROGRAM_CHECKSUM => 0x00000003;

sub new {
	my $class = shift;
	my $self = { };
	$self->{libdir} = shift;
	$self->{segdescs} = [];
	$self->{offset} = 0;	# cumulative offset from beginning of file
	$self->{checksum} = 0;	# cumulative checksum of the file
	$self->{summed} = 0;	# number of bytes checksummed
	$self->{data} = "";	# string buffer containing the output file
	bless $self, $class;
#	$self->_initialize();
	return $self;
}

sub add_pm_header ($$$$$)
{
	my ($self, $vendorinfo, $headerseg, $bootaddr, $progreturns) = @_;

	push(@{$self->{segdescs}}, pack('A4C4@16v2V5v6',
		MAGIC, ELFCLASS32, ELFDATA2LSB, EV_CURRENT,
		255,		# embedded ABI
		ET_EXEC,	# e_type
		EM_386, 	# e_machine
		EV_CURRENT,	# e_version
		$bootaddr,	# e_entry
		ELF_HDR_LEN,	# e_phoff
		0,		# e_shoff (come back and patch this)
		($progreturns ? 0x8000000 : 0),		# e_flags
		ELF_HDR_LEN,	# e_ehsize
		PROG_HDR_LEN,	# e_phentsize
		0,		# e_phnum (come back and patch this)
		SECT_HDR_LEN,	# e_shentsize
		1,		# e_shnum, one mandatory entry 0
		0));		# e_shstrndx
	$self->{offset} = HEADERSIZE;
}

sub compute_ip_checksum
{
	my ($str) = @_;
	my ($checksum, $i, $size, $shorts);
	$checksum = 0;
	$size = length($$str);
	$shorts = $size >> 1;
	# Perl has a fairly large loop overhead so a straight forward
	# implementation of the ip checksum is intolerably slow.
	# Instead we use the unpack checksum computation function,
	# and sum 16bit little endian words into a 32bit number, on at
	# most 64K of data at a time.  This ensures we do not overflow
	# the 32bit sum allowing carry wrap around to be implemented by
	# hand.
	for($i = 0; $i < $shorts; $i += 32768) {
		$checksum += unpack("%32v32768", substr($$str, $i <<1, 65536));
		while($checksum > 0xffff) {
			$checksum = ($checksum & 0xffff) + ($checksum >> 16);
		}
	}
	if ($size & 1) {
		$checksum += unpack('C', substr($$str, -1, 1));
		while($checksum > 0xffff) {
			$checksum = ($checksum & 0xffff) + ($checksum >> 16);
		}
	}
	$checksum = (~$checksum) & 0xFFFF;
	return $checksum;
}

sub add_summed_data
{
	my ($self, $str) = @_;
	my $new_sum = compute_ip_checksum($str);
	my $new = $new_sum;
	my $sum = $self->{checksum};
	my $checksum;
	$sum = ~$sum & 0xFFFF;
	$new = ~$new & 0xFFFF;
	if ($self->{summed} & 1) {
		$new = (($new >> 8) & 0xff) | (($new << 8) & 0xff00);
	}
	$checksum = $sum + $new;
	if ($checksum > 0xFFFF) {
		$checksum -= 0xFFFF;
	}
	$self->{checksum} = (~$checksum) & 0xFFFF;
	$self->{summed} += length($$str);
	print "$$str";
#	$self->{data} .= $$str;
#	print STDERR sprintf("sum: %02x %02x sz: %08x summed: %08x\n",
#		$new_sum, $self->{checksum}, length($$str), $self->{summed});
}


# This should not get called as we don't cater for real mode calls but
# is here just in case
sub add_header ($$$$$)
{
	my ($self, $vendorinfo, $headerseg, $bootseg, $bootoff) = @_;

	$self->add_pm_header($vendorinfo, $headerseg, ($bootseg << 4) + $bootoff, 0);
}

sub roundup ($$)
{
# Round up to next multiple of $blocksize, assumes that it's a power of 2
	my ($size, $blocksize) = @_;

	# Default to TFTPBLOCKSIZE if not specified
	$blocksize = TFTPBLOCKSIZE if (!defined($blocksize));
	return ($size + $blocksize - 1) & ~($blocksize - 1);
}

# Grab N bytes from a file
sub peek_file ($$$$)
{
	my ($self, $descriptor, $dataptr, $datalen) = @_;
	my ($file, $fromoff, $status);

	$file = $$descriptor{'file'} if exists $$descriptor{'file'};
	$fromoff = $$descriptor{'fromoff'} if exists $$descriptor{'fromoff'};
	return 0 if !defined($file) or !open(R, "$file");
	binmode(R);
	if (defined($fromoff)) {
		return 0 if !seek(R, $fromoff, SEEK_SET);
	}
	# Read up to $datalen bytes
	$status = read(R, $$dataptr, $datalen);
	close(R);
	return ($status);
}

# Add a segment descriptor from a file or a string
sub add_segment ($$$)
{
	my ($self, $descriptor, $vendorinfo) = @_;
	my ($file, $string, $segment, $len, $maxlen, $fromoff, $align,
		$id, $end, $vilen);

	$end = 0;
	$file = $$descriptor{'file'} if exists $$descriptor{'file'};
	$string = $$descriptor{'string'} if exists $$descriptor{'string'};
	$segment = $$descriptor{'segment'} if exists $$descriptor{'segment'};
	$len = $$descriptor{'len'} if exists $$descriptor{'len'};
	$maxlen = $$descriptor{'maxlen'} if exists $$descriptor{'maxlen'};
	$fromoff = $$descriptor{'fromoff'} if exists $$descriptor{'fromoff'};
	$align = $$descriptor{'align'} if exists $$descriptor{'align'};
	$id = $$descriptor{'id'} if exists $$descriptor{'id'};
	$end = $$descriptor{'end'} if exists $$descriptor{'end'};
	if (!defined($len)) {
		if (defined($string)) {
			$len = length($string);
		} else {
			if (defined($fromoff)) {
				$len = (-s $file) - $fromoff;
			} else {
				$len = -s $file;
			}
			return 0 if !defined($len);		# no such file
		}
	}
	if (defined($align)) {
		$len = &roundup($len, $align);
	} else {
		$len = &roundup($len);
	}
	$maxlen = $len if (!defined($maxlen));
	push(@{$self->{segdescs}}, pack('V8',
		PT_LOAD,
		$self->{offset},	# p_offset
		$segment << 4,		# p_vaddr
		$segment << 4,		# p_paddr
		$len,			# p_filesz
		$len,			# p_memsz == p_filesz
		7,			# p_flags == rwx
		TFTPBLOCKSIZE));	# p_align
	$self->{offset} += $len;
	return ($len);			# assumes always > 0
}

sub pad_with_nulls ($$$)
{
	my ($self, $i, $blocksize) = @_;

	$blocksize = TFTPBLOCKSIZE if (!defined($blocksize));
	# Pad with nulls to next block boundary
	$i %= $blocksize;
	if ($i != 0) {
		# Nulls do not change the checksum
		print "\0" x ($blocksize - $i);
#		$self->{data} .= "\0" x ($blocksize - $i);
		$self->{summed} += ($blocksize - $i);
	}
}

# Copy data from file to stdout
sub copy_file ($$)
{
	my ($self, $descriptor) = @_;
	my ($i, $file, $fromoff, $align, $len, $seglen, $nread, $data, $status);

	$file = $$descriptor{'file'} if exists $$descriptor{'file'};
	$fromoff = $$descriptor{'fromoff'} if exists $$descriptor{'fromoff'};
	$align = $$descriptor{'align'} if exists $$descriptor{'align'};
	$len = $$descriptor{'len'} if exists $$descriptor{'len'};
	return 0 if !open(R, "$file");
	if (defined($fromoff)) {
		return 0 if !seek(R, $fromoff, SEEK_SET);
		$len = (-s $file) - $fromoff if !defined($len);
	} else {
		$len = -s $file if !defined($len);
	}
	binmode(R);
	# Copy file in TFTPBLOCKSIZE chunks
	$nread = 0;
	while ($nread != $len) {
		$status = read(R, $data, TFTPBLOCKSIZE);
		last if (!defined($status) or $status == 0);
		$self->add_summed_data(\$data);
		$nread += $status;
	}
	close(R);
	if (defined($align)) {
		$self->pad_with_nulls($nread, $align);
	} else {
		$self->pad_with_nulls($nread);
	}
	return ($nread);
}

# Copy data from string to stdout
sub copy_string ($$)
{
	my ($self, $descriptor) = @_;
	my ($i, $string, $len, $align, $data);

	$string = $$descriptor{'string'} if exists $$descriptor{'string'};
	$len = $$descriptor{'len'} if exists $$descriptor{'len'};
	$align = $$descriptor{'align'} if exists $$descriptor{'align'};
	return 0 if !defined($string);
	$len = length($string) if !defined($len);
	$data = substr($string, 0, $len);
	$self->add_summed_data(\$data);
	defined($align) ? $self->pad_with_nulls($len, $align) : $self->pad_with_nulls($len);
	return ($len);
}

sub dump_segments
{
	my ($self) = @_;
	my ($s, $nsegs, @segdescs);

	# generate the note header
	my $notes = pack('V3Z8S2',
		8,			# n_namesz
		2,			# n_descsz
		EIN_PROGRAM_CHECKSUM,	# n_type
		"ELFBoot",		# n_name
		0, 			# n_desc (Initial checksum value)
		0);			# padding to a 4byte boundary
	my $note_len = length($notes);

	# Add the note header
	push(@{$self->{segdescs}}, pack('V8',
		PT_NOTE,		# p_type
		HEADERSIZE - $note_len,	# p_offset
		0,			# p_vaddr
		0,			# p_paddr
		$note_len,		# p_filesz
		0,			# p_memsz == p_filesz
		0,			# p_flags
		0));			# p_align
					
	@segdescs = @{$self->{segdescs}};
	$nsegs = $#segdescs;	# number of program header entries
	# fill in e_phnum
	substr($segdescs[0], 44, 2) = pack('v', $nsegs);
	# fill in e_shoff to point to a record after program headers
	substr($segdescs[0], 32, 4) = pack('V',
		ELF_HDR_LEN + PROG_HDR_LEN * $nsegs);
	$self->{checksum} = 0;
	$self->{summed} = 0;
	while ($s = shift(@segdescs)) {
		$self->add_summed_data(\$s);
	}
	# insert section header 0
	# we just need to account for the length, the null fill
	# will create the record we want
	# warn if we have overflowed allocated header area
	print STDERR "Warning, too many segments in file\n"
		if ($self->{summed} > HEADERSIZE - SECT_HDR_LEN - $note_len);
	print "\0" x (HEADERSIZE - $self->{summed});
#	$self->{data} .= "\0" x (HEADERSIZE - $self->{summed});

	# Write the note header;
	seek(STDOUT, HEADERSIZE - $note_len, SEEK_SET) or die "Cannot seek to note header\n";
	print "$notes";
#	substr($self->{data}, HEADERSIZE - $note_len, $note_len) = $notes
}

sub finalise_image 
{
	my ($self) = @_;
	# Fill in the checksum
	seek(STDOUT, HEADERSIZE - 4, SEEK_SET) or die "Cannot seek to checksum\n";
	print pack('S', $self->{checksum});
#	substr($self->{data}, (HEADERSIZE - 4), 2) = pack('S', $self->{checksum});
#	print $self->{data};
}


1;
