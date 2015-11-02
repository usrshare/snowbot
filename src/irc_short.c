// vim: cin:sts=4:sw=4 
#include "irc_short.h"
#include "http.h"
#include <string.h>
#include <stdio.h>

const char* irc_shorten(const char* url) {

	char vgdurl[strlen(url) + 128];

	sprintf(vgdurl,"http://v.gd/create.php?format=simple&url=%s",url);
	
	return make_http_request(vgdurl,0);
}
