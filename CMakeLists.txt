cmake_minimum_required(VERSION 3.16.0 FATAL_ERROR)

option(CANFORM_BUILD_SHARED "Build shared library" ${BUILD_SHARED_LIBS})
option(CANFORM_BUILD_TEST "Build test" ON)
option(CANFORM_BUILD_WX "Build wxWidgets version" OFF)
option(CANFORM_BUILD_GTK "Build GTK version" ON)

project(CANFORM
	VERSION 1.0.0
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

add_library(canform ${CANFORM_TYPE}
	src/canvas.cpp)

target_include_directories(canform PRIVATE include)

if(EMSCRIPTEN)
else()
	if(CANFORM_BUILD_WX)
		find_package(wxWidgets REQUIRED COMPONENTS core base)
		include(${wxWidgets_USE_FILE})

		add_library(canform_wx ${CANFORM_TYPE}
			src/wx/wx.cpp
			src/wx/render.cpp)

		target_include_directories(canform_wx PRIVATE include)

		target_link_libraries(canform_wx PRIVATE ${wxWidgets_LIBRARIES})

		if(CANFORM_BUILD_TEST)
			add_executable(canform_wx_test src/tests/wx/test.cpp)

			target_include_directories(canform_wx_test PRIVATE
				include)

			target_link_libraries(canform_wx_test PRIVATE
				${wxWidgets_LIBRARIES}
				canform
				canform_wx)
		endif()
	endif()
	if(CANFORM_BUILD_GTK)
		find_package(PkgConfig REQUIRED)
		pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

		add_library(canform_gtk ${CANFORM_TYPE}
			src/gtk/gtk.cpp
			src/gtk/render.cpp)

		target_include_directories(canform_gtk PRIVATE
			include
			${GTK3_INCLUDE_DIRS})

		target_link_libraries(canform_gtk PRIVATE ${GTK3_LIBRARIES})

		if(CANFORM_BUILD_TEST)
			add_executable(canform_gtk_test src/tests/gtk/test.cpp)

			target_include_directories(canform_gtk_test PRIVATE
				include
				${GTK3_INCLUDE_DIRS})

			target_link_libraries(canform_gtk_test PRIVATE
				${GTK3_LIBRARIES}
				canform
				canform_gtk)
		endif()
	endif()
endif()