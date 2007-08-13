// IRCClient.cpp
// This file is (C) 2004 Royce Mitchell III
// and released under the BSD & LGPL licenses

#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif//_MSC_VER

#include <vector>
#include <sstream>

#include "IRCClient.h"
#include "MD5.h"
#include "cram_md5.h"
#include "trim.h"
#include "chomp.h"
#include "SplitJoin.h"
#include "base64.h"
#include "config.h"

using std::string;
using std::stringstream;
using std::vector;

bool IRCClient::_debug = true;

// see rfc1459 for IRC-Protocoll Reference

IRCClient::IRCClient()
	: _timeout(10*60*1000), _inRun(false)
{
}

bool IRCClient::Connect ( const string& server, short port )
{
	string buf;
	Close();
	Attach ( suTcpSocket() );
	if ( !suConnect ( *this, server.c_str(), port ) )
		return false;
	return true;
}

bool
IRCClient::User ( const string& user, const string& mode,
	const string& network, const string& realname )
{
	string buf;
	buf = "USER " + user + " \"" + mode + "\" \"" + network + "\" :" + realname + "\n";
	return Send ( buf );
}

bool
IRCClient::Nick ( const string& nick )
{
	_nick = nick;
	Send ( "NICK " + _nick + "\n" );
	PrivMsg ("NickServ", "IDENTIFY " + (string)PASS);
	return true;
}

bool
IRCClient::Mode ( const string& mode )
{
	return Send ( "MODE " + _nick + " " + mode + "\n" );
}

bool
IRCClient::Names ( const string& channel )
{
	return Send ( "NAMES " + channel + "\n" );
}

bool
IRCClient::Mode ( const string& channel, const string& mode, const string& target )
{
	return Send ( "MODE " + channel + " " + mode + " " + target + "\n" );
}

bool
IRCClient::Join ( const string& channel )
{
	return Send("JOIN " + channel + "\n");
}

bool
IRCClient::PrivMsg ( const string& to, const string& text )
{
	return Send ( "PRIVMSG " + to + " :" + text + '\n' );
}

bool
IRCClient::Action ( const string& to, const string& text )
{
	return Send ( "PRIVMSG " + to + " :" + (char)1 + "ACTION " + text + (char)1 + '\n' );
}

bool
IRCClient::Part ( const string& channel, const string& text )
{
	return Send ( "PART " + channel + " :" + text + "\n" );
}

bool
IRCClient::Quit ( const string& text )
{
	return Send( "QUIT :" + text + "\n");
}

bool IRCClient::_Recv ( string& buf )
{
	bool b = (recvUntil ( buf, '\n', _timeout ) > 0);
	if ( b && _debug )
	{
		printf ( ">> %s", buf.c_str() );
		if ( buf[buf.length()-1] != '\n' )
			printf ( "\n" );
	}
	return b;
}

bool IRCClient::Send ( const string& buf )
{
	if ( _debug )
	{
		printf ( "<< %s", buf.c_str() );
		if ( buf[buf.length()-1] != '\n' )
			printf ( "\n" );
	}
	return ( buf.length() == (size_t)send ( *this, buf.c_str(), buf.length(), 0 ) );
}

bool IRCClient::OnPing( const string& text )
{
	return Send( "PONG " + text + "\n" );
}


int THREADAPI IRCClient::Callback ( IRCClient* irc )
{
	return irc->Run ( false );
}

int IRCClient::Run ( bool launch_thread )
{
	if ( (SOCKET)*this == INVALID_SOCKET )
		return 0;
	if ( _inRun ) return 1;
	if ( launch_thread )
	{
		ThreadPool::Instance().Launch ( (ThreadPoolFunc*)IRCClient::Callback, this );
		return 1;
	} 
	_inRun = true;
	if ( _debug ) printf ( "IRCClient::Run() - waiting for responses\n" );
	string buf;
	while ( _Recv(buf) )
	{
		if ( !strnicmp ( buf.c_str(), "NOTICE ", 7 ) )
		{
			//printf ( "recv'd NOTICE msg...\n" );
			// TODO...
			//OnAuth ( 
		}
		else if ( !strnicmp ( buf.c_str(), "PING ", 5 ) )
		{
			const char* p = &buf[5]; // point to first char after "PING "
			while ( *p == ':' )      // then read past the colons
				p++;
			const char* p2 = strpbrk ( p, "\r\n" ); // find the end of line
			string text ( p, p2-p );                // and set the text
			OnPing( text );
		}
		else if ( buf[0] == ':' )
		{
			const char* p = &buf[1]; // skip first colon...
			const char* p2 = strpbrk ( p, " !" );
			if ( !p2 )
			{
				printf ( "!!!:OnRecv failure 0: ", buf.c_str() );
				continue;
			}
			string src ( p, p2-p );
			if ( !src.length() )
			{
				printf ( "!!!:OnRecv failure 0.5: %s", buf.c_str() );
				continue;
			}
			p = p2 + 1;
			if ( *p2 == '!' )
			{
				p2 = strchr ( p, ' ' );
				if ( !p2 )
				{
					printf ( "!!!:OnRecv failure 1: %s", buf.c_str() );
					continue;
				}
				//string srchost ( p, p2-p );
				p = p2 + 1;
			}
			p2 = strchr ( p, ' ' );
			if ( !p2 )
			{
				printf ( "!!!:OnRecv failure 2: %s", buf.c_str() );
				continue;
			}
			string cmd ( p, p2-p );
			p = p2 + 1;
			p2 = strpbrk ( p, " :" );
			if ( !p2 )
			{
				printf ( "!!!:OnRecv failure 3: %s", buf.c_str() );
				continue;
			}
			string tgt ( p, p2-p );
			p = p2 + 1;
			p += strspn ( p, " " );
			if ( *p == '=' )
			{
				p++;
				p += strspn ( p, " " );
			}
			if ( *p == ':' )
				p++;
			p2 = strpbrk ( p, "\r\n" );
			if ( !p2 )
			{
				printf ( "!!!:OnRecv failure 4: %s", buf.c_str() );
				continue;
			}
			string text ( p, p2-p );
			strlwr ( &cmd[0] );
			if ( cmd == "privmsg" )
			{
				if ( !tgt.length() )
				{
					printf ( "!!!:OnRecv failure 5 (PRIVMSG w/o target): %s", buf.c_str() );
					continue;
				}
				if ( *p == 1 )
				{
					p++;
					p2 = strchr ( p, ' ' );
					if ( !p2 ) p2 = p + strlen(p);
					cmd = string ( p, p2-p );
					strlwr ( &cmd[0] );
					p = p2 + 1;
					p2 = strchr ( p, 1 );
					if ( !p2 )
					{
						printf ( "!!!:OnRecv failure 6 (no terminating \x01 for initial \x01 found: %s", buf.c_str() );
						continue;
					}
					text = string ( p, p2-p );
					if ( cmd == "action" )
					{
						if ( tgt[0] == '#' )
							OnChannelAction ( tgt, src, text );
						else
							OnPrivAction ( src, text );
					}
					else
					{
						printf ( "!!!:OnRecv failure 7 (unrecognized \x01 command '%s': %s", cmd.c_str(), buf.c_str() );
						continue;
					}
				}
				else
				{
					if ( tgt[0] == '#' )
						OnChannelMsg ( tgt, src, text );
					else
						OnPrivMsg ( src, text );
				}
			}
			else if ( cmd == "mode" )
			{
				// two diff. kinds of mode notifications...
				//printf ( "[MODE] src='%s' cmd='%s' tgt='%s' text='%s'", src.c_str(), cmd.c_str(), tgt.c_str(), text.c_str() );
				//OnMode ( 
				// self mode change:
				// [MODE] src=Nick cmd=mode tgt=Nick  text=+i
				// channel mode change:
				// [MODE] src=Nick cmd=mode tgt=#Channel text=+o Nick
				if ( tgt[0] == '#' )
				{
					p = text.c_str();
					p2 = strchr ( p, ' ' );
					if ( p2 && *p2 )
					{
						string mode ( p, p2-p );
						p = p2 + 1;
						p += strspn ( p, " " );
						OnUserModeInChannel ( src, tgt, mode, trim(p) );
					}
					else
						OnChannelMode ( tgt, text );
				}
				else
					OnMode ( tgt, text );
			}
			else if ( cmd == "join" )
			{
				mychannel = text;
				OnJoin ( src, text );
			}
			else if ( cmd == "part" )
			{
				OnPart ( src, text );
			}
			else if ( cmd == "nick" )
			{
				OnNick ( src, text );
			}
			else if ( cmd == "kick" )
			{
				OnKick ();
			}
			else if ( isdigit(cmd[0]) )
			{
				int i = atoi(cmd.c_str());
				switch ( i )
				{

				case 1: // "Welcome!" - i.e. it's okay to issue commands now...
					OnConnected();
					break;

				case 353: // user list for channel....
					{
						p = text.c_str();
						p2 = strpbrk ( p, " :" );
						if ( !p2 ) continue;
						string channel ( p, p2-p );
						p = strchr ( p2, ':' );
						if ( !p ) continue;
						p++;
						vector<string> users;
						while ( *p )
						{
							p2 = strchr ( p, ' ' );
							if ( !p2 )
								p2 = p + strlen(p);
							users.push_back ( string ( p, p2-p ) );
							p = p2+1;
							p += strspn ( p, " " );
						}
						OnChannelUsers ( channel, users );
					}
					break;

				case 366: // END of user list for channel
					{
						p = text.c_str();
						p2 = strpbrk ( p, " :" );
						if ( !p2 ) continue;
						string channel ( p, p2-p );
						OnEndChannelUsers ( channel );
					}
					break;

				case 474: // You are banned
					{
						p = text.c_str();
						p2 = strpbrk ( p, " :" );
						if ( !p2 ) continue;
						string channel ( p, p2-p );
						OnBanned ( channel ); 
					}
					break;

				case 433: // Nick in Use
					{
						string nick = _nick;
						Nick (nick + "_");

						PrivMsg ("NickServ", "GHOST " + nick + " " + PASS);

						//	HACK HACK HACK 
						Mode ( "+i" );
						Join ( CHANNEL ); // this is because IRC client does not review if his commands were sucessfull

						Sleep ( 1000 );
						Nick ( nick );
					}
					break;

				case 2: //MOTD
				case 376: //MOTD
				case 372:
					break;

				default:
					if ( _debug ) printf ( "unknown command %i: %s", i, buf.c_str() );
					break;
				}
			}
			else
			{
				if ( strstr ( buf.c_str(), "ACTION" ) )
				{
					printf ( "ACTION: " );
					for ( int i = 0; i < buf.size(); i++ )
						printf ( "%c(%xh)", buf[i], (unsigned)(unsigned char)buf[i] );
				}
				else if ( _debug ) printf ( "unrecognized ':' response: %s", buf.c_str() );
			}
		}
		else
		{
			if ( _debug ) printf ( "unrecognized irc msg: %s", buf.c_str() );
		}
		//OnRecv ( buf );
	}
	if ( _debug ) printf ( "IRCClient::Run() - exiting\n" );
	_inRun = false;
	return 0;
}
