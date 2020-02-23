#ifndef _INCL_VIEWS
#define _INCL_VIEWS

/*
** View handlers
*/
void homeViewHandler(struct mg_connection * connection, int event, void * p);
void browseViewHandler(struct mg_connection * connection, int event, void * p);
void uploadCmdHandler(struct mg_connection * connection, int event, void * p);
void cssHandler(struct mg_connection * connection, int event, void * p);

#endif
