// irc_test.cpp

#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif//_MSC_VER

#include <conio.h>
#include <stdio.h>

#include "IRCClient.h"

using std::string;
using std::vector;

// do custom stuff with the IRCClient from your subclass via the provided callbacks...
class MyIRCClient : public IRCClient
{
public:
	// see IRCClient.h for documentation on these callbacks...
	bool OnConnected()
	{
		return true;
	}
	bool OnJoin ( const string& user, const string& channel )
	{
		return true;
	}
	bool OnEndChannelUsers ( const string& channel )
	{
		return true;
	}
	bool OnPrivMsg ( const string& from, const string& text )
	{
		printf ( "<%s> %s\n", from.c_str(), text.c_str() );
		return true;
	}
	bool OnChannelMsg ( const string& channel, const string& from, const string& text )
	{
		printf ( "%s <%s> %s\n", channel.c_str(), from.c_str(), text.c_str() );
		if ( !stricmp ( from.c_str(), "royce3" ) )
			PrivMsg ( channel, text );
		return true;
	}
	bool OnChannelMode ( const string& channel, const string& mode )
	{
		//printf ( "OnChannelMode(%s,%s)\n", channel.c_str(), mode.c_str() );
		return true;
	}
	bool OnUserModeInChannel ( const string& src, const string& channel, const string& user, const string& mode )
	{
		//printf ( "OnUserModeInChannel(%s,%s%s,%s)\n", src.c_str(), channel.c_str(), user.c_str(), mode.c_str() );
		return true;
	}
	bool OnMode ( const string& user, const string& mode )
	{
		//printf ( "OnMode(%s,%s)\n", user.c_str(), mode.c_str() );
		return true;
	}
	bool OnChannelUsers ( const string& channel, const vector<string>& users )
	{
		printf ( "[%s has %i users]: ", channel.c_str(), users.size() );
		for ( int i = 0; i < users.size(); i++ )
		{
			if ( i )
				printf ( ", " );
			printf ( "%s", users[i].c_str() );
		}
		printf ( "\n" );
		return true;
	}
};

int main ( int argc, char** argv )
{
	printf ( "initializing IRCClient debugging\n" );
	IRCClient::SetDebug ( true );
	printf ( "calling suStartup()\n" );
	suStartup();
	printf ( "creating IRCClient object\n" );
	MyIRCClient irc;
	printf ( "connecting to freenode\n" );
	if ( !irc.Connect ( "140.211.166.3" ) ) // irc.freenode.net
	{
		printf ( "couldn't connect to server\n" );
		return -1;
	}
	printf ( "sending user command\n" );
	if ( !irc.User ( "Royce3", "", "irc.freenode.net", "Royce Mitchell III" ) )
	{
		printf ( "USER command failed\n" );
		return -1;
	}
	printf ( "sending nick\n" );
	if ( !irc.Nick ( "Royce3" ) )
	{
		printf ( "NICK command failed\n" );
		return -1;
	}
	printf ( "setting mode\n" );
	if ( !irc.Mode ( "+i" ) )
	{
		printf ( "MODE command failed\n" );
		return -1;
	}
	printf ( "joining #ReactOS\n" );
	if ( !irc.Join ( "#ReactOS" ) )
	{
		printf ( "JOIN command failed\n" );
		return -1;
	}
	printf ( "entering irc client processor\n" );
	irc.Run ( true ); // do the processing in this thread...
	string cmd;
	for ( ;; )
	{
		char c = getch();
		if ( c == '\n' || c == '\r' )
		{

		}
	}
	return 0;
}
