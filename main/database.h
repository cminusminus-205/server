#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <list>
#include <vector>
#include <sstream>
#include <iostream>
#include <map>
#include "json.hpp"

using json = nlohmann::json;

class Query;

std::list<Query*> operation_queue;

class Query {
public:
	Query(std::string _SQL) {
		SQL = _SQL;
	}
	~Query() {}

	std::list<json> result;

	std::string SQL;
};

class Database {
public:
	Database(std::string path){
		int status = sqlite3_open(path.c_str(), &db);
		std::cout << status;
	}
	~Database(){}

	void execute(Query* operation){
		//store our output as a list of rows, with each row being a json object
		std::list<json> output;

		//create the sql stmt we want to execute
		sqlite3_stmt* stmt;

		// Query* operation = operation_queue.front();
		sqlite3_prepare_v2(db, operation -> SQL.c_str(), -1, &stmt, NULL);

		while ( sqlite3_step(stmt) == SQLITE_ROW) {
			// add a new row ot the list
			
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

		// store the result
		operation -> result = output;
	}

	// wipes the database
	void wipe(){
		*this << new Query("DROP TABLE IF EXISTS users;");
		*this << new Query("DROP TABLE IF EXISTS emergencies;");
		*this << new Query("CREATE TABLE users (id INTEGER UNIQUE PRIMARY KEY, first_name TEXT, last_name TEXT, email TEXT NOT NULL UNIQUE, password_hash TEXT NOT NULL, privileges TEXT NOT NULL);");
		*this << new Query("CREATE TABLE emergencies (id INTEGER UNIQUE PRIMARY KEY, type TEXT NOT NULL, latitude DOUBLE, longitude DOUBLE);");

		// the database should always have a god user
		*this << new Query("INSERT INTO users (email, first_name, password_hash, privileges) VALUES ('god@user.com', 'god', 'password', 'GOD');");
	}

	sqlite3* db;
	char* error_message;

	Database* operator<<(Query* operation) {
		// operation_queue.push_back(operation);
		this -> execute(operation);
		return this;
	}
};


#endif