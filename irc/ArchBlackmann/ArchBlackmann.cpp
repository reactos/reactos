// irc_test.cpp

#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif//_MSC_VER

#include <time.h>
#include <stdio.h>

#include "File.h"
#include "ssprintf.h"

#include "IRCClient.h"

using std::string;
using std::vector;

const char* ArchBlackmann = "ArchBlackmann";

vector<string> tech, module, dev, stru, period;

void ImportList ( vector<string>& list, const char* filename )
{
	File f ( filename, "r" );
	string line;
	while ( f.next_line ( line, true ) )
		list.push_back ( line );
}

const char* ListRand ( const vector<string>& list )
{
	return list[rand()%list.size()].c_str();
}

string TechReply()
{
	string t = ListRand(tech);
	string out;
	const char* p = t.c_str();
	while ( *p )
	{
		if ( *p == '%' )
		{
			if ( !strnicmp ( p, "%dev%", 5 ) )
			{
				out += ListRand(dev);
				p += 5;
			}
			else if ( !strnicmp ( p, "%period%", 8 ) )
			{
				out += ListRand(period);
				p += 8;
			}
			else if ( !strnicmp ( p, "%module%", 8 ) )
			{
				out += ListRand(module);
				p += 8;
			}
			else if ( !strnicmp ( p, "%stru%", 6 ) )
			{
				out += ListRand(stru);
				p += 6;
			}
			else
				out += *p++;
		}
		const char* p2 = strchr ( p, '%' );
		if ( !p2 )
			p2 = p + strlen(p);
		if ( p2 > p )
		{
			out += string ( p, p2-p );
			p = p2;
		}
	}
	return out;
}

// do custom stuff with the IRCClient from your subclass via the provided callbacks...
class MyIRCClient : public IRCClient
{
	File flog;
public:
	MyIRCClient()
	{
		flog.open ( "arch.log", "w+" );
	}
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
		flog.printf ( "<%s> %s\n", from.c_str(), text.c_str() );
		return PrivMsg ( from, "hey, your tongue doesn't belong there!" );
	}
	bool OnChannelMsg ( const string& channel, const string& from, const string& text )
	{
		printf ( "%s <%s> %s\n", channel.c_str(), from.c_str(), text.c_str() );
		flog.printf ( "%s <%s> %s\n", channel.c_str(), from.c_str(), text.c_str() );
		string text2(text);
		strlwr ( &text2[0] );
		if ( !strnicmp ( text2.c_str(), ArchBlackmann, strlen(ArchBlackmann) ) )
		{
			string reply = ssprintf("%s: %s", from.c_str(), TechReply().c_str());
			flog.printf ( "TECH-REPLY: %s\n", reply.c_str() );
			return PrivMsg ( channel, reply );
		}
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
	srand ( time(NULL) );
	ImportList ( tech, "tech.txt" );
	ImportList ( stru, "stru.txt" );
	ImportList ( dev, "dev.txt" );
	ImportList ( period, "period.txt" );
	ImportList ( module, "module.txt" );

	printf ( "initializing IRCClient debugging\n" );
	IRCClient::SetDebug ( true );
	printf ( "calling suStartup()\n" );
	suStartup();
	printf ( "creating IRCClient object\n" );
	MyIRCClient irc;
	printf ( "connecting to freenode\n" );
	if ( !irc.Connect ( "212.204.214.114" ) ) // irc.freenode.net
	{
		printf ( "couldn't connect to server\n" );
		return -1;
	}
	printf ( "sending user command\n" );
	if ( !irc.User ( "ArchBlackmann", "", "irc.freenode.net", "Arch Blackmann" ) )
	{
		printf ( "USER command failed\n" );
		return -1;
	}
	printf ( "sending nick\n" );
	if ( !irc.Nick ( "ArchBlackmann" ) )
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
	irc.Run ( false ); // do the processing in this thread...
	return 0;
}
