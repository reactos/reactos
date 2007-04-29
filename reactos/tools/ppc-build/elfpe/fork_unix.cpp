#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "fork_execvp.h"

class UnixProcessHolder : public ProcessHolder {
public:
    UnixProcessHolder( const std::vector<std::string> &args ) 
	: read_fd(-1), child_pid(-1), at_end(false), status(-1) {
	std::vector<char *> argvect(args.size()+1);
	for( size_t i = 0; i < args.size(); i++ )
	    argvect[i] = (char *)args[i].c_str();
	argvect[args.size()] = 0;

	int err_fd[2];

	if( pipe( err_fd ) == -1 ) 
	    return;

	fflush(stdout);

	child_pid = fork();

	if( !child_pid ) {
	    dup2( err_fd[1], 2 );
	    close( err_fd[0] );
	    close( err_fd[1] );

	    execvp(args[0].c_str(), &argvect[0] );
	    exit(1);
	} else {
	    close( err_fd[1] );
	    read_fd = err_fd[0];
	}
    }

    ~UnixProcessHolder() {
	close( read_fd );
    }

    std::string ReadStdError() {
	char buf[1024];
	int rl = read( read_fd, buf, sizeof(buf) );
	if( rl < 1 ) { 
	    at_end = true; 
	    waitpid( child_pid, &status, 0 );
	    return "";
	}
	return std::string(buf, rl);
    }

    bool ProcessStarted() const { return child_pid != -1; }
    bool EndOfStream() const { return at_end; }
    int  GetStatus() const { return status; }

private:
    bool at_end;
    int read_fd;
    int child_pid;
    int status;
};

ProcessHolder *fork_execvp( const std::vector<std::string> &args ) {
    return new UnixProcessHolder( args );
}
