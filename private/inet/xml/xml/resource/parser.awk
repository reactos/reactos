BEGIN { found=false; message=""; }

/MessageText:/ { message= ""; continue; }
/#define MSG_/ { printf("<tr><td><font size=-1>XML%s</font><TD><font size=-1>%s</font><TD>%s\n",substr($2,4),substr($3,0,length($3)-1),message) }
{ message = message  substr($0,4); }