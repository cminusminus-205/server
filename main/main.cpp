#include <iostream>
#include "httplib.h"
#include <sqlite3.h>
#include "json.hpp"
#include <sstream>

#include "database.h"

using json = nlohmann::json;
using namespace httplib;

Server app;
Database* db;

std::string get_privileges(httplib::Headers headers){
	std::string auth_email = "";
	std::string auth_password_hash = "";

	try {
		std::for_each(headers.begin(), headers.end(), [&](auto& pair){
			if(pair.first == "email")			auth_email = pair.second;
			if(pair.first == "password_hash")	auth_password_hash = pair.second;
		});
	} catch (std::exception e) {
		return "BAD REQUEST";
	}

	if(auth_email == "" || auth_password_hash == "") 
		return "BAD REQUEST";

	// Query the user database
	Query* get_user_privileges = new Query("SELECT password_hash,privileges FROM users WHERE email = '" + auth_email + "';");
	*db << get_user_privileges;

	if(get_user_privileges -> result.size() == 0) 
		return "AUTH FAILED: NOT A USER";

	// next, check to make sure the password match
	if(get_user_privileges -> result.front()["password_hash"] != auth_password_hash)
		return "AUTH FAILED: INCORRECT PASSWORD";

	// finally, return the privileges
	return get_user_privileges -> result.front()["privileges"];

}

int main() {
	//open a connection to our database
	db = new Database("/root/server/data.db");

	db -> wipe();

	app.post("/data/execute", [&](const auto& req, auto& res){

		std::string privileges = get_privileges(req.headers);

		if(privileges != "GOD") {
			res.set_content("ERROR: NOT AUTHORIZED", "text/plain");
			return;
		}

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

		try {
			// extract username and password hash
			std::string email = data["email"];
			std::string password_hash = data["password_hash"];

			//get user with this username from database
			Query* query = new Query("SELECT password_hash FROM users WHERE email = \"" + email + "\";");
			*db << query;

			// res.set_content(query -> result, "text/plain");

			// if the passwords match, let the user know they have logged in successfully
			if(query -> result.size() > 0 && query -> result.front()["password_hash"] == password_hash) {
				res.set_content("SUCCESS", "text/plain");
			} else {
				res.set_content("FAILURE", "text/plain");
			}

		} catch(const nlohmann::detail::type_error e) {
			res.set_content("ERROR: invalid request", "text/plain");
		}
	});

	app.post("/user/signup", [&](const auto& req, auto& res){
		auto data = json::parse(req.body);

		std::string email;
		std::string first_name; 
		std::string last_name;
		std::string password_hash;

		try {
			email = data["email"];
			first_name = data["first_name"];
			last_name = data["last_name"];
			password_hash = data["password_hash"];
			} catch(const nlohmann::detail::type_error e){
				res.set_content("ERROR: invalid request", "text/plain");
				return;
			}

		//Check to see if the user already exists
		Query* check_if_exists = new Query("SELECT id FROM users WHERE email = \"" + email + "\"");
		*db << check_if_exists;

		if(check_if_exists -> result.size() == 0) {
			Query* add_user = new Query("INSERT INTO users (email, first_name, last_name, password_hash, priveleges) VALUES ( \"" + email + "\", \"" + first_name + "\", \"" + last_name + "\", \"" + password_hash + "\", 'PUBLIC'); ");
			*db << add_user;

			res.set_content("added user", "text/plain");
		} else {
			res.set_content("email already registered", "text/plain");
		}
	});

	// fetches info on the user with the given email and returns it
	app.post("/user/info", [&](const auto& req, auto& res){
		Query* get_user = new Query("SELECT * FROM users WHERE email = \"" + req.body + "\";");
		*db << get_user;

		if(get_user -> result.size() == 0) {
			res.set_content("NOT REGISTERED", "text/plain");
		} else {
			//json j_vec(get_user -> result);
			res.set_content(get_user -> result.front().dump(), "application/json");
		}
	});

	//list the current emergencies, possibly with a given selector
	app.post("/emergency/list", [&](const auto& req, auto& res){
		std::string selector = req.body;
		Query* get_emergencies;


		if(selector == "") {
			get_emergencies = new Query("SELECT * FROM emergencies;");
		} else {
			get_emergencies = new Query("SELECT * FROM emergencies WHERE \"" + selector + ";");
		}

		*db << get_emergencies;

		json j_vec(get_emergencies -> result);
		res.set_content(j_vec.dump(), "text/plain");
	});

	// submits an emergency report, it will need to be verified
	app.post("/emergency/report", [&](const auto& req, auto& res){
		json data;

		std::for_each(req.headers.begin(), req.headers.end(), [&](auto& pair){

			std::stringstream output;
			output << pair.first;

			const std::string str = output.str();

			res.set_content(str, "text/plain");

			return;
		});

		// res.set_content(req.headers["username"], "text/plain");

		return;

		try {
			data = json::parse(req.body);
		} catch(nlohmann::detail::parse_error e) {
			res.set_content("ERROR: Invalid request", "text/plain");
		}

		std::string type;
		std::string description;
		double latitude;
		double longitude;

		// get latitude and longitude data
		try {
			// latitude = data["location"]["latitude"];
			// longitude = data["location"]["longitude"];
		} catch (nlohmann::detail::parse_error e) {
			latitude = NAN;
			longitude = NAN;
		}

		// get type and description data
		try {
			// type = data["type"];
			// description = data["description"];
		} catch (nlohmann::detail::parse_error e) {
			res.set_content("ERROR: INCOMPLETE INFORMATION", "text/plain");
			return;
		}

		Query* submit_report;

		if(latitude == NAN || longitude == NAN){
			submit_report = new Query("INSERT INTO emergencies (type, description, status) VALUES ('" + type + "', '" + description + "', '" + "UNVERIFIED" + "');");
		} else {
			submit_report = new 	Query("INSERT INTO emergencies (type, description, status, latitude, longitude)"
									+ std::string("VALUES ('" + type + "', '" + description + "', '" + "UNVERIFIED" + "', ") 
									+ std::to_string(latitude) + ", " + std::to_string(longitude) + ");");
		}

		*db << submit_report;
		// json j_vec(submit_report -> result);

		res.set_content("Submited report", "/text/plain");
	});

	app.post("/emergency/info", [&](const auto& req, auto& res){
		auto data = json::parse(req.body);
	});

	// app.post("/emergency/")

	app.get("/test", [](const Request& req, Response& res){
		res.set_content("HelloWorld!", "text/plain");
	});

	app.listen("0.0.0.0", 8000);
	
	
	

	return 0;
}
