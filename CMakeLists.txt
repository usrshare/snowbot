cmake_minimum_required(VERSION 2.8)
project(snowbot)

set (CMAKE_C_FLAGS "-std=c99 -Wall -Wextra -D_POSIX_C_SOURCE=2")

set_source_files_properties(tags PROPERTIES GENERATED true)

add_custom_target(tags
        COMMAND ctags -R .
	        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

include_directories (${CMAKE_SOURCE_DIR}/inc)

add_executable (snowbot
		src/main.c
		src/irc.c
		src/activity.c
		src/paste.c
	       )

include_directories(${CMAKE_SOURCE_DIR}/libircclient/libircclient/include)

target_link_libraries(snowbot ${CMAKE_SOURCE_DIR}/libircclient/libircclient/src/libircclient.a)

install(TARGETS snowbot
	RUNTIME DESTINATION bin
	)
