// IRCClient.h
// This file is (C) 2004 Royce Mitchell III
// and released under the BSD & LGPL licenses

#ifndef IRCCLIENT_H
#define IRCCLIENT_H

#include <string>
#include <vector>
#include "SockUtils.h"
#include "ThreadPool.h"

class IRCClient : public suBufferedRecvSocket
{

public:
	IRCClient();

	std::string mychannel;
	static bool GetDebug() { return _debug; }
	static bool SetDebug ( bool debug ) { bool old = _debug; _debug = debug; return old; }

	// connect to server ( record greeting for apop if it exists )
	bool Connect ( const std::string& server, short port = 6667 );

	bool Running() { return _inRun; }

	////////////////////////// IRC Client Protocol commands ///////////////////////

	// first thing you must call... mode can be ""
	// network can be same as name of server used in Connect() above
	bool User ( const std::string& user, const std::string& mode,
		const std::string& network, const std::string& realname );

	// change nick...
	bool Nick ( const std::string& nick );

	// change mode for self...
	bool Mode ( const std::string& mode );

	// set someone's mode in channel ( like oping someone )
	bool Mode ( const std::string& channel, const std::string& mode, const std::string& target );

	// request a list of names of clients in a channel
	bool Names ( const std::string& channel );

	// join a channel...
	bool Join ( const std::string& channel );

	// send message to someone or some channel
	bool PrivMsg ( const std::string& to, const std::string& text );

	// send /me to someone or some channel
	bool Action ( const std::string& to, const std::string& text );

	// leave a channel
	bool Part ( const std::string& channel, const std::string& text );

	// log off
	bool Quit ( const std::string& text );

	////////////////////// callback functions ////////////////////////////

	// OnConnected: you just successfully logged into irc server
	virtual bool OnConnected() = 0;

	virtual bool OnNick ( const std::string& oldNick, const std::string& newNick ) { return true; }

	// OnJoin: someone just successfully joined a channel you are in ( also sent when you successfully join a channel )
	virtual bool OnJoin ( const std::string& user, const std::string& channel ) { return true; }

	// OnPart: someone just left a channel you are in
	virtual bool OnPart ( const std::string& user, const std::string& channel ) { return true; }

	// OnPrivMsg: you just received a private message from a user
	virtual bool OnPrivMsg ( const std::string& from, const std::string& text ) { return true; }

	virtual bool OnPrivAction ( const std::string& from, const std::string& text ) { return true; }

	// OnChannelMsg: you just received a chat line in a channel
	virtual bool OnChannelMsg ( const std::string& channel, const std::string& from,
		const std::string& text ) { return true; }

	// OnChannelAction: you just received a "/me" line in a channel
	virtual bool OnChannelAction ( const std::string& channel, const std::string& from,
		const std::string& text ) { return true; }

	// OnChannelMode: notification of a change of a channel's mode
	virtual bool OnChannelMode ( const std::string& channel, const std::string& mode )
		{ return true; }

	// OnUserModeInChannel: notification of a mode change of a user with respect to a channel.
	// f.ex.: this will be called when someone is oped in a channel.
	virtual bool OnUserModeInChannel ( const std::string& src, const std::string& channel,
		const std::string& mode, const std::string& target ) { return true; }

	// OnMode: you will receive this when you change your own mode, at least...
	virtual bool OnMode ( const std::string& user, const std::string& mode ) { return true; }

	// notification of what users are in a channel ( you may get multiple of these... )
	virtual bool OnChannelUsers ( const std::string& channel, const std::vector<std::string>& users )
		{ return true; }

	// OnKick: if the client has been kicked
	virtual bool OnKick ( void ) { return true; }

	// OnKick: if the client has been kicked
	virtual bool OnBanned ( const std::string& channel ) { return true; }

	// notification that you have received the entire list of users for a channel
	virtual bool OnEndChannelUsers ( const std::string& channel ) { return true; }

	// OnPing - default implementation replies to PING with a valid PONG. required on some systems to
	// log in. Most systems require a response in order to stay connected, used to verify a client hasn't
	// dropped.
	virtual bool OnPing ( const std::string& text );
	
	////////////////////// other functions ////////////////////////////

	// this is for sending data to irc server. it's public in case you need to send some
	// command not supported by this base class...
	bool Send ( const std::string& buf );

	// if launch_thread is true, this function will spawn a thread that will process
	// incoming packets until the socket dies.
	// otherwise ( launch_thread is false ) this function will do that processing
	// in *this* thread, and not return until the socket dies.
	int Run ( bool launch_thread );

	////////////////////// private stuff ////////////////////////////
private:
	bool _Recv ( std::string& buf );

	static int THREADAPI Callback ( IRCClient* irc );

	static bool _debug;
	std::string _nick;
	int _timeout;
	std::string _apop_challenge;

	volatile bool _inRun;

	// disable copy semantics
	IRCClient ( const IRCClient& );
	IRCClient& operator = ( const IRCClient& );
};

#endif//IRCCLIENT_H
