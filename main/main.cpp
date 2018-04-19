#include <iostream>
#include "httplib.h"
#include <sqlite3.h>

#include "database.h"

int main() {

	using namespace httplib;

	Server app;

	//open a connection to our database
	Database* db = new Database("/Users/connorwiniarczyk/Desktop/data.db");

	app.post("/data/execute", [&](const auto& req, auto& res){
		Query* query = new Query(req.body);
		*db << query;

		res.set_content(query -> result, "text/plain");
	});

	app.post("/data/add_user", [&](const auto& req, auto& res){
		res.set_content("Post!", "text/plain");
	});

	app.get("/test", [](const Request& req, Response& res){
		res.set_content("HelloWorld!", "text/plain");
	});

	app.listen("0.0.0.0", 8000);
	
	
	

	return 0;
}
