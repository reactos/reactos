This is the svga ui port
send any fixes or improvments to me Jay Sorg(j@american-data.com)
svgalib should be installed
tested with versions 1.4.3, 1.9.x

thanks to
  Donald Gordon - original work
  Peter Nikolow - misc fixes

run make -f makefile_svga to compile it

svgareadme.txt - notes, this file
makefile_svga - makefile
svgawin.c - ui lib

svgalib has some support for acceleration but most drivers
do not support it.  I hope they fix this.
The ones that do are Cirus Logic and ATI Mach 32 cards.
If running on really slow hardware(486), use one of these cards,
it improves performance alot.

run ./svgardesktop with no parameters to see a list of
commnad line options

You will need to modify the libvga.config file most likely.
Its in /etc/vga.
Here is what mine looks like.
BOF
 mouse imps2
 mouse_fake_kbd_event 112 113
 mouse_accel_mult 1.5
 mouse_accel_type normal
 HorizSync 31.5 56.0
 VertRefresh 50 90
 nosigint
EOF
The mouse_fake_kbd_event line makes the wheel mouse work.

Jay
