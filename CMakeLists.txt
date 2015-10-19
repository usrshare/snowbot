cmake_minimum_required(VERSION 2.8)
project(snowbot)

set (CMAKE_C_FLAGS "-std=c99 -Wall -Wextra -D_POSIX_C_SOURCE=200809L")

set_source_files_properties(tags PROPERTIES GENERATED true)

add_custom_target(tags
        COMMAND ctags -R .
	        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

include_directories (${CMAKE_SOURCE_DIR}/inc)

add_executable (snowbot
		src/main.c
		src/irc.c
		src/irc_bookmarks.c
		src/irc_commands.c
		src/irc_common.c
		src/irc_proto.c
		src/irc_user.c
		src/irc_watch.c
		src/http.c
		src/activity.c
		src/derail.c
		src/hashtable.c
		src/paste.c
		src/savefile.c
		src/weather.c
	       	src/xrates.c
		)

include_directories(${CMAKE_SOURCE_DIR}/libircclient/libircclient/include)

target_link_libraries(snowbot m curl json-c ${CMAKE_SOURCE_DIR}/libircclient/libircclient/src/libircclient.a)

install(TARGETS snowbot
	RUNTIME DESTINATION bin
	)
