#include <iostream>
#include "httplib.h"

int main() {

	using namespace httplib;

	Server app;

	app.get("/test", [](const Request& req, Response& res){
		res.set_content("HelloWorld!", "text/plain");
	});

	app.listen("167.99.144.213", 8000);

	return 0;
}
