set(MAIN_SRCS
	httplib.h
	main.cpp
	database.h
	json.hpp
	endpoints/user.cpp
)

add_executable(
	main 
	${MAIN_SRCS}
)

target_link_libraries(
	main
	pthread
	sqlite
)
