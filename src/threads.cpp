#include <stdint.h>
#include <unistd.h>

#include "configmgr.h"
#include "logger.h"
#include "clk_error.h"
#include "threads.h"
#include "webadmin.h"

extern "C" {
#include "strutils.h"
}

using namespace std;

void ThreadManager::startThreads()
{
	Logger & log = Logger::getInstance();

	this->pWebListenThread = new WebListenThread();
	if (this->pWebListenThread->start()) {
		log.logStatus("Started WebListenThread successfully");
	}
	else {
		throw clk_error("Failed to start AdminListenThread", __FILE__, __LINE__);
	}
}

void ThreadManager::killThreads()
{
	if (this->pWebListenThread != NULL) {
		this->pWebListenThread->stop();
	}
}

void * WebListenThread::run()
{
	WebAdmin & web = WebAdmin::getInstance();
	Logger & log = Logger::getInstance();

	web.listen();

	log.logError("web.listen returned...");

	return NULL;
}
