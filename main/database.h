#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <list>
#include <vector>
#include <sstream>
#include <iostream>
#include <map>

enum {BUSY, DONE} status;

class Query;

int sqlite_callback(void *NotUsed, int argc, char** argv, char** azColName);
std::list<Query*> operation_queue;

class Query {
public:
	Query(std::string _SQL) {
		SQL = _SQL;
		finished = false;
	}
	~Query() {}

	void write_back(std::string row, bool done) {
		if(done) {
			finished = true;
		} else {
			// result.push_back(row);
		}
	}

	std::string result;

	std::string SQL;
	bool finished;
};

class Database {
public:
	Database(std::string path){
		int status = sqlite3_open(path.c_str(), &db);
	}
	~Database(){}

	void execute_next(){
		std::stringstream output;

		sqlite3_stmt* stmt;
		int rc;

		Query* operation = operation_queue.front();
		sqlite3_prepare_v2(db, operation -> SQL.c_str(), -1, &stmt, NULL);

		// for(int i=0; i<sqlite3_column_count(stmt); i++) {
		// 	output << sqlite3_column_name(stmt, i) << ",";
		// }

		// output << std::endl;

		while ( (rc = sqlite3_step(stmt)) == SQLITE_ROW) {  
			for(int i=0; i<sqlite3_column_count(stmt); i++) {
				const char* col = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
				output << (col ? std::string(col) : "NULL");
				// output << (sqlite3_column_text(stmt, i) ? (std::string s(sqlite3_column_text(stmt, i))) : "NULL") << ",";
			}
		}

		operation_queue.front() -> result = output.str();
		if(!operation_queue.empty()) operation_queue.pop_front();	
	}

	sqlite3* db;
	char* error_message;

	Database* operator<<(Query* operation) {
		operation_queue.push_back(operation);
		this -> execute_next();
		return this;
	}
};

int sqlite_callback(void *NotUsed, int argc, char** argv, char** azColName){
	std::cout << "called" << std::endl;
	std::stringstream output;
	for(int i = 0; i<argc; i++) {
		output << azColName[i] << " = " << (argv[i] ? argv[i] : "NULL") << "\n";
	}

	operation_queue.front() -> write_back(output.str(), false);
	if(!operation_queue.empty()) operation_queue.pop_front();

	return 0;
}


#endif