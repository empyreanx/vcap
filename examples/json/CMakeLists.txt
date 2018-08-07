if(NOT JANSSON_FOUND)
    message(FATAL_ERROR "You need Jansson installed in order to build examples/json.c")
endif()

add_executable(json "json.c")
target_link_libraries(json vcap ${LIBRARIES})
