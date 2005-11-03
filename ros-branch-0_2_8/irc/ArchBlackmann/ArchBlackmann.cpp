// irc_test.cpp

#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif//_MSC_VER

#include <time.h>
#include <stdio.h>

#include "File.h"
#include "ssprintf.h"
#include "trim.h"

#include "IRCClient.h"
#include "config.h"

using std::string;
using std::vector;

//vector<string> tech, module, dev, stru, period, status, type, func, irql, curse, cursecop;

class List
{
public:
	string name;
	bool macro;
	std::vector<std::string> list;
	string tag;
	int last;
	List() { last = -1; }
	List ( const char* _name, bool _macro ) : name(_name), macro(_macro)
	{
		tag = ssprintf("%%%s%%",_name);
		last = -1;
	}
};

vector<List> lists;
vector<string> ops;

void ImportList ( const char* listname, bool macro )
{
	lists.push_back ( List ( listname, macro ) );
	List& list = lists.back();
	File f ( ssprintf("%s.txt",listname).c_str(), "r" );
	string line;
	while ( f.next_line ( line, true ) )
		list.list.push_back ( line );
}

const char* ListRand ( List& list )
{
	vector<string>& l = list.list;
	if ( !l.size() )
	{
		static string nothing;
		nothing = ssprintf ( "<list '%s' empty>", list.name.c_str() );
		return nothing.c_str();
	}
	else if ( l.size() == 1 )
		return l[0].c_str();
	int sel = list.last;
	while ( sel == list.last )
		sel = rand()%l.size();
	list.last = sel;
	return l[sel].c_str();
}

const char* ListRand ( int i )
{
	return ListRand ( lists[i] );
}

int GetListIndex ( const char* listname )
{
	for ( int i = 0; i < lists.size(); i++ )
	{
		if ( !stricmp ( lists[i].name.c_str(), listname ) )
			return i;
	}
	return -1;
}

List& GetList ( const char* listname )
{
	return lists[GetListIndex(listname)];
}

const char* ListRand ( const char* list )
{
	int i = GetListIndex ( list );
	if ( i < 0 )
		return NULL;
	return ListRand(i);
}

string TaggedReply ( const char* listname )
{
	string t = ListRand(listname);
	string out;
	const char* p = t.c_str();
	while ( *p )
	{
		if ( *p == '%' )
		{
			bool found = false;
			for ( int i = 0; i < lists.size() && !found; i++ )
			{
				if ( lists[i].macro && !strnicmp ( p, lists[i].tag.c_str(), lists[i].tag.size() ) )
				{
					out += ListRand(i);
					p += lists[i].tag.size();
					found = true;
				}
			}
			if ( !found )
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

string gobble ( string& s, const char* delim )
{
	const char* p = s.c_str();
	p += strspn ( p, delim );
	const char* p2 = strpbrk ( p, delim );
	if ( !p2 ) p2 = p + strlen(p);
	string out ( p, p2-p );
	p2 += strspn ( p2, delim );
	s = string ( p2 );
	return out;
}

bool isop ( const string& who )
{
	for ( int i = 0; i < ops.size(); i++ )
	{
		if ( ops[i] == who )
			return true;
	}
	return false;
}


// do custom stuff with the IRCClient from your subclass via the provided callbacks...
class MyIRCClient : public IRCClient
{
	File flog;
	clock_t brake_silence;

	// wait another 30 mins to brake the silence
	#define NOIDLE brake_silence = clock () + 30 * CLK_TCK * 60

	void CheckIdle ( void )
	{
		while (true) // _inRun
		{
			while (clock() < brake_silence)
				Sleep(10000);

			string out = TaggedReply("idle");

			if ( !strnicmp ( out.c_str(), "/me ", 4 ) )
				Action ( CHANNEL, &out[4] );
			else
				PrivMsg ( CHANNEL, out );

			NOIDLE;
		}
	}

	static void THREADAPI CallMe ( MyIRCClient* irc )
	{
		irc->CheckIdle();
	}

public:

	MyIRCClient()
	{
		NOIDLE;
		ThreadPool::Instance().Launch ( (ThreadPoolFunc*)MyIRCClient::CallMe, this );
		flog.open ( "arch.log", "r+" );
	}
	// see IRCClient.h for documentation on these callbacks...
	bool OnConnected()
	{
		return true;
	}
	bool OnJoin ( const string& user, const string& channel )
	{
		//printf ( "user '%s' joined channel '%s'\n", user.c_str(), channel.c_str() );
		return true;
	}
	bool OnPart ( const std::string& user, const std::string& channel )
	{
		std::vector<std::string>::iterator it = ops.begin();
		for ( ; it != ops.end(); it++ )
		{
			if ( *it == user )
			{
				printf ( "remove '%s' to ops list\n", user.c_str() );
				ops.erase ( it );
			}
		}
		return true;
	}
	bool OnNick ( const std::string& oldNick, const std::string& newNick )
	{
		for ( int i = 0; i < ops.size(); i++ )
		{
			if ( ops[i] == oldNick )
			{
				printf ( "op '%s' changed nick to '%s'\n", oldNick.c_str(), newNick.c_str() );
				ops[i] = newNick;
				return true;
			}
		}
		return true;
	}
	bool OnEndChannelUsers ( const string& channel )
	{
		return true;
	}
	bool OnPrivMsg ( const string& from, const string& text )
	{
		//flog.flush();
		printf ( "<%s> %s\n", from.c_str(), text.c_str() );
		flog.printf ( "<%s> %s\n", from.c_str(), text.c_str() );

		if ( !isop(from) )
			return PrivMsg ( from, "hey, your tongue doesn't belong there!" );

		else if ( strnicmp ( text.c_str(), "!say ", 5 ) )
			return PrivMsg ( from, "Talk to me on normal Chanel" );

		string say = trim(&text[5]);

		if ( !strnicmp ( say.c_str(), "/me ", 4 ) )
			return Action ( CHANNEL, trim(&say[4]) );
		else
			return PrivMsg ( CHANNEL, trim(say) );
	}
	bool OnChannelMsg ( const string& channel, const string& from, const string& text )
	{
		fflush ( flog );
		printf ( "%s <%s> %s\n", channel.c_str(), from.c_str(), text.c_str() );
		flog.printf ( "%s <%s> %s\n", channel.c_str(), from.c_str(), text.c_str() );
		NOIDLE; // add 30 mins till idle

		bool found_name = false;
		string text2 ( text );
		strlwr ( &text2[0] );

		if ( !strnicmp ( text.c_str(), BOTNAME, strlen(BOTNAME) ) )
			found_name = true;

		string s ( text );

		if ( found_name )
			gobble ( s, " \t" ); // remove bot name

		// command
		if ( s[0] == '!' )
		{
			bool from_op = isop(from);
			string cmd = gobble ( s, " \t" );

			// from all		
			if ( false && cmd == "!svn" && from == "TechBot" ) // || cmd == "!help" && !TechBotOnline
			{
					PrivMsg ( channel, "For my help try !what." );
			}

			// from normel user
			else if ( !from_op )
			{
				if ( cmd == "!grovel" )
				{
					string out = ssprintf(TaggedReply("nogrovel").c_str(),from.c_str());
					if ( !strnicmp ( out.c_str(), "/me ", 4 ) )
						return Action ( channel, &out[4] );
					else
						return PrivMsg ( channel, out );
				}
				
				else if ( cmd == "!what" )
				{
					return PrivMsg ( channel, ssprintf("For you, %s, I only support the \"!grovel\" command.", from.c_str()).c_str() );
				}
				
				else if ( cmd == "!grovel" || cmd == "!kiss" || cmd == "!hug" 
				       || cmd == "!give" || cmd == "!what" || cmd == "!add" || cmd == "!remove" )
				{
					PrivMsg ( channel, ssprintf("%s: I only take commands from ops",from.c_str()) );
				}
			}
			
			// from op
			else if ( cmd == "!grovel" )
			{
				string out = ssprintf(TaggedReply("grovel").c_str(),from.c_str());
				if ( !strnicmp ( out.c_str(), "/me ", 4 ) )
					return Action ( channel, &out[4] );
				else
					return PrivMsg ( channel, out );
			}
			else if ( cmd == "!kiss" )
			{
				if ( s.size() )
					return Action ( channel, ssprintf("kisses %s",s.c_str()) );
				else
					return PrivMsg ( channel, ssprintf("%s: huh?",from.c_str()) );
			}
			else if ( cmd == "!hug" )
			{
				if ( s.size() )
					return Action ( channel, ssprintf("hugs %s",s.c_str()) );
				else
					return PrivMsg ( channel, ssprintf("%s: huh?",from.c_str()) );
			}	
			else if ( cmd == "!give" )
			{
				string who = gobble(s," \t");
				if ( who.size() && s.size() )
					return Action ( channel, ssprintf("gives %s a %s",who.c_str(),s.c_str()) );
				else
					return PrivMsg ( channel, ssprintf("%s: huh?",from.c_str()) );
			}
			else if ( cmd == "!what" )
			{		
				PrivMsg ( channel, "For ops I support the following commands:" );
				PrivMsg ( channel, "!grovel" );
				PrivMsg ( channel, "!kiss" );
				PrivMsg ( channel, "!hug" );
				PrivMsg ( channel, "!give" );
				PrivMsg ( channel, "!say (the input is a private message)" );
				PrivMsg ( channel, "!add" );
				PrivMsg ( channel, "!remove" );
				PrivMsg ( channel, " - for more info see wiki" );
			}
			else if ( cmd == "!add" )
			{
				string listname = gobble ( s, " \t" );
				int i = GetListIndex ( listname.c_str() );
				if ( i == -1 )
					return PrivMsg ( channel, ssprintf("%s: I don't have a list named '%s'",from.c_str(),listname.c_str()) );
				List& list = lists[i];
				if ( s[0] == '\"' || s[0] == '\'' )
				{
					char delim = s[0];
					const char* p = &s[1];
					const char* p2 = strchr ( p, delim );
					if ( !p2 )
						return PrivMsg ( channel, ssprintf("%s: Couldn't add, unmatched quotes",from.c_str()) );
					s = string ( p, p2-p );
				}
				for ( i = 0; i < list.list.size(); i++ )
				{
					if ( list.list[i] == s )
						return PrivMsg ( channel, ssprintf("%s: entry already exists in list '%s'",from.c_str(),listname.c_str()) );
				}
				if ( !stricmp ( listname.c_str(), "curse" ) )
					strlwr ( &s[0] );
				list.list.push_back ( s );
				{
					File f ( ssprintf("%s.txt",list.name.c_str()), "w" );
					for ( i = 0; i < list.list.size(); i++ )
						f.printf ( "%s\n", list.list[i].c_str() );
				}
				return PrivMsg ( channel, ssprintf("%s: entry added to list '%s'",from.c_str(),listname.c_str()) );
			}
			else if ( cmd == "!remove" )
			{
				string listname = gobble ( s, " \t" );
				int i = GetListIndex ( listname.c_str() );
				if ( i == -1 )
					return PrivMsg ( channel, ssprintf("%s: I don't have a list named '%s'",from.c_str(),listname.c_str()) );
				List& list = lists[i];
				if ( s[0] == '\"' || s[0] == '\'' )
				{
					char delim = s[0];
					const char* p = &s[1];
					const char* p2 = strchr ( p, delim );
					if ( !p2 )
						return PrivMsg ( channel, ssprintf("%s: Couldn't add, unmatched quotes",from.c_str()) );
					s = string ( p, p2-p );
				}
				std::vector<std::string>::iterator it = list.list.begin();
				for ( ; it != list.list.end(); it++ )
				{
					if ( *it == s )
					{
						list.list.erase ( it );
						{
							File f ( ssprintf("%s.txt",list.name.c_str()), "w" );
							it = list.list.begin();
							for ( ; it < list.list.end(); it++ )
								f.printf ( "%s\n", it->c_str() );
						}
						return PrivMsg ( channel, ssprintf("%s: entry removed from list '%s'",from.c_str(),listname.c_str()) );
					}
				}
				return PrivMsg ( channel, ssprintf("%s: entry doesn't exist in list '%s'",from.c_str(),listname.c_str()) );
			}
			else
			{
				if (found_name)
					return PrivMsg ( channel, ssprintf("%s: huh?",from.c_str()) );
			}
		
		} // if (command)

		bool found_curse = false;
		static vector<string>& curse = GetList("curse").list;
		text2 = " " + text2 + " ";

		for ( int i = 0; i < curse.size() && !found_curse; i++ )
		{
			if ( strstr ( text2.c_str(), curse[i].c_str() ) )
				found_curse = true;
		}
		if ( found_curse )
		{
			static List& cursecop = GetList("cursecop");
			return PrivMsg ( channel, ssprintf("%s: %s", from.c_str(), ListRand(cursecop)) );
		}

		string botname (BOTNAME);
		strlwr ( &botname[0] );
		//botname = " " + botname + " ";

		if ( strstr(text2.c_str(), botname.c_str()) || strstr(text2.c_str(), " arch ") || found_name )
		{
			string out = ssprintf("%s: %s", from.c_str(), TaggedReply("tech").c_str());
			flog.printf ( "TECH-REPLY: %s\n", out.c_str() );
			if ( !strnicmp ( out.c_str(), "/me ", 4 ) )
				return Action ( channel, &out[4] );
			else
				return PrivMsg ( channel, out );
		}
		return true;

	} // On Chanel Message

	bool OnChannelMode ( const string& channel, const string& mode )
	{
		//printf ( "OnChannelMode(%s,%s)\n", channel.c_str(), mode.c_str() );
		return true;
	}
	bool OnUserModeInChannel ( const string& src, const string& channel, const string& mode, const string& target )
	{
		printf ( "OnUserModeInChannel(%s,%s,%s,%s)\n", src.c_str(), channel.c_str(), mode.c_str(), target.c_str() );
		const char* p = mode.c_str();
		if ( !p )
			return true;
		while ( *p )
		{
			switch ( *p++ )
			{
			case '+':
				while ( *p != 0 && *p != ' ' )
				{
					if ( *p == 'o' )
					{
						printf ( "adding '%s' to ops list\n", target.c_str() );
						ops.push_back ( target );
					}
					break;
				}
				break;
			case '-':
				while ( *p != 0 && *p != ' ' )
				{
					if ( *p == 'o' )
					{
						std::vector<std::string>::iterator it = ops.begin();
						for ( ; it != ops.end(); it++ )
						{
							if ( *it == target )
							{
								printf ( "remove '%s' to ops list\n", target.c_str() );
								ops.erase ( it );
							}
						}
						break;
					}
				}
			}
		}
		return true;
	}
	bool OnMode ( const string& user, const string& mode )
	{
		//printf ( "OnMode(%s,%s)\n", user.c_str(), mode.c_str() );
		return true;
	}
	bool OnChannelUsers ( const string& channel, const vector<string>& users )
	{
		//printf ( "[%s has %i users]: ", channel.c_str(), users.size() );
		for ( int i = 0; i < users.size(); i++ )
		{
			if ( users[i][0] == '@' )
				ops.push_back ( &users[i][1] );
			/*if ( i )
				printf ( ", " );
			printf ( "%s", users[i].c_str() );*/
		}
		//printf ( "\n" );
		return true;
	}
	bool OnKick ( void )
	{
		Join(CHANNEL);
		return true;
	}
	bool OnBanned ( const std::string& channel )
	{
		Sleep(10000);
		return Join(CHANNEL);
	}
};

int main ( int argc, char** argv )
{
	srand ( time(NULL) );

	ImportList ( "dev", true );
	ImportList ( "func", true );
	ImportList ( "dev", true );
	ImportList ( "func", true );
	ImportList ( "irql", true );
	ImportList ( "module", true );
	ImportList ( "period", true );
	ImportList ( "status", true );
	ImportList ( "stru", true );
	ImportList ( "type", true );

	ImportList ( "tech", false );
	ImportList ( "curse", false );
	ImportList ( "cursecop", false );
	ImportList ( "grovel", false );
	ImportList ( "nogrovel", false );
	ImportList ( "idle", false );

#ifdef _DEBUG
	printf ( "initializing IRCClient debugging\n" );
	IRCClient::SetDebug ( true );
#endif//_DEBUG

	while (true)
	{
		printf ( "calling suStartup()\n" );
		suStartup();
		printf ( "creating IRCClient object\n" );
		MyIRCClient irc;
		printf ( "connecting to freenode\n" );

		if ( !irc.Connect ( SERVER ) ) // irc.freenode.net
		{
			printf ( "couldn't connect to server\n" );
			Sleep(10000);
			continue;
		}
		printf ( "sending user command\n" );
		if ( !irc.User ( BOTNAME, "reactos.com", SERVER, "ArchBlackmann" ) )
		{
			printf ( "USER command failed, retying ...\n" );
			Sleep(10000);
			continue;
		}
		printf ( "sending nick\n" );
		if ( !irc.Nick ( BOTNAME ) )
		{
			printf ( "NICK command failed, retying ...\n" );
			Sleep(10000);
			continue;
		}
		printf ( "setting mode\n" );
		if ( !irc.Mode ( MODE ) )
		{
			printf ( "MODE command failed, retying ...\n" );
			Sleep(10000);
			continue;
		}
		printf ( "joining channel\n" );
		if ( !irc.Join ( CHANNEL ) )
		{
			printf ( "JOIN command failed, retying ...\n" );
			Sleep(10000);
			continue;
		}

		printf ( "entering irc client processor\n" );
		irc.Run ( false ); // do the processing in this thread...
	}

	return 0;
}
