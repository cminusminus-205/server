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
	Query* get_user_privileges = new Query("SELECT password_hash,privileges FROM users WHERE email = \"" + auth_email + "\";");
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
			Query* query = new Query("SELECT password_hash, privileges FROM users WHERE email = \"" + email + "\";");
			*db << query;

			// if the passwords match, let the user know they have logged in successfully
			if(query -> result.size() > 0 && query -> result.front()["password_hash"] == password_hash) {
				reply["STATUS"] = "SUCCESS";
				reply["PRIVILEGES"] = query -> result.front()["privileges"];
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
		Query* check_if_exists = new Query("SELECT id FROM users WHERE email = \"" + email + "\"");
		*db << check_if_exists;

		if(check_if_exists -> result.size() == 0) {
			Query* add_user = new Query("INSERT INTO users (email, first_name, last_name, password_hash, privileges) VALUES ( \"" + email + "\", \"" + first_name + "\", \"" + last_name + "\", \"" + password_hash + "\", \"PUBLIC\"); ");
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
	app.get("/emergency/list", [&](const auto& req, auto& res){
		Query* get_emergencies = new Query("SELECT * FROM emergencies ORDER BY id DESC;");
		*db << get_emergencies;
		json j_vec(get_emergencies -> result);
		
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
			submit_report = new Query("INSERT INTO reports (type, description, status) VALUES (\"" + type + "\", \"" + description + "\", \"UNVERIFIED\");");
		} else {
			submit_report = new 	Query("INSERT INTO reports (type, description, status, latitude, longitude)"
									+ std::string("VALUES (\"" + type + "\", \"" + description + "\", \"" + "UNVERIFIED" + "\", \"") 
									+ latitude + "\", \"" + longitude + "\");");
		}

		*db << submit_report;

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
		Query* get_reports = new Query("SELECT * FROM reports WHERE status = \"UNVERIFIED\" ORDER BY time;");
		*db << get_reports;

		reply = get_reports -> result;

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

		Query* get_report = new Query("SELECT * FROM reports WHERE id=" + id + ";");
		*db << get_report;
		json report = get_report -> result.front();

		if(action == "DENY") {
			Query* deny_report = new Query("UPDATE reports SET status = \"DENIED\" WHERE id = " + id + ";");
			*db << deny_report;
			reply["STATUS"] = "SUCCESS";
		} else if(action == "PERMIT") {
			Query* permit_report = new Query("UPDATE reports SET status = \"PERMITTED\" WHERE id = " + id + ";");
			*db << permit_report;
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
			Query* add_emergency = new Query("INSERT INTO emergencies (type, latitude, longitude, status, description) VALUES (\"" + type + "\", \"" + latitude + "\", \"" + longitude + "\", \"ACTIVE\", \"" + description + "\");");
			*db << add_emergency;
		}

		res.set_content(reply.dump(), "application/json");

		// add_emergency();
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
			res.set_content(reply.dump(), "application/json");
			return;
		}

		Query* get_chat = new Query("SELECT * FROM chat;");
		*db << get_chat;

		json j_vec(get_chat -> result);

		res.set_content(j_vec.dump(), "/application/json");
	});

	app.post("/chat/post", [&](const auto& req, auto& res){
		json reply;
		if(!(		get_privileges(req.headers) == "GOD"
			||	get_privileges(req.headers) == "CONTROL CENTER"
			||	get_privileges(req.headers) == "FIRST RESPONDER")){

			reply["STATUS"] = "FAILURE";
			reply["ERROR_MSG"] = "You are not authorized";
			res.set_content(reply.dump(), "application/json");
			return;
		}

		json data;

		std::string email;
		std::string message;

		try {
			data = json::parse(req.body);

			email = get_email(req.headers);
			message = data["message"];

			res.set_content(email + " " + message, "text/plain");
			return;
		} catch (std::exception e) {
			reply["STATUS"] = "FAILURE";
			reply["ERROR_MSG"] = "Invalid request";
			res.set_content(reply.dump(), "application/json");
			return;
		}

		Query* post_message = new Query("INSRT INTO chat (sent_by, message) VALUES (\"" + email + "\", \"" + message + "\");");
		*db << post_message;

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
