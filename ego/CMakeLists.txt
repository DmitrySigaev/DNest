file(GLOB_RECURSE Ego_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.java)
file(GLOB Ego_RES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} resources/*)
file(GLOB Ego_LIBS lib/*.jar)

find_package(Java REQUIRED)
include(UseJava)

get_target_property(imago_jar_filename imago_java JAR_FILE)
set(CMAKE_JAVA_INCLUDE_PATH ${Ego_LIBS} ${imago_jar_filename})
add_jar(ego ${Ego_SRC})
add_dependencies(ego imago_java)

#Add manifest
get_target_property(ego_jar_filename ego JAR_FILE)
add_custom_command(TARGET ego POST_BUILD
				   COMMAND "${Java_JAR_EXECUTABLE}" "ufm" "${ego_jar_filename}" "manifest.mf"
				   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
#Add resources
add_custom_command(TARGET ego POST_BUILD
				   COMMAND "${Java_JAR_EXECUTABLE}" "uf" "${ego_jar_filename}" "resources"
				   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

install_jar(ego ego)
install(FILES ${CMAKE_JAVA_INCLUDE_PATH} DESTINATION ego/lib)

if (UNIX)
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/runscripts/ego DESTINATION ego)
else()
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/runscripts/ego.bat DESTINATION ego)
endif()

message(STATUS "Ego added")