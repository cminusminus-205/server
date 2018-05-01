#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <list>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <map>
#include "json.hpp"

using json = nlohmann::json;

class Query;

// std::list<Query*> operation_queue;

class Query {
public:
	Query(std::string _SQL) {
		SQL = _SQL;
		finished = false;
	}
	~Query() {}

	// std::list<json> result;

	std::string SQL;
	bool finished;
};

class Database {
public:
	Database(std::string path){
		int status = sqlite3_open(path.c_str(), &db);
	}
	~Database(){}

	std::list<json> execute_next(Query operation){
		//store our output as a list of rows, with each row being a json object
		std::list<json> output;

		//create the sql stmt we want to execute
		sqlite3_stmt* stmt;

		// Query* operation = operation_queue.front();
		sqlite3_prepare_v2(db, operation.SQL.c_str(), -1, &stmt, NULL);

		while ( sqlite3_step(stmt) == SQLITE_ROW) {
			//add a new row ot the list
			
			json row;

			for(int i=0; i<sqlite3_column_count(stmt); i++) {
				//get column name from statement:
				const char* column_raw = sqlite3_column_name(stmt, i);
				std::string column = std::string(column_raw);

				//get column value from statement:
				const char* value_raw = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
				std::string value = value_raw ? value_raw : "NULL";
	
				// update the row
				row[column] = value;
			}

			// add the row to the list
			output.push_back(row);
		}

		//return the result
		return output;
	}

	// wipes the database
	void wipe(){
		*this << Query("DROP TABLE IF EXISTS users;");
		*this << Query("DROP TABLE IF EXISTS emergencies;");
		*this << Query("DROP TABLE IF EXISTS reports;");
		*this << Query("DROP TABLE IF EXISTS chat");

		*this << Query("CREATE TABLE users (id INTEGER UNIQUE PRIMARY KEY, first_name TEXT, last_name TEXT, email TEXT NOT NULL UNIQUE, password_hash TEXT NOT NULL, privileges TEXT NOT NULL, phone TEXT, workAd TEXT, homeAd TEXT, workZip INTEGER, homeZip INTEGER, workType TEXT, workStation TEXT, stationZip INTEGER, verify INTEGER, identity INTEGER);");
		*this << Query("CREATE TABLE emergencies (id INTEGER UNIQUE PRIMARY KEY, type TEXT NOT NULL, latitude TEXT, longitude TEXT, status TEXT NOT NULL, description TEXT NOT NULL, time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, IS_SIMULATION TEXT);");
		*this << Query("CREATE TABLE reports (id INTEGER UNIQUE PRIMARY KEY,  type TEXT NOT NULL, latitude TEXT, longitude TEXT, description TEXT NOT NULL, status TEXT NOT NULL, time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, sent_by TEXT, sent_by_type TEXT );");
		*this << Query("CREATE TABLE chat (id INTEGER UNIQUE PRIMARY KEY, sent_by TEXT NOT NULL, message TEXT, time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP);");

		// the dase should always have a god user
		*this << Query("INSERT INTO users (email, first_name, password_hash, privileges) VALUES ('god@user.com', 'god', 'password', 'GOD');");
		*this << Query("INSERT INTO users (email, first_name, password_hash, privileges) VALUES ('test@test.com', 'public', 'password', 'PUBLIC');");
		*this << Query("INSERT INTO users (email, first_name, password_hash, privileges) VALUES ('firstresponder@test.com', 'first', 'password', 'FIRST RESPONDER');");

		*this << Query("INSERT INTO emergencies (type, latitude, longitude, status, description) VALUES ('TEST', '40.699190', '-75.211171', 'ACTIVE', 'Test, please ignore');");

	}

	sqlite3* db;
	char* error_message;

	std::list<json> operator<<(Query operation) {
		return this -> execute_next(operation);
	}
};


#endif
