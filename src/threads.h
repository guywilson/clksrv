#include "posixthread.h"

#ifndef _INCL_THREADS
#define _INCL_THREADS

class WebListenThread : public PosixThread
{
public:
    WebListenThread() : PosixThread(true) {}

    void * run();
};

class ThreadManager
{
public:
    static ThreadManager &  getInstance() {
        static ThreadManager instance;
        return instance;
    }

private:
    ThreadManager() {}

    WebListenThread *     pWebListenThread = NULL;

public:
    void                    startThreads();
    void                    killThreads();
};

#endif
