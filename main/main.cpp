#include <iostream>
#include "httplib.h"
#include <sqlite3.h>
#include "json.hpp"

#include "database.h"

int main() {

	using json = nlohmann::json;

	using namespace httplib;

	Server app;

	//open a connection to our database
	Database* db = new Database("/Users/connorwiniarczyk/server/data.db");

	app.post("/data/execute", [&](const auto& req, auto& res){
		Query* query = new Query(req.body);
		*db << query;

		res.set_content(query -> result, "text/plain");
	});

	app.post("/data/add_user", [&](const auto& req, auto& res){
		res.set_content("Post!", "text/plain");
	});

	app.post("/user/login", [&](const auto& req, auto& res){

		auto data = json::parse(req.body);

		// extract username and password hash
		std::string username = data["username"];
		std::string password_hash = data["password_hash"];

		// get user with this username from database
		Query* query = new Query("SELECT password_hash FROM users WHERE username = \"" + username + "\";");
		*db << query;

		// res.set_content(query -> result, "text/plain");

		// if the passwords match, let the user know they have logged in successfully
		if(query -> result != "" && query -> result == password_hash) {
			res.set_content("SUCCESS", "text/plain");
		} else {
			res.set_content("FAILURE", "text/plain");
		}
	});

	app.get("/test", [](const Request& req, Response& res){
		res.set_content("HelloWorld!", "text/plain");
	});

	app.listen("0.0.0.0", 8000);
	
	
	

	return 0;
}
