find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

include_directories (${OPENGL_INCLUDE_DIRS} ${GLFW_INCLUDE_DIRS})
add_executable (Default Default.cpp)
target_link_libraries(Default Box ${BOXDEPS} ${GLFW_STATIC_LIBRARIES} ${OPENGL_LIBRARIES})