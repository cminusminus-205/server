set(MAIN_SRCS
	httplib.h
	main.cpp
	database.h
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
