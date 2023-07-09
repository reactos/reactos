From: 	Chris Mah (Independent Contractor) (Exchange)
Sent: 	Tuesday, April 15, 1997 11:46 PM
To: 	Alan Back
Subject: 	Help with NT service program

Hi Alan,

I'm a contractor working for Bryan Cooke on the Microsoft Mail NT MMTA program.
I've run into a problem trying to fix a bug in the program that appears only to
happen in NT 4.0 but not in 3.51.

Bryan said I could contact you for some possible help with this.

Below are some notes that I've made regarding the problem.

I've tracked the problem down to NT 4.0 not being able to send an ESC character
to a console app. from the service program.  All other characters can be sent
from 3.51 and 4.0 except the ESC character.  If we can make the service program
send the ESC character, the bug will be fixed.  We may need to contact someone
in the NT group that understands this problem better.  I cannot find a reason
why the ESC character cannot be sent in NT 4.0.

I can fix the bug by putting in a kludge in External where instead of pressing
"ESC y" to stop the program, just pressing "y" will stop the program.  I think
the service program though is where we should really be doing the fix.

Here is the code that actually sends the "ESC y" sequence.
<< attacment of code fragment - see writescy.c >>

I've also tested this code by using a for loop to try sending ascii characters
1 to 122.  Both 3.51 and 4.0 will send the same characters except 4.0 will not
send ascii 27, the ESC character.  It's almost as if it's being filtered for
some reason.

I'd really appreciate any help that you can give on this.

Thanks,
Chris Mah
