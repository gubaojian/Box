find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Lua 5.2 REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

include_directories (${OPENGL_INCLUDE_DIRS} ${GLFW_INCLUDE_DIRS} ${LUA_INCLUDE_DIR})
add_executable (Default Default.cpp)
target_link_libraries(Default Box ${BOXDEPS} ${GLFW_STATIC_LIBRARIES} ${OPENGL_LIBRARIES} ${LUA_LIBRARIES})