// #include "../json.hpp"
// #include "../httplib.h"
// #include "../database.h"

// using json = nlohmann::json;
// using namespace httplib;

// namespace endpoints {
// 	namespace user {
// 		void include_endpoints(Server& app, Database& db){

// 		}
// 	}
// }
// 	app.post("/user/login", [&](const auto& req, auto& res){

// 		auto data = json::parse(req.body);

// 		try {
// 			// extract username and password hash
// 			std::string email = data["email"];
// 			std::string password_hash = data["password_hash"];

// 			//get user with this username from database
// 			Query* query = new Query("SELECT password_hash FROM users WHERE email = \"" + email + "\";");
// 			*db << query;

// 			// res.set_content(query -> result, "text/plain");

// 			// if the passwords match, let the user know they have logged in successfully
// 			if(query -> result.size() > 0 && query -> result.front()["password_hash"] == password_hash) {
// 				res.set_content("SUCCESS", "text/plain");
// 			} else {
// 				res.set_content("FAILURE", "text/plain");
// 			}

// 		} catch(const nlohmann::detail::type_error e) {
// 			res.set_content("ERROR: invalid request", "text/plain");
// 		}
// 	});

// 	app.post("/user/signup", [&](const auto& req, auto& res){
// 		auto data = json::parse(req.body);

// 		std::string email;
// 		std::string first_name; 
// 		std::string last_name;
// 		std::string password_hash;

// 		try {
// 			email = data["email"];
// 			first_name = data["first_name"];
// 			last_name = data["last_name"];
// 			password_hash = data["password_hash"];
// 			} catch(const nlohmann::detail::type_error e){
// 				res.set_content("ERROR: invalid request", "text/plain");
// 				return;
// 			}

// 		//Check to see if the user already exists
// 		Query* check_if_exists = new Query("SELECT id FROM users WHERE email = \"" + email + "\"");
// 		*db << check_if_exists;

// 		if(check_if_exists -> result.size() == 0) {
// 			Query* add_user = new Query("INSERT INTO users (email, first_name, last_name, password_hash) VALUES ( \"" + email + "\", \"" + first_name + "\", \"" + last_name + "\", \"" + password_hash + "\"); ");
// 			*db << add_user;

// 			res.set_content("added user", "text/plain");
// 		} else {
// 			res.set_content("email already registered", "text/plain");
// 		}
// 	});

// 	app.post("/user/info", [&](const auto& req, auto& res){
// 		Query* get_user = new Query("SELECT * FROM users WHERE email = \"" + req.body + "\";");
// 		*db << get_user;

// 		if(get_user -> result.size() == 0) {
// 			res.set_content("NOT REGISTERED", "text/plain");
// 		} else {
// 			//json j_vec(get_user -> result);
// 			res.set_content(get_user -> result.front().dump(), "application/json");
// 		}
// 	});
// }