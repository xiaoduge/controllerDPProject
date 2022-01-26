#ifndef _WEB_SERVER_H_
#define _WEB_SERVER_H_

#include <string>

int webserver_main(void) ;

bool webserver_query(std::string& uri,std::string& query,std::string &rsp);
bool webserver_post(std::string& uri,std::string &json,std::string &rsp);

#endif /* _WORK_H_ */

