add_executable (BoxTests BoxTests.cpp)
message(${BOXDEPS})
target_link_libraries(BoxTests Box ${BOXDEPS})
add_custom_target(check COMMAND BoxTests)
