This doc can be also found on the wiki.

ArchBlackman is a IRC-Chat bot of the #reactos irc channel. He has been coded by Royce3 and DrFred (mbosma) currently runs him. 


If some one use a swear word he'll tell him not to do so and you can also ask him something technical about reactos. :)

Besides this he also knows some commands. The systax for giving him commands "ArchBlackmann: !grovel". (don't forget to address him)

  - !grovel - This is the only command that non ops can do (No Parameter)
  - !kiss <person>
  - !hug <person>
  - !give <someone> <something> 
  - !say <something> - You can tell him to say something on the channel via PrivateMessage


ArchBlackmann know what he should say from some text files. They can be found on the svn-dir. But can also edit them online if you are op using:

  - !add <list> <item>
  - !remove <list> <item>

List that are used directly to create responses are:
  
  - tech - here are the sentences ArchBlackmann says when he finds his name 
  - curse - this are the curses he looks for 
  - cursecop - this the responces to them
  - grovel - this is said when an op does the grovel command
  - nogrovel - this when someone else does it

The remaining lists are not used directly, but by the other lists.

They are: 

  - dev
  - func
  - irql
  - module
  - period
  - status
  - stru
  - type

And they are used like this:
  /me thinks %s is smarter than %dev%