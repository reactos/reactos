/* $Id: nvt.cpp,v 1.1 2001/01/27 22:38:43 ea Exp $
 *
 * FILE       : nvt.cpp
 * AUTHOR     : unknown (sources found on www.telnet.org)
 * PROJECT    : ReactOS Operating System
 * DESCRIPTION: telnet client for the W32 subsystem
 * DATE       : 2001-01-21
 * REVISIONS
 *	2001-02-21 ea	Modified to compile under 0.0.16 src tree
 */
///////////////////////////////////////////////////////////////////////////////
//
// file: nvt.cpp
// 
// purpose: Provides the "bare bones" telnet "Network Virtual Terminal"
//          that is our default. We only se a more capable terminal, if
//          properly requested via the telnet option.
//
// refrence: The following excerpt from rfc 854
//
///////////////////////////////////////////////////////////////////////////////
/*
THE NETWORK VIRTUAL TERMINAL
 
   The Network Virtual Terminal (NVT) is a bi-directional character
   device.  The NVT has a printer and a keyboard.  The printer responds
   to incoming data and the keyboard produces outgoing data which is
   sent over the TELNET connection and, if "echoes" are desired, to the
   NVT's printer as well.  "Echoes" will not be expected to traverse the
   network (although options exist to enable a "remote" echoing mode of
   operation, no host is required to implement this option).  The code
   set is seven-bit USASCII in an eight-bit field, except as modified
   herein.  Any code conversion and timing considerations are local
   problems and do not affect the NVT.
 
   TRANSMISSION OF DATA
 
      Although a TELNET connection through the network is intrinsically
      full duplex, the NVT is to be viewed as a half-duplex device
      operating in a line-buffered mode.  That is, unless and until
      options are negotiated to the contrary, the following default
      conditions pertain to the transmission of data over the TELNET
      connection:
 
         1)  Insofar as the availability of local buffer space permits,
         data should be accumulated in the host where it is generated
         until a complete line of data is ready for transmission, or
         until some locally-defined explicit signal to transmit occurs.
         This signal could be generated either by a process or by a
         human user.
 
         The motivation for this rule is the high cost, to some hosts,
         of processing network input interrupts, coupled with the
         default NVT specification that "echoes" do not traverse the
         network.  Thus, it is reasonable to buffer some amount of data
         at its source.  Many systems take some processing action at the
         end of each input line (even line printers or card punches
         frequently tend to work this way), so the transmission should
         be triggered at the end of a line.  On the other hand, a user
         or process may sometimes find it necessary or desirable to
         provide data which does not terminate at the end of a line;
         therefore implementers are cautioned to provide methods of
         locally signaling that all buffered data should be transmitted
         immediately.
 
         2)  When a process has completed sending data to an NVT printer
         and has no queued input from the NVT keyboard for further
         processing (i.e., when a process at one end of a TELNET
         connection cannot proceed without input from the other end),
         the process must transmit the TELNET Go Ahead (GA) command.
 
         This rule is not intended to require that the TELNET GA command
         be sent from a terminal at the end of each line, since server
         hosts do not normally require a special signal (in addition to
         end-of-line or other locally-defined characters) in order to
         commence processing.  Rather, the TELNET GA is designed to help
         a user's local host operate a physically half duplex terminal
         which has a "lockable" keyboard such as the IBM 2741.  A
         description of this type of terminal may help to explain the
         proper use of the GA command.
 
         The terminal-computer connection is always under control of
         either the user or the computer.  Neither can unilaterally
         seize control from the other; rather the controlling end must
         relinguish its control explicitly.  At the terminal end, the
         hardware is constructed so as to relinquish control each time
         that a "line" is terminated (i.e., when the "New Line" key is
         typed by the user).  When this occurs, the attached (local)
         computer processes the input data, decides if output should be
         generated, and if not returns control to the terminal.  If
         output should be generated, control is retained by the computer
         until all output has been transmitted.
 
         The difficulties of using this type of terminal through the
         network should be obvious.  The "local" computer is no longer
         able to decide whether to retain control after seeing an
         end-of-line signal or not; this decision can only be made by
         the "remote" computer which is processing the data.  Therefore,
         the TELNET GA command provides a mechanism whereby the "remote"
         (server) computer can signal the "local" (user) computer that
         it is time to pass control to the user of the terminal.  It
         should be transmitted at those times, and only at those times,
         when the user should be given control of the terminal.  Note
         that premature transmission of the GA command may result in the
         blocking of output, since the user is likely to assume that the
         transmitting system has paused, and therefore he will fail to
         turn the line around manually.
 
      The foregoing, of course, does not apply to the user-to-server
      direction of communication.  In this direction, GAs may be sent at
      any time, but need not ever be sent.  Also, if the TELNET
      connection is being used for process-to-process communication, GAs
      need not be sent in either direction.  Finally, for
      terminal-to-terminal communication, GAs may be required in
      neither, one, or both directions.  If a host plans to support
      terminal-to-terminal communication it is suggested that the host
      provide the user with a means of manually signaling that it is
      time for a GA to be sent over the TELNET connection; this,
      however, is not a requirement on the implementer of a TELNET
      process.
 
      Note that the symmetry of the TELNET model requires that there is
      an NVT at each end of the TELNET connection, at least
      conceptually.
*//*

   THE NVT PRINTER AND KEYBOARD
 
      The NVT printer has an unspecified carriage width and page length
      and can produce representations of all 95 USASCII graphics (codes
      32 through 126).  Of the 33 USASCII control codes (0 through 31
      and 127), and the 128 uncovered codes (128 through 255), the
      following have specified meaning to the NVT printer:
 
         NAME                  CODE         MEANING
 
         NULL (NUL)              0      No Operation
         Line Feed (LF)         10      Moves the printer to the
                                        next print line, keeping the
                                        same horizontal position.
         Carriage Return (CR)   13      Moves the printer to the left
                                        margin of the current line.
 
         In addition, the following codes shall have defined, but not
         required, effects on the NVT printer.  Neither end of a TELNET
         connection may assume that the other party will take, or will
         have taken, any particular action upon receipt or transmission
         of these:
 
         BELL (BEL)              7      Produces an audible or
                                        visible signal (which does
                                        NOT move the print head).
         Back Space (BS)         8      Moves the print head one
                                        character position towards
                                        the left margin.
         Horizontal Tab (HT)     9      Moves the printer to the
                                        next horizontal tab stop.
                                        It remains unspecified how
                                        either party determines or
                                        establishes where such tab
                                        stops are located.
         Vertical Tab (VT)       11     Moves the printer to the
                                        next vertical tab stop.  It
                                        remains unspecified how
                                        either party determines or
                                        establishes where such tab
                                        stops are located.
         Form Feed (FF)          12     Moves the printer to the top
                                        of the next page, keeping
                                        the same horizontal position.
 
      All remaining codes do not cause the NVT printer to take any
      action.
 
      The sequence "CR LF", as defined, will cause the NVT to be
      positioned at the left margin of the next print line (as would,
      for example, the sequence "LF CR").  However, many systems and
      terminals do not treat CR and LF independently, and will have to
      go to some effort to simulate their effect.  (For example, some
      terminals do not have a CR independent of the LF, but on such
      terminals it may be possible to simulate a CR by backspacing.)
      Therefore, the sequence "CR LF" must be treated as a single "new
      line" character and used whenever their combined action is
      intended; the sequence "CR NUL" must be used where a carriage
      return alone is actually desired; and the CR character must be
      avoided in other contexts.  This rule gives assurance to systems
      which must decide whether to perform a "new line" function or a
      multiple-backspace that the TELNET stream contains a character
      following a CR that will allow a rational decision.
 
         Note that "CR LF" or "CR NUL" is required in both directions
         (in the default ASCII mode), to preserve the symmetry of the
         NVT model.  Even though it may be known in some situations
         (e.g., with remote echo and suppress go ahead options in
         effect) that characters are not being sent to an actual
         printer, nonetheless, for the sake of consistency, the protocol
         requires that a NUL be inserted following a CR not followed by
         a LF in the data stream.  The converse of this is that a NUL
         received in the data stream after a CR (in the absence of
         options negotiations which explicitly specify otherwise) should
         be stripped out prior to applying the NVT to local character
         set mapping.
 
      The NVT keyboard has keys, or key combinations, or key sequences,
      for generating all 128 USASCII codes.  Note that although many
      have no effect on the NVT printer, the NVT keyboard is capable of
      generating them.
 
      In addition to these codes, the NVT keyboard shall be capable of
      generating the following additional codes which, except as noted,
      have defined, but not reguired, meanings.  The actual code
      assignments for these "characters" are in the TELNET Command
      section, because they are viewed as being, in some sense, generic
      and should be available even when the data stream is interpreted
      as being some other character set.
 
      Synch
 
         This key allows the user to clear his data path to the other
         party.  The activation of this key causes a DM (see command
         section) to be sent in the data stream and a TCP Urgent
         notification is associated with it.  The pair DM-Urgent is to
         have required meaning as defined previously.
 
      Break (BRK)
 
         This code is provided because it is a signal outside the
         USASCII set which is currently given local meaning within many
         systems.  It is intended to indicate that the Break Key or the
         Attention Key was hit.  Note, however, that this is intended to
         provide a 129th code for systems which require it, not as a
         synonym for the IP standard representation.
 
      Interrupt Process (IP)
 
         Suspend, interrupt, abort or terminate the process to which the
         NVT is connected.  Also, part of the out-of-band signal for
         other protocols which use TELNET.

      Abort Output (AO)
 
         Allow the current process to (appear to) run to completion, but
         do not send its output to the user.  Also, send a Synch to the
         user.
 
      Are You There (AYT)
 
         Send back to the NVT some visible (i.e., printable) evidence
         that the AYT was received.
 
      Erase Character (EC)
 
         The recipient should delete the last preceding undeleted
         character or "print position" from the data stream.
 
      Erase Line (EL)
 
         The recipient should delete characters from the data stream
         back to, but not including, the last "CR LF" sequence sent over
         the TELNET connection.
 
      The spirit of these "extra" keys, and also the printer format
      effectors, is that they should represent a natural extension of
      the mapping that already must be done from "NVT" into "local".
      Just as the NVT data byte 68 (104 octal) should be mapped into
      whatever the local code for "uppercase D" is, so the EC character
      should be mapped into whatever the local "Erase Character"
      function is.  Further, just as the mapping for 124 (174 octal) is
      somewhat arbitrary in an environment that has no "vertical bar"
      character, the EL character may have a somewhat arbitrary mapping
      (or none at all) if there is no local "Erase Line" facility.
      Similarly for format effectors:  if the terminal actually does
      have a "Vertical Tab", then the mapping for VT is obvious, and
      only when the terminal does not have a vertical tab should the

*/

#include <winsock.h>
#include <windows.h>

#include "telnet.h"

void nvt(SOCKET server,unsigned char data)
{
  DWORD z;
  switch(data)
  {
  case 0:  //eat null codes.
    break;
  default: //Send all else to the console.
    WriteConsole(StandardOutput, & data, 1, & z, NULL);
    break;
  }
}

/* EOF */
