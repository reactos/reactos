#ifndef FORK_EXECVP_H
#define FORK_EXECVP_H

#include <string>
#include <vector>

class ProcessHolder {
public:
    virtual ~ProcessHolder() { }
    virtual std::string ReadStdError() = 0;
    virtual bool ProcessStarted() const = 0;
    virtual bool EndOfStream() const = 0;
    virtual int  GetStatus() const = 0;
};

class Process {
public:
    Process( ProcessHolder *h ) : holder(h) { }
    ~Process() { delete holder; }
    ProcessHolder *operator -> () const { return holder; }
    operator ProcessHolder *() const { return (holder && holder->ProcessStarted()) ? holder : NULL; }
private:
    ProcessHolder *holder;
};

ProcessHolder *fork_execvp( const std::vector<std::string> &args );

#endif//FORK_EXECVP_H
