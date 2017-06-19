cmake_minimum_required (VERSION 2.8.11)
project (Box)
set(CMAKE_CXX_FLAGS "-std=c++11 -fno-exceptions -w")

option(BuildPlayground "BuildPlayground" OFF)
include_directories (${CMAKE_CURRENT_SOURCE_DIR} /usr/local/include)
link_directories(/usr/local/lib ${CMAKE_INSTALL_PREFIX}/lib)

set (BOXDEPS Stick Brick Scrub)

set (BOXINC 
Box/BasicTypes.hpp
Box/Box.hpp
Box/Constants.hpp
Box/Event.hpp
Box/EventForwarder.hpp
Box/EventPublisher.hpp
Box/MouseEvents.hpp
Box/MouseState.hpp
)

set (BOXSRC 
Box/Box.cpp
Box/Event.cpp
Box/MouseEvents.cpp
Box/MouseState.cpp
)

add_library (Box SHARED ${BOXSRC})
add_library (BoxStatic STATIC ${BOXSRC})
target_link_libraries(Box ${BOXDEPS})
target_link_libraries(BoxStatic ${BOXDEPS})
install (TARGETS Box BoxStatic DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
foreach ( file ${BOXINC} )
    get_filename_component( dir ${file} DIRECTORY )
    install( FILES ${file} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${dir} )
endforeach()
add_subdirectory (Tests)

if(BuildPlayground)
add_subdirectory (Playground)
endif()
