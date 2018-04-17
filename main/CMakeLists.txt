set(MAIN_SRCS
	httplib.h
	main.cpp
)

add_executable(
	main 
	${MAIN_SRCS}
)

target_link_libraries(
	main
	# http
)