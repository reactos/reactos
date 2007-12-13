#!/usr/bin/perl -w

# Program to create a netboot image for ROM/FreeDOS/DOS/Linux
# Placed under GNU Public License by Ken Yap, December 2000

# 2003.04.28 R. Main
#  Tweaks to work with new first-dos.S for large disk images

BEGIN {
	push(@INC, '@@LIBDIR@@');
}

use strict;
use Getopt::Long;
use IO::Seekable;
use Socket;

use Elf;

use constant;
use constant DEBUG => 0;
use constant LUA_VERSION => 0x04000100;	# 4.0.1.0

use bytes;

use vars qw($libdir $version $format $target $output $module $relocseg $relocsegstr
	$progreturns $param $append $rootdir $rootmode $ip $ramdisk $rdbase
	$simhd $dishd $squashfd $first32 $showversion);

sub check_file
{
	my ($f, $status);

	$status = 1;
	foreach $f (@_) {
		if (!-e $f) {
			print STDERR "$f: file not found\n";
			$status = 0;
		} elsif (!-f $f) {
			print STDERR "$f: not a plain file\n";
			$status = 0;
		} elsif (!-r $f) {
			print STDERR "$f: file not readable\n";
			$status = 0;
		}
	}
	return ($status);
}

sub mkelf_img ($)
{
	my ($module) = @_;
	my ($romdesc);

	$#ARGV >= 0 or die "Usage: $0 .img-file\n";
	return unless check_file($ARGV[0]);
	$module->add_pm_header("mkelf-img-$version", $relocseg + 0x3E0, 0x100000, 0);
	$romdesc = { file => $ARGV[0],
		segment => 0x10000,
		maxlen => 0x10000,
		id => 20,
		end => 1 };
	$module->add_segment($romdesc);
	$module->dump_segments();
	$module->copy_file($romdesc);
}

$libdir = '@@LIBDIR@@';		# where config and auxiliary files are stored

$version = '@@VERSION@@';
$showversion = '';
$simhd = 0;
$dishd = 0;
$squashfd = 1;
$relocsegstr = '0x9000';
$progreturns = 0;
GetOptions('format=s' => \$format,
	'target=s' => \$target,
	'output=s' => \$output,
	'param=s' => \$param,
	'append=s' => \$append,
	'rootdir=s' => \$rootdir,
	'rootmode=s' => \$rootmode,
	'ip=s' => \$ip,
	'rdbase=s' => \$rdbase,
	'harddisk!' => \$simhd,
	'disableharddisk!' => \$dishd,
	'squash!' => \$squashfd,
	'first32:s' => \$first32,
	'progreturns!' => \$progreturns,
	'relocseg=s' => \$relocsegstr,
	'version' => \$showversion);

if ($showversion) {
	print STDERR "$version\n";
	exit 0;
}

if (defined($ENV{LANG}) and $ENV{LANG} =~ /\.UTF-8$/i) {
	print STDERR <<'EOF';
Warning: Perl 5.8 may have a bug that affects handing of strings in Unicode
locales that may cause misbehaviour with binary files.  To work around this
problem, set $LANG to not have a suffix of .UTF-8 before running this program.
EOF
}

$format = "elf";
$target = "img";

if (!defined($format)) {
	print STDERR "No format specified with program name or --format=\n";
	exit 1;
}
if (!defined($target)) {
	print STDERR "No target specified with program name or --target=\n";
	exit 1;
}
if (defined($output)) {
	die "$output: $!\n" unless open(STDOUT, ">$output");
}
binmode(STDOUT);

if ($format eq 'elf') {
	$first32 = '' if !defined($first32);
	$module = Elf->new($libdir);
	die "Output must be file\n" unless (seek(STDOUT, 0, SEEK_SET));
} else {
	die "Format $format not supported\n";
}
if ($relocsegstr eq '0x9000' or $relocsegstr eq '0x8000') {
	$relocseg = hex($relocsegstr);
} else {
	print STDERR "relocseg must be 0x9000 or 0x8000 only, setting to 0x9000\n";
	$relocseg = 0x9000;
}
if ($target eq 'img') {
	&mkelf_img($module);
} else {
	print STDERR "Target $target not supported\n";
	exit;
}
$module->finalise_image();
close(STDOUT);
exit 0;

__END__

=head1 NAME

mknbi - make network bootable image

=head1 SYNOPSIS

B<mknbi> --version

B<mknbi> --format=I<format> --target=I<target> [--output=I<outputfile>] I<target-specific-arguments>

B<mkelf-linux> [--output=I<outputfile>] I<kernelimage> [I<ramdisk>]

B<mknbi-linux> [--output=I<outputfile>] I<kernelimage> [I<ramdisk>]

B<mknbi-rom> [--output=I<outputfile>] I<.z?rom-file>

B<mkelf-img> [--output=I<outputfile>] I<.z?img-file>

B<mkelf-menu> [--output=I<outputfile>] [I<dataimage>]

B<mknbi-menu> [--output=I<outputfile>] [I<dataimage>]

B<mkelf-nfl> [--output=I<outputfile>] [I<dataimage>]

B<mknbi-nfl> [--output=I<outputfile>] [I<dataimage>]

B<mkelf-lua> [--output=I<outputfile>] I<luabin>

B<mknbi-fdos> [--output=I<outputfile>] I<kernel.sys floppyimage>

B<mknbi-dos> [--output=I<outputfile>] I<floppyimage>

=head1 DESCRIPTION

B<mknbi> is a program that makes network bootable images for various
operating systems suitable for network loading by Etherboot or Netboot,
which are ROM boot loaders.  If you are looking to boot using PXE, look
no further, mknbi is not what you want. You probably want something like
PXELINUX which is part of the SYSLINUX package.

B<mknbi> --version prints the current version. Use this before reporting
problems.

B<mknbi> can be invoked with the B<--format> and B<--target> options or
links can be made to it under format and target specific names. E.g.
mkelf-linux is the same as mknbi --format=elf --target=linux.

B<--format>=I<format> Specify the format of the output. Currently
available are nbi and elf.  ELF format only works with linux and menu.
Otherwise the invocation is the same as for mknbi. In discussions below,
the mknbi form is used.

B<--target>=I<target> Specify the target binary. Currently available are
linux, menu, rom, fdos and dos. B<mknbi> is not needed for booting
FreeBSD.

B<--output=>I<outputfile> Specify the output file, can be used with
all variants.  Stdout is the default.

The package must be installed in the destination location before the
executables can be run, because it looks for library files.

Each of the variants will be described separately.

=head1 MKELF-LINUX

B<mkelf-linux> and B<mknbi-linux> makes a boot image from a Linux kernel
image, either a zImage or a bzImage.

=head1 MKELF-LINUX OPTIONS

B<--param=>I<string> Replace the default parameter string with the
specified one. This option overrides all the following options so you
should know what you are doing.

B<--append>=I<string> Appends the specified string to the existing
parameter string. This option operates after the other parameter options
have been evaluated.

B<--rootdir>=I<rootdir> Define name of directory to mount via NFS from
the boot server.

In the absence of this option, the default is to use the directory
C</tftpboot/>I<%s>, with the I<%s> representing the hostname or
IP-address of the booting system, depending on whether the hostname
attribute is present in the BOOTP/DHCP reply.

If C<rom> is given, and if the BOOTP/DHCP server is able to handle the RFC 1497
extensions, the value of the rootpath option is used as the root directory.

If the name given to the option starts with C</dev/>, the corresponding
device is used as the root device, and no NFS directory will be mounted.

B<--rootmode>=C<ro|rw> Defines whether the root device will be mounted
read-only or read-write respectively. Without this parameter, the
default is C<rw>.

B<--ip=>I<string> Define client and server IP addresses.

In the absence of this option no IP addresses are defined, and the
kernel will determine the IP addresses by itself, usually by using DHCP,
BOOTP or RARP.  Note that the kernel's query is I<in addition to> the
query made by the bootrom, and requires the IP: kernel level
autoconfiguration (CONFIG_IP_PNP) feature to be included in the kernel.

Important note: In Linux kernels 2.2.x where x >= 18, and 2.4.x where x
>= 5, it is B<necessary> to specify one of the enabling options in the
next paragraph to cause the IP autoconfiguration to be activated.
Unlike in previous kernels, IP autoconfiguration does not happen by
default. Also note that IP autoconfiguration and NFSroot are likely to
go away in Linux 2.6 and that userspace IP configuration methods using
ramdisk and userspace DHCP daemons are preferred now.

If one of the following: C<off, none, on, any, dhcp, bootp, rarp, both>,
is given, then the option will be passed unmodified to the kernel and
cause that autoconfig option to be chosen.

If C<rom> is given as the argument to this option, all necessary IP
addresses for NFS root mounting will be inherited from the BOOTP/DHCP
answer the bootrom got from the server.

It's also possible to define the addresses during compilation of the boot
image. Then, all addresses must be separated by a colon, and ordered in
the following way:

C<--ip=>I<client:server:gateway:netmask:hostname[:dev[:proto]]>

Using this option B<mkelf-linux> will automatically convert system names
into decimal IP addresses for the first three entries in this string.
The B<hostname> entry will be used by the kernel to set the host name of
the booted Linux diskless client.  When more than one network interface
is installed in the diskless client, it is possible to specify the name
of the interface to use for mounting the root directory via NFS by
giving the optional value C<dev>.  This entry has to start with the
string C<eth> followed by a number from 0 to 9. However, if only one
interface is installed in the client, this I<dev> entry including the
preceding semicolon can be left out. The I<proto> argument is one of the
IP autoconfiguration enabling options listed above.  (Author: it's not
clear to me what the IP autoconfiguration does when the parameters are
already specified.  Perhaps it's to obtain parameters not specified,
e.g. NIS domain.)

B<--rdbase=>I<top|asis|0xNNNNNNNN> Set the ramdisk load address.  C<top>
moves the ramdisk to the top of memory before jumping to the kernel.
This is the default if rdbase is not specified.  This option requires
that first-linux's kernel sizing work correctly.  C<asis> loads it at
0x100000 (1MB) if the kernel is loaded low; or leaves it just after the
kernel in memory, if the kernel is loaded high. For this option to work,
the kernel must be able to handle ramdisks at these addresses.
I<0xNNNNNNNN> moves the ramdisk to the hex address specified. The onus
is on the user to specify a suitable address that is acceptable to the
kernel and doesn't overlap with any other segments. It will have to be
aligned to a 4k byte boundary so you should ensure that this is so. (The
last three hex digits must be 0.)

B<--first32=>I<program> Override the default first stage setup
program.  It can be used to call extensions to the Etherboot code, which
paves the way for additional useful functionality without enlarging the
size of the Etherboot footprint.  --first32 is implied by the ELF
format.

B<--progreturns> This option is used in conjunction with and only valid
with the --first32 option to indicate to the Etherboot loader that the
called program will return to loader and hence Etherboot should not
disable the network device as is the case when the program will never
return to Etherboot.

B<--relocseg=>I<segaddr> This option is used to specify a relocation of
the Linux first, boot, setup, and parameter segments to another 64k
band.  Currently the only valid values are 0x9000 and 0x8000,
corresponding to linear addresses of 0x90000 and 0x80000 upwards. The
default is 0x9000.  Usually you use this option if you have relocated
Etherboot to 0x84000 to avoid other code in the 0x90000 segment like
DOC. The Linux kernel must support relocation which implies a 2.4 kernel
or later. --relocseg only works reliably with ELF or --first32=.

B<mem=>I<memsize> This is not a command line option but a kernel
parameter that is intercepted by the first32 stage and used as the top
of memory, to match Linux's interpretation. I<memsize> can be suffixed
by C<G> to indicate gibibytes (times 2^30), C<M> to indicate mebibytes
(times 2^20) or C<K> to indicate kibibytes (times 2^10). Note that the
suffixes are uppercase. This kernel parameter can be specified in
--append= or option-129 of the DHCP/BOOTP record.

Run the program thus:

mkelf-linux I<kernel-image> [I<ramdisk-image>] > linux.nb

Then move F<linux.nb> to where the network booting process expects to
find it.

=head1 MKELF-LINUX BOOTP/DHCP VENDOR TAGS

B<mkelf-linux> includes a startup code at the beginning of the Linux
kernel which is able to detect certain DHCP vendor defined options.
These can be used to modify the kernel loading process at runtime. To
use these options with ISC DHCPD v3, a popular DHCP daemon, the syntax
is as below. You will need to adjust the syntax for other DHCP or BOOTP
daemons.

option etherboot-signature code 128 = string;

option kernel-parameters code 129 = text;

...

		option etherboot-signature E4:45:74:68:00:00;

		option kernel-parameters "INITRD_DBG=6 NIC=3c509";

Option 128 is required to be the six byte signature above. See the
vendortags appendix of the Etherboot user manual for details.

The following option is presently supported by B<mkelf-linux>:

B<129> The I<string> value given with this option is appended verbatim to
the end of the kernel command line.  It can be used to specify arguments
like I/O addresses or DMA channels required for special hardware
like SCSI adapters, network cards etc. Please consult the Linux kernel
documentation about the syntax required by those options. It is the same
as the B<--append> command line option to B<mkelf-linux>, but works at
boot time instead of image build time.

B<130> With this option it is possible to the select the network adapter
used for mounting root via NFS on a multihomed diskless client. The
syntax for the I<string> value is the same as for the C<dev> entry used
with the B<--ip=> option as described above. However note that the
B<mkelf-linux> runtime setup routine does not check the syntax of the
string.

=head1 MKNBI-ROM

B<mknbi-rom> makes a boot image from an Etherboot C<.rom> or C<.zrom>
boot ROM image.  This allows it to be netbooted using an existing
ROM. This is useful for developing Etherboot drivers or to load a newer
version of Etherboot with an older one.

Run mknbi-rom like this:

mknbi-rom nic.zrom > nic.nb

Move F<nic.nb> to where the network booting process expects to find it.
The boot ROM will load this as the I<operating system> and execute the
ROM image.

=head1 MKELF-IMG

B<mkelf-img> makes a boot image from an Etherboot C<.img> or C<.zimg>
image.  This allows it to be netbooted using an existing ROM. This is
useful for developing Etherboot drivers or to load a newer version of
Etherboot with an older one.

Run mkelf-img like this:

mkelf-img nic.zimg > nic.nb

Move F<nic.nb> to where the network booting process expects to find it.
The boot ROM will load this as the I<operating system> and execute the
image.

Note that this does not test the ROM loader portion that's in a C<.z?rom>
image, but not in a C<.z?img>.

=head1 MKELF-MENU

B<mkelf-menu> and B<mknbi-menu> make a boot image from an auxiliary menu
program. Etherboot has the ability to load an auxiliary program which
can interact with the user, modify the DHCP structure, and return a
status.  Based on the status, Etherboot can load another binary, restart
or exit.  This makes it possible to have elaborate user interface
programs without having to modify Etherboot. The specification for
auxiliary program is documented in the Etherboot Developer's Manual.

B<mkelf-menu> and B<mknbi-menu> take a binary named C<menu> from the
library directory, which is assumed to have an entry point of 0x60000.
An optional argument is accepted, and this is loaded at 0x80000. This
can be a data file used by the menu program.

Currently, the menu binary provided duplicates the builtin menu facility
of Etherboot with the exception of a couple of small differences: no
server or gateway specifications are used and nested TFTP loads don't
work. You should not have MOTD or IMAGE_MENU defined in your Etherboot
build to be able to use this external menu binary. The specifications of
the DHCP option required is in the vendortags document in the Etherboot
user manual.

Typical usage is like this:

mkelf-menu > menu.nb

Then put menu.nb in the TFTP boot directory and edit your DHCP options
according to the documentation.

Alternate user interface programs are highly encouraged.

=head1 MKELF-NFL

B<mkelf-nfl> and B<mknbi-nfl> make a boot image from the NFL menu
program. This menu program takes the names of images from a
menu-text-file file which just contains lines with the filenames
(relative to the tftpd root directory) of images to load. The
user-interface is a light-bar, similar to that used in GRUB.  There is a
sample menu-text-file in C<menu-nfl.eg>. The special entry "Quit
Etherboot" (without quotes, of course) can be used in menu-text-files
as an entry that causes Etherboot to quit and return to the invoking
environment, which is the BIOS in the case of ROMs.

Typical usage is:

mkelf-nfl I<menu-text-file> > nfl.nb

Then put nfl.nb in the TFTP boot directory and specify as the boot
image. Chaining to other menus works.

Enhancements to the menu format accepted to specify other features such
as titles, timeout, colours, and so forth are highly encouraged.

=head1 MKELF-LUA

B<mkelf-lua> makes an ELF image from a precompiled Lua
(C<http://www.tecgraf.puc-rio.br/lua/>) program.

Typical usage is:

mkelf-lua hello.lb > luaprog.nb

where C<hello.lb> was generated from a Lua program by:

luac -o hello.lb hello.lua

The functions available to Lua programs in this environment is described
in a separate document.

=head1 MKNBI-FDOS

B<mknbi-fdos> makes a boot image from a FreeDOS kernel file and a floppy
image.  Note that the kernel image is not read from the floppy section
of the boot image, but is a separate section in the boot image. The
bootloader has been adjusted to jump to it directly. This means the
space that would be taken up on the I<floppy> by the kernel image file
can now be used for applications and data.

Obtain a distribution of FreeDOS with a recent kernel, probably at least
2006. It has been tested with 2012 but nothing older. You can get the
FreeDOS kernel here:

C<http://freedos.sourceforge.net/>

Follow the instructions to make a bootable floppy. Then get an image
of the floppy with:

dd if=/dev/fd0 of=/tmp/floppyimage

Also extract F<kernel.sys> from the floppy. You can do this from the
image using the mtools package, by specifying a file as a I<drive>
with a declaration like this in F<~/.mtoolsrc>:

drive x: file="/tmp/floppyimage"

Then run:

mcopy x:kernel.sys .

Then run mknbi by:

mknbi-fdos kernel.sys /tmp/floppyimage > freedos.nb

where F<kernel.sys> and F</tmp/floppyimage> are the files extracted above.
Then move F<freedos.nb> to where the network booting process expects to
find it.

If you have got it to netboot successfully, then you can go back and
add your files to the floppy image. You can delete F<kernel.sys> in
the floppy image to save space, that is not needed. Note that you can
create a floppy image of any size you desire with the mformat program
from mtools, you are not restricted to the actual size of the boot floppy.

=head1 MKNBI-FDOS OPTIONS

B<--harddisk> Make the boot ramdisk the first hard disk, i.e. C:. One
reason you might want to do this is because you want to use the real
floppy. The limit on "disk size" in the boot image is not raised by this
option so that is not a reason to use this option. This option is
incompatible with --disableharddisk.

B<--disableharddisk> When the ramdisk is simulating a floppy disk drive,
this switch will disable hard disk accesses.  This is necessary if the
client should use a network file system as drive C:, which is only
possible if there are no hard disks found by DOS. This option is
incompatible with --harddisk.

B<--nosquash> Do not try to chop unused sectors from the end of the
floppy image. This increases the boot image size and hence loading
time if the FAT filesystem on the floppy is mostly empty but you may
wish to use this option if you have doubts as to whether the squashing
algorithm is working correctly.

B<--rdbase=>I<0xNNNNNNNN> Set the ramdisk load address. The default
load address for the ramdisk is 0x110000. It can be moved higher
(lower will not work) if for some reason you need to load other stuff
at the address it currently occupies. As this is a linear address and
not a segment address, the last 4 bits are not used and should be 0.

=head1 MKNBI-DOS

B<mknbi-dos> makes a boot image from a floppy image containing a
bootable DOS filesystem.  It is not necessary to build the filesystem on
a physical floppy if you have the mtools package, but you need a
bootable floppy of any size to start with. First extract the boot block
from the floppy, this boot block must match the DOS kernel files you
will copy in the next step:

dd if=/dev/fd0 of=bootblock bs=512 count=1

Then get the DOS kernel files (this is correct for DR-DOS, the names
are different in MS-DOS, IO.SYS and MSDOS.SYS):

mcopy a:IBMBIO.COM a:IBMDOS.COM a:COMMAND.COM .

Next make an entry in F<~/.mtoolsrc> to declare a floppy to be mapped
to a file:

drive x: file="/tmp/floppyimage"

Now format a floppy of the desired size, in this example a 2.88 MB floppy,
at the same time writing the bootblock onto it:

mformat -C -t 80 -s 36 -h 2 -B bootblock x:

The size of the "floppy" is only limited by the limits on the number of
cylinders, sectors and heads, which are 1023, 63 and 255 respectively,
and the amount of RAM you are willing to allocate to the "floppy" in
memory. As RAM is precious, choose a size slightly bigger than what is
needed to hold your "floppy" files.

Finally, copy all your desired files onto the floppy:

mcopy IBMBIO.COM x:

mcopy IBMDOS.COM x:

mcopy COMMAND.COM x:

mcopy CONFIG.SYS AUTOEXEC.BAT APP.EXE APP.DAT ... x:

For MS-DOS substitute IO.SYS for IBMIO.COM, and MSDOS.SYS for
IBMDOS.COM.  The case of the files must be preserved, it may not work if
VFAT lower case names are generated in the floppy image.  Pay attention
to the order of copying as the boot block may expect the first two
entries on a newly formatted disk to be IO.SYS, MSDOS.SYS.  Possibly too
COMMAND.COM has to be the third entry so we play safe.  Thanks to Phil
Davey and Phillip Roa for these tips.

I have reports that the bootblock of MS-DOS 6.22 sometimes fails to boot
the ramdisk.  You could try using the boot block from Netboot instead of
getting the boot block off the floppy. I have provided this boot block
in the distribution as altboot.bin, and in source form as altboot.S and
boot.inc. One essential thing is to make IO.SYS the first file on the
disk, or this bootblock will not work.

If you happen to have a media of the same size you could test if the
image is bootable by copying it onto the media, and then booting it:

dd if=/tmp/floppyimage of=/dev/fd0

Then run mknbi-dos over the image F</tmp/floppyimage> to create a
boot image:

mknbi-dos /tmp/floppyimage > dos.nb

Move F<dos.nb> to where the network booting process expects to find it.

=head1 MKNBI-DOS OPTIONS

B<--harddisk> Make the boot ramdisk the first hard disk, i.e. C:. One
reason you might want to do this is because you want to use the real
floppy. The limit on "disk size" in the boot image is not raised by this
option so that is not a reason to use this option. This option is
incompatible with --disableharddisk.

B<--disableharddisk> When the ramdisk is simulating a floppy disk drive,
this switch will disable hard disk accesses.  This is necessary if the
client should use a network file system as drive C:, which is only
possible if there are no hard disks found by DOS. This option is
incompatible with --harddisk.

B<--nosquash> Do not try to chop unused sectors from the end of the
floppy image. This increases the boot image size and hence loading
time if the FAT filesystem on the floppy is mostly empty but you may
wish to use this option if you have doubts as to whether the squashing
algorithm is working correctly.

B<--rdbase=>I<0xNNNNNNNN> Set the ramdisk load address. The default
load address for the ramdisk is 0x110000. It can be moved higher
(lower will not work) if for some reason you need to load other stuff
at the address it currently occupies. As this is a linear address and
not a segment address, the last 4 bits are not used and should be 0.

=head1 BUGS

Please report all bugs to Etherboot users mailing list:
<https://sourceforge.net/mail/?group_id=4233>

=head1 SEE ALSO

Etherboot tutorial at C<http://etherboot.sourceforge.net/> Mtools package
is at C<http://wauug.erols.com/pub/knaff/mtools/> Make sure you have a
recent version, the ability to map a drive to a file is not present in
old versions.

=head1 COPYRIGHT

B<mknbi> is under the GNU Public License

=head1 AUTHOR

Ken Yap

mk{elf,nbi}-nfl was contributed by Robb Main of Genedyne.

=head1 DATE

See man page footer for date and version. Sorry, not available in the
HTML version.
