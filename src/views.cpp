#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "views.h"
#include "webadmin.h"
#include "clk_error.h"
#include "logger.h"
#include "mongoose.h"
#include "html_template.h"

extern "C" {
#include "strutils.h"
#include "version.h"
}

using namespace std;

static char * getMethod(struct http_message * message)
{
	char *				pszMethod;

	pszMethod = (char *)malloc(message->method.len + 1);

	if (pszMethod == NULL) {
		throw clk_error("Failed to allocate memory for method...", __FILE__, __LINE__);
	}

	memcpy(pszMethod, message->method.p, message->method.len);
	pszMethod[message->method.len] = 0;

	return pszMethod;
}

static char * getURI(struct http_message * message)
{
	char *				pszURI;

	pszURI = (char *)malloc(message->uri.len + 1);

	if (pszURI == NULL) {
		throw clk_error("Failed to allocate memory for URI...", __FILE__, __LINE__);
	}

	memcpy(pszURI, message->uri.p, message->uri.len);
	pszURI[message->uri.len] = 0;

	return pszURI;
}

static struct mg_serve_http_opts getHTMLOpts()
{
	static struct mg_serve_http_opts opts;

	ConfigManager & cfg = ConfigManager::getInstance();
	WebAdmin & web = WebAdmin::getInstance();

	opts.document_root = web.getHTMLDocRoot();
	opts.enable_directory_listing = "no";
	opts.global_auth_file = cfg.getValue("admin.authfile");

	return opts;
}

static struct mg_serve_http_opts getCSSOpts()
{
	static struct mg_serve_http_opts opts;

	ConfigManager & cfg = ConfigManager::getInstance();
	WebAdmin & web = WebAdmin::getInstance();

	opts.document_root = web.getCSSDocRoot();
	opts.enable_directory_listing = "no";
	opts.global_auth_file = cfg.getValue("admin.authfile");

	return opts;
}

void homeViewHandler(struct mg_connection * connection, int event, void * p)
{
	struct http_message *			message;
	char *							pszMethod;
	char *							pszURI;
	const char *					pszClkVersion = "";
	const char *					pszClkBuildDate = "";

	Logger & log = Logger::getInstance();

	switch (event) {
		case MG_EV_HTTP_REQUEST:
			message = (struct http_message *)p;

			pszMethod = getMethod(message);
			pszURI = getURI(message);

			log.logInfo("Got %s request for '%s'", pszMethod, pszURI);
	
			if (strncmp(pszMethod, "GET", 3) == 0) {
				WebAdmin & web = WebAdmin::getInstance();

				log.logInfo("Serving file '%s'", pszURI);

				if (str_endswith(pszURI, "/") > 0) {
					pszClkVersion = getVersion();
					pszClkBuildDate = getBuildDate();

					log.logInfo("Got Clk version %s [%s]", pszClkVersion, pszClkBuildDate);

					string htmlFileName(web.getHTMLDocRoot());
					htmlFileName.append(pszURI);
					htmlFileName.append("index.html");

					string templateFileName(htmlFileName);
					templateFileName.append(".template");

					log.logDebug("Opening template file [%s]", templateFileName.c_str());

					tmpl::html_template templ(templateFileName);

					templ("clk-version") = pszClkVersion;
					templ("clk-builddate") = pszClkBuildDate;

					templ.Process();

					fstream fs;
					fs.open(htmlFileName, ios::out);
					fs << templ;
					fs.close();
				}

				mg_serve_http(connection, message, getHTMLOpts());
			}

			free(pszMethod);
			free(pszURI);
			break;

		default:
			break;
	}
}

void cssHandler(struct mg_connection * connection, int event, void * p)
{
	struct http_message *			message;
	char *							pszMethod;
	char *							pszURI;

	Logger & log = Logger::getInstance();

	switch (event) {
		case MG_EV_HTTP_REQUEST:
			message = (struct http_message *)p;

			pszMethod = getMethod(message);
			pszURI = getURI(message);

			log.logInfo("Got %s request for '%s'", pszMethod, pszURI);

			if (strncmp(pszMethod, "GET", 3) == 0) {
				log.logInfo("Serving file '%s'", pszURI);

				mg_serve_http(connection, message, getCSSOpts());
			}

			free(pszMethod);
			free(pszURI);

			mg_printf(connection, "HTTP/1.1 200 OK");
			connection->flags |= MG_F_SEND_AND_CLOSE;
			break;

		default:
			break;
	}
}
