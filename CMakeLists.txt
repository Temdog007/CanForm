cmake_minimum_required(VERSION 3.16.0 FATAL_ERROR)

option(CANFORM_BUILD_SHARED "Build shared library" ${BUILD_SHARED_LIBS})
option(CANFORM_BUILD_TEST "Build test" ON)

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
	src/canform.cpp)

target_include_directories(canform PRIVATE include)

if(EMSCRIPTEN)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${USE_FLAGS} -sALLOW_MEMORY_GROWTH=1")
    set(CMAKE_EXE_LINKER_FLAGS
    	"${CMAKE_EXE_LINKER_FLAGS} ${USE_FLAGS} -sEXPORTED_RUNTIME_METHODS=ccall,stringToNewUTF8")

	add_library(canform_em ${CANFORM_TYPE}
		src/em/em.cpp)

	target_include_directories(canform_em PRIVATE
		include)

	target_link_libraries(canform_em PRIVATE canform)

	if(CANFORM_BUILD_TEST)
		add_executable(canform_em_test
			src/tests/test.cpp
			src/tests/em/main.cpp)

		target_include_directories(canform_em_test PRIVATE
			include)

		target_link_libraries(canform_em_test PRIVATE
			canform
			canform_em)
	endif()
else()
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)

	add_library(canform_gtkmm ${CANFORM_TYPE}
		src/gtkmm/gtkmm.cpp)

	target_include_directories(canform_gtkmm PRIVATE
		include
		${GTKMM_INCLUDE_DIRS})

	target_link_libraries(canform_gtkmm PRIVATE canform ${GTKMM_LIBRARIES})

	if(CANFORM_BUILD_TEST)
		set(CANFORM_GTKMM_SOURCES
			src/tests/gtkmm/test.cpp
			src/tests/test.cpp)

		if(WIN32)
			add_executable(canform_gtkmm_test WIN32 ${CANFORM_GTKMM_SOURCES})
		else()
			add_executable(canform_gtkmm_test ${CANFORM_GTKMM_SOURCES})
		endif()

		target_include_directories(canform_gtkmm_test PRIVATE
			include
			${GTKMM_INCLUDE_DIRS})

		target_link_libraries(canform_gtkmm_test PRIVATE
			${GTKMM_LIBRARIES}
			canform
			canform_gtkmm)
	endif()
endif()