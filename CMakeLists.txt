cmake_minimum_required(VERSION 3.16.0 FATAL_ERROR)

option(CANFORM_BUILD_SHARED "Build shared library" ${BUILD_SHARED_LIBS})
option(CANFORM_BUILD_TEST "Build test" ON)

project(VMM
	VERSION 1.0.1
	DESCRIPTION "GUI library for Canvases and Forms"
	LANGUAGES CXX)

add_compile_options(-Wall -Wextra -Wpedantic)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CANFORM_TYPE STATIC)
if(CANFORM_BUILD_SHARED)
	set(CANFORM_TYPE SHARED)
	message(STATUS "Building shared library for CanForm")
else()
	message(STATUS "Building static library for CanForm")
endif()

if(EMSCRIPTEN)
else()
	find_package(wxWidgets REQUIRED COMPONENTS core base)
	if(wxWidgets_USE_FILE) # not defined in CONFIG mode
	    include(${wxWidgets_USE_FILE})
	endif()

	add_library(canform ${CANFORM_TYPE}
		src/wx/wx.cpp
		src/wx/render.cpp)

	target_include_directories(canform PRIVATE include)

	target_link_libraries(canform PRIVATE ${wxWidgets_LIBRARIES})

	add_executable(canform_wx src/wx/test.cpp)

	target_include_directories(canform_wx PRIVATE include)

	target_link_libraries(canform_wx PRIVATE ${wxWidgets_LIBRARIES} canform)
endif()