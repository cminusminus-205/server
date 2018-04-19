set(SQLITE_SRCS
	shell.c
	sqlite3.c
	sqlite3.h
	sqlite3ext.h
)

add_library(
	sqlite STATIC
	${SQLITE_SRCS}
)

target_include_directories(sqlite PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)