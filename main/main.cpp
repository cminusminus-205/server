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

		json j_vec(query -> result);

		res.set_content(j_vec.dump(4), "text/plain");
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
		if(query -> result.size() > 0 && query -> result.front()["password_hash"] == password_hash) {
			res.set_content("SUCCESS", "text/plain");
		} else {
			res.set_content("FAILURE", "text/plain");
		}
	});

	app.post("/user/signup", [&](const auto& req, auto& res){
		auto data = json::parse(req.body);

		std::string username = data["username"];
		std::string email = data["email"];
		std::string password_hash = data["password_hash"];

		//Check to see if the user already exists
		Query* check_if_exists = new Query("SELECT id FROM users WHERE username = \"" + username + "\"");
		*db << check_if_exists;

		if(check_if_exists -> result != "") {
			res.set_content("username already exists", "text/plain");
		} else {
			Query* add_user = new Query("INSERT INTO users (username, email, password_hash) VALUES ( \"" + username + "\", \"" + email + "\", \"" + password_hash + "\"); ");
			*db << add_user;

			res.set_content("added user", "text/plain");
		}

		// Query* query = new Query("INSERT INTO users")
	});

	app.get("/test", [](const Request& req, Response& res){
		res.set_content("HelloWorld!", "text/plain");
	});

	app.listen("0.0.0.0", 8000);
	
	
	

	return 0;
}
