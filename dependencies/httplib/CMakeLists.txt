add_library(
	http INTERFACE
)

target_sources(http INTERFACE)

target_include_directories(
	http INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}
)