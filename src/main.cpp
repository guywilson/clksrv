#include <iostream>
#include <cstring>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <syslog.h>
#include <sys/stat.h>
#include <clk.h>

#include "clk_error.h"
#include "configmgr.h"
#include "webadmin.h"
#include "threads.h"
#include "views.h"
#include "logger.h"

using namespace std;

void printUsage(char * pszAppName)
{
	printf("\n Usage: %s [OPTIONS]\n\n", pszAppName);
	printf("  Options:\n");
	printf("   -h/?             Print this help\n");
	printf("   -version         Print the program version\n");
	printf("   -cfg configfile  Specify the cfg file, default is ./webconfig.cfg\n");
	printf("   -d               Daemonise this application\n");
	printf("\n");
}

void cleanup(void)
{
	/*
	** Kill the threads...
	*/
	ThreadManager & threadMgr = ThreadManager::getInstance();

	threadMgr.killThreads();

	/*
	** Close the logger...
	*/
	Logger & log = Logger::getInstance();
	log.logInfo("Cleaning up and exiting...");
	log.closeLogger();

	closelog();
}

void handleSignal(int sigNum)
{
	Logger & log = Logger::getInstance();

	switch (sigNum) {
		case SIGINT:
			log.logStatus("Detected SIGINT, cleaning up...");
			break;

		case SIGTERM:
			log.logStatus("Detected SIGTERM, cleaning up...");
			break;

		case SIGUSR1:
			/*
			** We're interpreting this as a request to turn on/off debug logging...
			*/
			log.logStatus("Detected SIGUSR1...");

			if (log.isLogLevel(LOG_LEVEL_INFO)) {
				int level = log.getLogLevel();
				level &= ~LOG_LEVEL_INFO;
				log.setLogLevel(level);
			}
			else {
				int level = log.getLogLevel();
				level |= LOG_LEVEL_INFO;
				log.setLogLevel(level);
			}

			if (log.isLogLevel(LOG_LEVEL_DEBUG)) {
				int level = log.getLogLevel();
				level &= ~LOG_LEVEL_DEBUG;
				log.setLogLevel(level);
			}
			else {
				int level = log.getLogLevel();
				level |= LOG_LEVEL_DEBUG;
				log.setLogLevel(level);
			}
			return;

		case SIGUSR2:
			/*
			** We're interpreting this as a request to reload config...
			*/
			log.logStatus("Detected SIGUSR2, reloading config...");

			ConfigManager & cfg = ConfigManager::getInstance();
			cfg.readConfig();

			/*
			** The only thing we can change dynamically (at present)
			** is the logging level...
			*/
			log.setLogLevel(cfg.getValue("log.level"));
			
			return;
	}

	cleanup();

    exit(0);
}

void daemonise()
{
	pid_t			pid;
	pid_t			sid;

	fprintf(stdout, "Starting daemon...\n");
	fflush(stdout);

	do {
		pid = fork();
	}
	while ((pid == -1) && (errno == EAGAIN));

	if (pid < 0) {
		fprintf(stderr, "Forking daemon failed...\n");
		fflush(stderr);
		exit(EXIT_FAILURE);
	}
	if (pid > 0) {
		fprintf(stdout, "Exiting child process...\n");
		fflush(stdout);
		exit(EXIT_SUCCESS);
	}

	sid = setsid();
	
	if(sid < 0) {
		fprintf(stderr, "Failed calling setsid()...\n");
		fflush(stderr);
		exit(EXIT_FAILURE);
	}

	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP, SIG_IGN);    
	
	umask(0);

	if((chdir("/") == -1)) {
		fprintf(stderr, "Failed changing directory\n");
		fflush(stderr);
		exit(EXIT_FAILURE);
	}
	
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
}

int main(int argc, char *argv[])
{
	FILE *			fptr_pid;
	char *			pszAppName;
	char *			pszLogFileName = NULL;
	char *			pszConfigFileName = NULL;
	char			szPidFileName[PATH_MAX];
	int				i;
	bool			isDaemonised = false;
	bool			isDumpConfig = false;
	char			cwd[PATH_MAX];
	int				defaultLoggingLevel = LOG_LEVEL_INFO | LOG_LEVEL_ERROR | LOG_LEVEL_FATAL;
	
	pszAppName = strdup(argv[0]);

	getcwd(cwd, sizeof(cwd));
	
	strcpy(szPidFileName, cwd);
	strcat(szPidFileName, "/clksrv.pid");

	printf("\nRunning %s from %s\n", pszAppName, cwd);

	if (argc > 1) {
		for (i = 1;i < argc;i++) {
			if (argv[i][0] == '-') {
				if (argv[i][1] == 'd') {
					isDaemonised = true;
				}
				else if (strcmp(&argv[i][1], "cfg") == 0) {
					pszConfigFileName = strdup(&argv[++i][0]);
				}
				else if (strcmp(&argv[i][1], "-dump-config") == 0) {
					isDumpConfig = true;
				}
				else if (argv[i][1] == 'h' || argv[i][1] == '?') {
					printUsage(pszAppName);
					return 0;
				}
				else if (strcmp(&argv[i][1], "version") == 0) {
					printf("%s Version: [%s], Build date: [%s]\n\n", pszAppName, getVersion(), getBuildDate());
					return 0;
				}
				else {
					printf("Unknown argument '%s'", &argv[i][0]);
					printUsage(pszAppName);
					return 0;
				}
			}
		}
	}
	else {
		printUsage(pszAppName);
		return -1;
	}

	if (isDaemonised) {
		daemonise();
	}

	fptr_pid = fopen(szPidFileName, "wt");
	
	if (fptr_pid == NULL) {
		fprintf(stderr, "Failed top open PID file\n");
		fflush(stderr);
	}
	else {
		fprintf(fptr_pid, "%d\n", getpid());
		fclose(fptr_pid);
	}
	
	openlog(pszAppName, LOG_PID|LOG_CONS, LOG_DAEMON);
	syslog(LOG_INFO, "Started %s", pszAppName);

	ConfigManager & cfg = ConfigManager::getInstance();
	
	try {
		cfg.initialise(pszConfigFileName);
	}
	catch (clk_error & e) {
		fprintf(stderr, "Could not read config file: %s [%s]\n", pszConfigFileName, e.what());
		fprintf(stderr, "Aborting!\n\n");
		fflush(stderr);
		exit(EXIT_FAILURE);
	}

	if (pszConfigFileName != NULL) {
		free(pszConfigFileName);
	}

	if (isDumpConfig) {
		cfg.dumpConfig();
	}

	Logger & log = Logger::getInstance();

	if (pszLogFileName != NULL) {
		log.initLogger(pszLogFileName, defaultLoggingLevel);
		free(pszLogFileName);
	}
	else {
		const char * filename = cfg.getValue("log.filename");
		const char * level = cfg.getValue("log.level");

		if (strlen(filename) == 0 && strlen(level) == 0) {
			log.initLogger(defaultLoggingLevel);
		}
		else if (strlen(level) == 0) {
			log.initLogger(filename, defaultLoggingLevel);
		}
		else {
			log.initLogger(filename, level);
		}
	}

	/*
	 * Register signal handler for cleanup...
	 */
	if (signal(SIGINT, &handleSignal) == SIG_ERR) {
		log.logFatal("Failed to register signal handler for SIGINT");
		return -1;
	}

	if (signal(SIGTERM, &handleSignal) == SIG_ERR) {
		log.logFatal("Failed to register signal handler for SIGTERM");
		return -1;
	}

	if (signal(SIGUSR1, &handleSignal) == SIG_ERR) {
		log.logFatal("Failed to register signal handler for SIGUSR1");
		return -1;
	}

	if (signal(SIGUSR2, &handleSignal) == SIG_ERR) {
		log.logFatal("Failed to register signal handler for SIGUSR2");
		return -1;
	}

	/*
	** Setup admin server...
	*/
	WebAdmin & web = WebAdmin::getInstance();

	try {
		web.initListener();
	}
	catch (clk_error & e) {
		log.logError("Failed to initialise admin console, console disabled...");
        exit(-1);
	}

    web.registerHandler("/", homeViewHandler);
    web.registerHandler("/css", cssHandler);

	/*
	 * Start threads...
	 */
	ThreadManager & threadMgr = ThreadManager::getInstance();

	threadMgr.startThreads();

	while (1) {
		PosixThread::sleep(PosixThread::seconds, 5L);
	}
	
	cleanup();

    return 0;
}
