/* 
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS test application
 * FILE:        apps/net/netreg/netreg.cpp
 * PURPOSE:     HTTP Registry Server
 * PROGRAMMERS: Art Yerkes (arty@users.sf.net)
 * REVISIONS:
 *   01-17-2005 arty -- initial
 */
#include <windows.h>
#include <winsock.h>
#include <stdlib.h>
#include <map>
#include <string>
#include <sstream>
#include <iomanip>

using std::hex;
using std::setw;
using std::setfill;
using std::map;
using std::string;
using std::ostringstream;

const char *root_entries[] = { 
  "HKEY_LOCAL_MACHINE",
  "HKEY_CURRENT_USER",
  "HKEY_CLASSES_ROOT",
  "HKEY_CURRENT_CONFIG",
  "HKEY_USERS",
  0
};

const HKEY root_handles[] = {
  HKEY_LOCAL_MACHINE,
  HKEY_CURRENT_USER,
  HKEY_CLASSES_ROOT,
  HKEY_CURRENT_CONFIG,
  HKEY_USERS
};

class RequestHandler {
public:
  RequestHandler( SOCKET s ) : socket( s ), state( NO_REQUEST_YET ) {}
  ~RequestHandler() { closesocket( socket ); }
  void RecvData( string input ) {
    full_input += input;
    if( full_input.find( "\r\n\r\n" ) != string::npos ) {
      // Full request received...
      size_t space_pos = full_input.find( ' ' );
      if( space_pos == string::npos ) { state = SHOULD_DIE; return; }
      string method = full_input.substr( 0, space_pos );
      if( method != "GET" ) { state = SHOULD_DIE; return; }
      space_pos++;
      if( full_input[space_pos] != '/' ) { state = SHOULD_DIE; return; }
      space_pos++;
      string reg_key_and_remainder = 
	full_input.substr( space_pos, full_input.size() - space_pos );
      space_pos = reg_key_and_remainder.find( ' ' );
      if( space_pos == string::npos ) { state = SHOULD_DIE; return; }
      string reg_key_name = reg_key_and_remainder.substr( 0, space_pos );
      process_request( urldec( reg_key_name ) );
      state = REQUEST_RECVD_SENDING_REPLY;
    }
  }
  void OkToSend() {
    int rv = send( socket, 
		   remaining_output.c_str(), 
		   remaining_output.size(), 0 );
    if( rv < 0 ) {
      state = SHOULD_DIE;
      return;
    } else {
      remaining_output = 
	remaining_output.substr( rv, remaining_output.size() - rv );
      if( remaining_output.size() == 0 ) {
	state = SHOULD_DIE;
      }
    }
  }

  SOCKET GetSocket() const { return socket; }

  bool ShouldDie() const {
    return state == SHOULD_DIE;
  }

  bool WantPollout() const {
    return state == REQUEST_RECVD_SENDING_REPLY;
  }
						   

private:
  string urlenc( string in ) {
    ostringstream out;

    for( string::iterator i = in.begin();
	 i != in.end();
	 i++ ) {
      if( isalnum( *i ) || *i == '/' ) 
	out << *i; 
      else {
	char minibuf[10];
	sprintf( minibuf, "%02x", *i );
	out << "%" << minibuf;
      }
    }

    return out.str();
  }

  string urldec( string in ) {
    string out;

    for( string::iterator i = in.begin();
	 i != in.end();
	 i++ ) {
      if( *i == '%' ) {
	char buf[3];
	int res = ' ';

	i++;
	if( i != in.end() ) {
	  buf[0] = *i;
	  i++;
	  if( i != in.end() ) {
	    buf[1] = *i;
	    buf[2] = 0;
	    sscanf( buf, "%x", &res );
	    fprintf( stderr, "Interpreting %c%c as %02x\n", 
		     buf[0], buf[1], 
		     res );
	    out += (char)res;
	  }
	}
      } else out += *i;
    }

    return out;
  }

  string dump_one_line( const char *data, int llen, int len, int addr ) {
    ostringstream out;
    int i;
    
    out << setw( 8 ) << setfill( '0' ) << hex << addr << ": ";
    
    for( i = 0; i < llen; i++ ) {
      if( i < len ) out << setw( 2 ) << setfill( '0' ) << hex << 
		      (data[i] & 0xff) << " ";
      else out << "   ";
    }

    out << " : ";

    for( i = 0; i < llen; i++ ) {
      if( i < len && i < llen && 
	  data[i] >= ' ' && data[i] < 0x7f ) out << data[i]; else out << '.';
    }

    out << "\n";

    return out.str();
  }

  string bindump( const char *data, int len ) {
    const char *end = data + len;
    string out;
    int addr = 0;

    out += "<pre>";
    
    while( data < end ) {
      out += dump_one_line( data, 16, end - data, addr );
      addr += 16;
      data += 16;
    }

    out += "</pre>";

    return out;
  }

  string present_value( DWORD type, const char *data, DWORD len ) {
    switch( type ) {
    default:
      return bindump( data, len );
    }
  }

  void process_valid_request( HKEY open_reg_key, string key_name ) {
    size_t ending_slash;
    string up_level;
    ostringstream text_out;

    DWORD num_sub_keys;
    DWORD max_subkey_len;
    DWORD num_values;
    DWORD max_value_name_len;
    DWORD max_value_len;
    
    char *value_name_buf;
    char *value_buf;
    char *key_name_buf;

    if( RegQueryInfoKey( open_reg_key,
			 NULL,
			 NULL,
			 NULL,
			 &num_sub_keys,
			 &max_subkey_len,
			 NULL,
			 &num_values,
			 &max_value_name_len,
			 &max_value_len,
			 NULL,
			 NULL ) != ERROR_SUCCESS ) {
      process_invalid_request( key_name );
      return;
    }
    
    value_name_buf = new char [max_value_name_len+1];
    value_buf      = new char [max_value_len+1];
    key_name_buf   = new char [max_subkey_len+1];

    ending_slash = key_name.rfind( '/' );
    if( ending_slash != string::npos )
      up_level = key_name.substr( 0, ending_slash );

    text_out << "HTTP/1.0 200 OK\r\n" 
	     << "Content-Type: text/html\r\n"
	     << "\r\n"
	     << "<html><head><title>Registry Key `"
	     << key_name
	     << "'</title></head><body>\r\n"
	     << "<h1>Registry Key `" << key_name << "'</h1>\r\n"
	     << "<a href='/" << urlenc(up_level)
	     << "'>(Up one level)</a><p>\r\n"
	     << "<h2>Subkeys:</h2><table border='1'>\r\n";
    
    DWORD which_index;
    DWORD key_name_size;
 
    for( which_index = 0; which_index < num_sub_keys; which_index++ ) {
      key_name_size = max_subkey_len+1;
      RegEnumKeyEx( open_reg_key,
		    which_index,
		    key_name_buf,
		    &key_name_size,
		    NULL,
		    NULL,
		    NULL,
		    NULL );
      text_out << "<tr><td><a href='/" << urlenc(key_name) << "/"
	       << urlenc(string(key_name_buf,key_name_size)) << "'>"
	       << string(key_name_buf,key_name_size)
	       << "</a></td></tr>\r\n";
    }
    
    text_out << "</table><h2>Values:</h2><table border='1'>\r\n";
    
    DWORD value_name_size;
    DWORD value_data_size;
    DWORD value_type;

    for( which_index = 0; which_index < num_values; which_index++ ) {
      value_name_size = max_value_name_len+1;
      value_data_size = max_value_len+1;

      RegEnumValue( open_reg_key,
		    which_index,
		    value_name_buf,
		    &value_name_size,
		    NULL,
		    &value_type,
		    (BYTE *)value_buf,
		    &value_data_size );

      text_out << "<tr><td><b>" << string(value_name_buf,value_name_size) 
	       << "</b></td><td>"
	       << present_value( value_type, value_buf, value_data_size )
	       << "</td></tr>";
    }
    
    text_out << "</ul></body></html>\r\n";
    
    delete [] key_name_buf;
    delete [] value_name_buf;
    delete [] value_buf;

    remaining_output = text_out.str();
  }

  void process_invalid_request( string reg_key ) {
    ostringstream text_out;
    text_out << "HTTP/1.0 404 Not Found\r\n"
	     << "Content-Type: text/html\r\n"
	     << "\r\n"
	     << "<html><head><title>Can't find registry key `"
	     << reg_key
	     << "'</title></head><body>\r\n"
	     << "<H1>Can't find registry key `"
	     << reg_key
	     << "'</H1>\r\n"
	     << "The registry key doesn't exist in the local registry.\r\n"
	     << "</body></html>\r\n";

    remaining_output = text_out.str();
  }

  void process_root_request() {
    ostringstream text_out;
    int i;

    text_out << "HTTP/1.0 200 OK\r\n"
	     << "Content-Type: text/html\r\n"
	     << "\r\n"
	     << "<html><head><title>Registry Browser</title></head>\r\n"
	     << "<body>\r\n"
	     << "<H1>Registry Browser</H1>"
	     << "You can use this interface to browse the registry."
	     << "You will be presented with one registry key at a time and "
	     << "the decendents.\r\n"
	     << "<h2>Root Level</h2>\r\n"
	     << "Subkeys:<ul>\r\n";

      for( i = 0; root_entries[i]; i++ )
	text_out << "<li>"
		 << "<a href='/" << urlenc(root_entries[i]) 
		 << "'>" << root_entries[i]
		 << "</a></li>\r\n";

      text_out << "</ul></body></html>\r\n";

      remaining_output = text_out.str();
  }

  void process_request( string reg_key ) {
    int i;
    bool is_predefined_key = true;

    if( reg_key == "" ) { process_root_request(); return; }
    HKEY hRegKey = 0;

    // Parse the key name...
    size_t slash = reg_key.find( '/' );
    string reg_initial = "";
    
    if( slash == string::npos ) // A root key...
      reg_initial = reg_key;
    else // Any other key
      reg_initial = reg_key.substr( 0, slash );
    
    fprintf( stderr, "reg_init = %s, reg_key = %s\n", 
	     reg_initial.c_str(),
	     reg_key.c_str() );

    for( i = 0; root_entries[i]; i++ ) 
      if( reg_initial == root_entries[i] ) hRegKey = root_handles[i];

    if( hRegKey != 0 && reg_initial != reg_key ) {
      size_t start_of_reg_path = reg_initial.size() + 1;
      string reg_path = reg_key.substr( start_of_reg_path,
					reg_key.size() - start_of_reg_path );

      string reg_open_path = reg_path;
      do {
	slash = reg_open_path.find( '/' );
	string reg_single_key = reg_open_path;

	if( slash != string::npos ) {
	  reg_single_key = reg_open_path.substr( 0, slash );
	  reg_open_path = reg_open_path.substr( slash+1, 
						reg_open_path.size() );
	}

	HKEY oldKey = hRegKey;

	fprintf( stderr, "Opening %s\n", reg_single_key.c_str() );

	if( RegOpenKey( hRegKey, reg_single_key.c_str(), &hRegKey ) != 
	    ERROR_SUCCESS ) {
	  hRegKey = 0;
	  break;
	} else RegCloseKey( oldKey );

	is_predefined_key = false;
      } while( slash != string::npos );
    }

    if( hRegKey == 0 ) process_invalid_request( reg_key );
    else {
      process_valid_request( hRegKey, reg_key );
      if( !is_predefined_key ) RegCloseKey( hRegKey );
    }
  }

  typedef enum _RHState {
    NO_REQUEST_YET,
    REQUEST_RECVD_SENDING_REPLY,
    SHOULD_DIE
  } RHState;

  string full_input;
  string remaining_output;
  SOCKET socket;
  RHState state;
};

SOCKET make_listening_socket( int port ) {
  struct sockaddr_in sa;
  
  ZeroMemory( &sa, sizeof( sa ) );
  
  sa.sin_family = PF_INET;
  sa.sin_port = ntohs( port );
  
  fprintf( stderr, "Creating the listener\n" );
  SOCKET l = socket( PF_INET, SOCK_STREAM, 0 );
  fprintf( stderr, "Socket %x\n", l );

  if( l == INVALID_SOCKET ) return l;
  if( bind( l, (struct sockaddr *)&sa, sizeof( sa ) ) < 0 ) {
    fprintf( stderr, "Bad response from bind: %d\n", WSAGetLastError() );
    closesocket( l ); return INVALID_SOCKET;
  }
  if( listen( l, 5 ) < 0 ) {
    fprintf( stderr, "Listening: %d\n", WSAGetLastError() );
    closesocket( l );
    return INVALID_SOCKET;
  }

  return l;
}

int main( int argc, char **argv ) {
  WSADATA wdWinsockData;
  map<SOCKET,RequestHandler *> requests;
  fd_set pollin,pollout,pollerr;
  SOCKET listen_socket;
  int i;
  int port_to_listen = 80;
  unsigned int active_fds = 0;

  for( i = 1; i < argc; i++ ) {
    if( string( "-p" ) == argv[i] ) {
      i++;
      if( i < argc ) port_to_listen = atoi( argv[i] );
    }
  }

  WSAStartup( 0x0101, &wdWinsockData );

  listen_socket = make_listening_socket( port_to_listen );
  if( listen_socket == INVALID_SOCKET ) return 1;

  while( true ) {
    FD_ZERO( &pollin );
    FD_ZERO( &pollout );
    FD_ZERO( &pollerr );
    active_fds = listen_socket + 1;

    for( std::map<SOCKET,RequestHandler *>::iterator i = requests.begin();
	 i != requests.end();
	 i++ ) {
      if( i->second->ShouldDie() ) {
	delete i->second;
	requests.erase( i );
	i = requests.begin();
	break;
      }

      FD_SET(i->first,&pollin);
      FD_SET(i->first,&pollerr);

      if( i->first > active_fds ) active_fds = i->first + 1;

      if( i->second->WantPollout() ) FD_SET(i->first,&pollout);
    }

    FD_SET(listen_socket,&pollin);

    active_fds = select( active_fds, &pollin, &pollout, &pollerr, NULL );
    
    if( active_fds > 0 ) {
      if( FD_ISSET(listen_socket,&pollin) ) {
	SOCKET ns = accept( listen_socket, NULL, NULL );
	if( ns != INVALID_SOCKET ) {
	  requests.insert( std::make_pair( ns, new RequestHandler( ns ) ) );
	}
      }

      for( std::map<SOCKET,RequestHandler *>::iterator i = requests.begin();
	   i != requests.end();
	   i++ ) {
	if( FD_ISSET(i->first,&pollin) ) {
	  char inbuf[1024];
	  int rv = recv(i->first,inbuf,1024,0);
	  if( rv < 0 ) {
	    delete i->second;
	    requests.erase( i );
	    i = requests.begin();
	    break;
	  } else i->second->RecvData( string( inbuf, rv ) );
	}
	if( FD_ISSET(i->first,&pollout) ) {
	  i->second->OkToSend();
	}
	if( FD_ISSET(i->first,&pollerr) ) {
	  delete i->second;
	  requests.erase( i );
	  i = requests.begin();
	  break;
	}
      }
    }
  }

  WSACleanup();
}
