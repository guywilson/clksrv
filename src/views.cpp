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

const char * getMethod(struct http_message * message)
{
	static char *		pszMethod;

	pszMethod = (char *)malloc(message->method.len + 1);

	if (pszMethod == NULL) {
		throw clk_error("Failled to allocate memory for method", __FILE__, __LINE__);
	}

	memcpy(pszMethod, message->method.p, message->method.len);
	pszMethod[message->method.len] = 0;

	return pszMethod;
}

const char * getURI(struct http_message * message)
{
	static char *		pszURI;

	pszURI = (char *)malloc(message->uri.len + 1);

	if (pszURI == NULL) {
		throw clk_error("Failled to allocate memory for URI", __FILE__, __LINE__);
	}

	memcpy(pszURI, message->uri.p, message->uri.len);
	pszURI[message->uri.len] = 0;

	return pszURI;
}

struct mg_serve_http_opts getHTMLOpts()
{
	static struct mg_serve_http_opts opts;

	WebAdmin & web = WebAdmin::getInstance();

	opts.document_root = web.getHTMLDocRoot();
	opts.enable_directory_listing = "no";
	opts.global_auth_file = NULL;

	return opts;
}

struct mg_serve_http_opts getCSSOpts()
{
	struct mg_serve_http_opts opts;

	WebAdmin & web = WebAdmin::getInstance();

	opts.document_root = web.getCSSDocRoot();
	opts.enable_directory_listing = "no";
	opts.global_auth_file = NULL;

	return opts;
}

void homeViewHandler(struct mg_connection * connection, int event, void * p)
{
	struct http_message *			message;
	struct mg_serve_http_opts 		opts;
	const char *					pszMethod;
	const char *					pszURI;

	Logger & log = Logger::getInstance();

	memset(&opts, 0, sizeof(opts));

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
					string clkVersion = getVersion();
					string clkBuildDate = getBuildDate();

					log.logInfo("Got Clk version %s [%s]", clkVersion.c_str(), clkBuildDate.c_str());

					string htmlFileName(web.getHTMLDocRoot());
					htmlFileName.append(pszURI);
					htmlFileName.append("index.html");

					string templateFileName(htmlFileName);
					templateFileName.append(".template");

					log.logDebug("Opening template file [%s]", templateFileName.c_str());

					tmpl::html_template templ(templateFileName);

					templ("clk-version") = clkVersion;
					templ("clk-builddate") = clkBuildDate;

					templ.Process();

					log.logDebug("Processed template file...");

					fstream fs;
					fs.open(htmlFileName, ios::out);
					fs << templ;
					fs.close();

					log.logDebug("Written html file %s", htmlFileName.c_str());
				}

				opts.document_root = web.getHTMLDocRoot();
				opts.enable_directory_listing = "no";
				opts.global_auth_file = NULL;
				opts.ip_acl = NULL;

				mg_serve_http(connection, message, opts);
			}

			break;

		default:
			break;
	}
}

void cssHandler(struct mg_connection * connection, int event, void * p)
{
	struct http_message *			message;
	struct mg_serve_http_opts 		opts;
	const char *					pszMethod;
	const char *					pszURI;

	Logger & log = Logger::getInstance();

	memset(&opts, 0, sizeof(opts));
	
	switch (event) {
		case MG_EV_HTTP_REQUEST:
			message = (struct http_message *)p;

			pszMethod = getMethod(message);
			pszURI = getURI(message);

			log.logInfo("Got %s request for '%s'", pszMethod, pszURI);

			if (strncmp(pszMethod, "GET", 3) == 0) {
				log.logInfo("Serving file '%s'", pszURI);

				WebAdmin & web = WebAdmin::getInstance();

				opts.document_root = web.getCSSDocRoot();
				opts.enable_directory_listing = "no";
				opts.global_auth_file = NULL;
				opts.ip_acl = NULL;

				mg_serve_http(connection, message, opts);
			}

			break;

		default:
			break;
	}
}
