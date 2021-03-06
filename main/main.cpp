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


std::string get_email(httplib::Headers headers){
	std::string email = "";

	try {
		std::for_each(headers.begin(), headers.end(), [&](auto& pair){
			if(pair.first == "email")	email = pair.second;
		});
	} catch(std::exception e) {
		email = "";
	}

	return email;
}

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
	Query get_user_privileges = Query("SELECT password_hash,privileges FROM users WHERE email = \"" + auth_email + "\";");
	auto result = *db << get_user_privileges;

	if(result.size() == 0) 
		return "AUTH FAILED: NOT A USER";

	// next, check to make sure the password match
	if(result.front()["password_hash"] != auth_password_hash)
		return "AUTH FAILED: INCORRECT PASSWORD";

	// finally, return the privileges
	std::string output = result.front()["privileges"];
	return output;
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

		json j_vec(*db << Query(req.body));
		res.set_content(j_vec.dump(4), "text/plain");

	});

	app.post("/user/login", [&](const auto& req, auto& res){
		json data;

		try {
			data = json::parse(req.body);
		} catch (nlohmann::detail::parse_error e) {
			res.set_content("ERROR: invalid request", "text/plain");
			return;
		}

		try {
			json reply;

			// extract username and password hash
			std::string email = data["email"];
			std::string password_hash = data["password_hash"];

			//get user with this username from database
			std::list<json> result = *db << Query("SELECT password_hash, privileges FROM users WHERE email = \"" + email + "\";");

			// if the passwords match, let the user know they have logged in successfully
			if(result.size() > 0 && result.front()["password_hash"] == password_hash) {
				reply["STATUS"] = "SUCCESS";
				reply["PRIVILEGES"] = result.front()["privileges"];
			} else {
				reply["STATUS"] = "FAILURE";
				reply["ERROR_MSG"] = "Your email or password was incorrect";
			}

			res.set_content(reply.dump(), "application/json");


		} catch(const nlohmann::detail::type_error e) {
			res.set_content("ERROR: invalid request", "text/plain");
		}
	});

	app.post("/user/signup", [&](const auto& req, auto& res){
		json data;

		try{
			data = json::parse(req.body);
		} catch(nlohmann::detail::parse_error e) {
			res.set_content("ERROR: invalid request", "text/plain");
			return;
		}

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
		std::list<json> result = *db << Query("SELECT id FROM users WHERE email = \"" + email + "\"");

		if(result.size() == 0) {
			*db << Query("INSERT INTO users (email, first_name, last_name, password_hash, privileges) VALUES ( \"" + email + "\", \"" + first_name + "\", \"" + last_name + "\", \"" + password_hash + "\", \"PUBLIC\"); ");
			res.set_content("added user", "text/plain");
		} else {
			res.set_content("email already registered", "text/plain");
		}

	});

	// fetches info on the user with the given email and returns it
	app.post("/user/info", [&](const auto& req, auto& res){
		std::list<json> result = *db << Query("SELECT * FROM users WHERE email = \"" + req.body + "\";");

		if(result.size() == 0) {
			res.set_content("NOT REGISTERED", "text/plain");
		} else {
			res.set_content(result.front().dump(), "application/json");
		}

	});

	//list the current emergencies, possibly with a given selector
	app.get("/emergency/list", [&](const auto& req, auto& res){
		json j_vec(*db << Query("SELECT * FROM emergencies ORDER BY id DESC;"));		
		res.set_content(j_vec.dump(), "application/json");
	});

	// submits an emergency report, it will need to be verified
	app.post("/emergency/report", [&](const auto& req, auto& res){
		json data;
		json reply;

		if(!(		get_privileges(req.headers) == "PUBLIC" 
			||	get_privileges(req.headers) == "FIRST RESPONDER" 
			||	get_privileges(req.headers) == "CONTROL CENTER"
			||	get_privileges(req.headers) == "GOD")) {

			reply["STATUS"] = "FAILURE";
			reply["ERROR_MSG"] = "You are not authorized to do that";
			res.set_content(reply.dump(), "application/json");
			return;
		}

		try {
			data = json::parse(req.body);
		} catch(nlohmann::detail::parse_error e) {
			res.set_content("ERROR: Invalid request", "text/plain");
		}

		std::string type;
		std::string description;
		std::string latitude = "";
		std::string longitude = "";

		// get latitude and longitude data
		try {
			latitude = data["latitude"];
			longitude = data["longitude"];
		} catch (std::exception e) {
			latitude = "";
			longitude = "";
		}

		// get type and description data
		try {
			type = data["type"];
			description = data["description"];
		} catch (std::exception e) {
			reply["STATUS"] = "FAILURE";
			reply["ERROR_MSG"] = "Invalid request";
			res.set_content(reply.dump(), "application/json");
			return;
		}

		Query* submit_report;

		if(latitude == "" || longitude == ""){
			*db << Query("INSERT INTO reports (type, description, status, sent_by, sent_by_type) VALUES (\"" + type + "\", \"" + description + "\", \"UNVERIFIED\", \"" + get_email(req.headers) + "\", \"" + get_privileges(req.headers) + "\");");
		} else {
			*db << Query("INSERT INTO reports (type, description, status, latitude, longitude, sent_by, sent_by_type)"
					+ std::string("VALUES (\"" + type + "\", \"" + description + "\", \"" + "UNVERIFIED" + "\", \"") 
					+ latitude + "\", \"" + longitude + "\", \"" + get_email(req.headers) + "\", \"" + get_privileges(req.headers) + "\");");
		}

		reply["STATUS"] = "SUCCESS";
		res.set_content(reply.dump(), "application/json");

	});

	app.get("/emergency/unverified", [&](const auto& req, auto& res){
		json reply;

		//make sure the user is authorized to view this information
		if(!(get_privileges(req.headers) == "GOD" || get_privileges(req.headers) == "CONTROL CENTER")) {
			json error_message;

			error_message["STATUS"] = "FAILURE";
			error_message["ERROR_MSG"] = "You are not authorized";

			reply[0] = error_message;

			res.set_content(reply.dump(), "application/json");
			return;
		}

		// if the user is authorized, proceed with the data
		json j_vec(*db << Query("SELECT * FROM reports WHERE status = \"UNVERIFIED\" ORDER BY time;"));
		reply = j_vec;

		res.set_content(reply.dump(), "application/json");

	});

	app.post("/emergency/verify", [&](const auto& req, auto& res){
		json data;
		json reply;

		std::string id;
		std::string action;

		// make sure the user is authorized
		if(!(get_privileges(req.headers) == "GOD" || get_privileges(req.headers) == "CONTROL CENTER")) {
			reply["STATUS"] = "FAILURE";
			reply["ERROR_MSG"] = "You are not authorized";
			res.set_content(reply.dump(), "application/json");
			return;
		}

		try {
			data = json::parse(req.body);
			id = data["id"];
			action = data["action"];
		} catch(std::exception e) {
			reply["STATUS"] = "FAILURE";
			reply["ERROR_MSG"] = "Invalid request";
			res.set_content(reply.dump(), "application/json");
			return;
		}

		json report = (*db << Query("SELECT * FROM reports WHERE id=" + id + ";")).front();

		if(action == "DENY") {
			*db << Query("UPDATE reports SET status = \"DENIED\" WHERE id = " + id + ";");
			reply["STATUS"] = "SUCCESS";
		} else if(action == "PERMIT") {
			*db << Query("UPDATE reports SET status = \"PERMITTED\" WHERE id = " + id + ";");
			reply["STATUS"] = "SUCCESS";

			std::string type;
			std::string latitude;
			std::string longitude;
			std::string description;

			// res.set_content(report.dump(), "application/json");

			try {
				type = report["type"];
				latitude = report["latitude"];
				longitude = report["longitude"];
				description = report["description"];
			} catch (std::exception e) {
				reply["STATUS"] = "FAILURE";
				reply["ERROR_MSG"] = "Inavlid request";
				res.set_content(reply.dump(), "application/json");

				return;
			}

			// add report to emergency
			*db << Query("INSERT INTO emergencies (type, latitude, longitude, status, description) VALUES (\"" + type + "\", \"" + latitude + "\", \"" + longitude + "\", \"ACTIVE\", \"" + description + "\");");
		}

		res.set_content(reply.dump(), "application/json");

	});

	app.post("/emergency/info", [&](const auto& req, auto& res){
		auto data = json::parse(req.body);
	});

	app.get("/chat/get", [&](const auto& req, auto& res){
		json reply;
		if(!(		get_privileges(req.headers) == "GOD"
			||	get_privileges(req.headers) == "CONTROL CENTER"
			||	get_privileges(req.headers) == "FIRST RESPONDER")){

			reply["STATUS"] = "FAILURE";
			reply["ERROR_MSG"] = "You are not authorized";
			res.set_content("[" + reply.dump() + "]", "application/json");
			return;
		}

		json j_vec(*db << Query("SELECT * FROM chat ORDER BY time DESC;"));
		res.set_content(j_vec.dump(), "/application/json");
	});

	app.post("/chat/post", [&](const auto& req, auto& res){
		json reply;
		if(!(		get_privileges(req.headers) == "GOD"
			||	get_privileges(req.headers) == "CONTROL CENTER"
			||	get_privileges(req.headers) == "FIRST RESPONDER")){

			reply["STATUS"] = "FAILURE";
			reply["ERROR_MSG"] = "You are not authorized";
			res.set_content("[" + reply.dump() + "]", "application/json");
			return;
		}

		json data;

		std::string email;
		std::string message;

		try {
			data = json::parse(req.body);

			email = get_email(req.headers);
			message = data["message"];
		} catch (std::exception e) {
			reply["STATUS"] = "FAILURE";
			reply["ERROR_MSG"] = "Invalid request";

			res.set_content("[" + reply.dump() + "]", "application/json");
			return;
		}

		*db << Query("INSERT INTO chat (sent_by, message) VALUES (\"" + email + "\", \"" + message + "\"); ORDER BY ");

		reply["STATUS"] = "SUCCESS";
		res.set_content(reply.dump(), "application/json");

	});

	// app.post("/emergency/")

	app.get("/test", [](const Request& req, Response& res){
		res.set_content("HelloWorld!", "text/plain");
	});

	app.listen("0.0.0.0", 8000);
	
	
	

	return 0;
}
