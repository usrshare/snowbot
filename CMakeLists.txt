cmake_minimum_required(VERSION 2.8)
project(snowbot)

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")

set (CMAKE_C_FLAGS "-std=c99 -Wall -Wextra -D_POSIX_C_SOURCE=200809L ${CMAKE_C_FLAGS}")

else()

set (CMAKE_C_FLAGS "-Wall -D_POSIX_C_SOURCE=200809L ${CMAKE_C_FLAGS}")

endif()

set_source_files_properties(tags PROPERTIES GENERATED true)

add_custom_target(tags
        COMMAND ctags -R .
	        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

include_directories (${CMAKE_SOURCE_DIR}/inc)

add_executable (snowbot
		src/main.c
		src/boyer-moore.c
		src/irc.c
		src/irc_bookmarks.c
		src/irc_commands.c
		src/irc_commands_weather.c
		src/irc_common.c
		src/irc_proto.c
		src/irc_user.c
		src/irc_watch.c
		src/http.c
		src/activity.c
		src/convert.c
		src/derail.c
		src/entities.c
		src/hashtable.c
		src/paste.c
		src/pwdhash.c
		src/resolve.c
		src/savefile.c
		src/short.c
		src/weather.c
	       	src/xrates.c
		)

target_link_libraries(snowbot m curl json-c pthread)

install(TARGETS snowbot
	RUNTIME DESTINATION bin
	)
