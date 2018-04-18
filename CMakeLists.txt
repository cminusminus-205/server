cmake_minimum_required(VERSION 2.8)

project(server)

# set(CMAKE_CXX_STANDARD 17)

if(APPLE)
	set(CMAKE_CXX_FLAGS "-std=c++11")
else()
	set(CMAKE_CXX_FLAGS "-std=c++17")
endif()

add_subdirectory(main)
#add_subdirectory(dependencies)
