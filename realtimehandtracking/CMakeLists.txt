cmake_minimum_required(VERSION 3.16.3)

project(ONR_HAI_HandTracking VERSION "0.0.0" LANGUAGES C)

if (WIN32)
	set(ULTRALEAP_PATH_ROOT "$ENV{ProgramFiles}/Ultraleap")
	list(APPEND CMAKE_FIND_LIBRARY_SUFFIXES ".dll.a" ".a")
elseif (UNIX)
	set(ULTRALEAP_PATH_ROOT "/usr/share/doc/ultraleap-hand-tracking-service/lib/cmake/LeapSDK")
endif()

find_package(LeapSDK
	5
	REQUIRED
	PATHS
		"${ULTRALEAP_PATH_ROOT}")

if (UNIX)    
    find_package(Threads REQUIRED)    
endif (UNIX)

add_executable(
	leapc_example
	"leapc_main.c")

target_link_libraries(
	leapc_example
	PRIVATE
		LeapSDK::LeapC)

get_target_property(
	LEAPC_IMPORTED_CONFIG
	LeapSDK::LeapC
	IMPORTED_CONFIGURATIONS
)

get_target_property(
	LEAPC_SHARED_LIB_PATH
	LeapSDK::LeapC
	IMPORTED_LOCATION_${LEAPC_IMPORTED_CONFIG}
)

add_custom_command(
	TARGET
		leapc_example
	POST_BUILD
	COMMAND
		${CMAKE_COMMAND} -E copy
		${LEAPC_SHARED_LIB_PATH}
		$<TARGET_FILE_DIR:leapc_example>)

add_library(
	libExampleConnection
	OBJECT
	"ExampleConnection.c")

target_link_libraries(
	libExampleConnection
	PUBLIC
	LeapSDK::LeapC)

if (UNIX)
	target_link_libraries(
		libExampleConnection
		PRIVATE
		Threads::Threads)
endif()    

target_include_directories(
	libExampleConnection
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR})


# Add Handtracking
add_executable("RealTimeHandTracking" "realtimehandtracking.c")
set_property(TARGET "RealTimeHandTracking" PROPERTY FOLDER "HAI ONR")
target_link_libraries("RealTimeHandTracking" PUBLIC libExampleConnection)
if (NOT WIN32)
	target_link_libraries("RealTimeHandTracking" PUBLIC m)
endif()

# Libcurl
find_package(CURL REQUIRED)
target_link_libraries("RealTimeHandTracking" PUBLIC CURL::libcurl)

# cJSON
add_library(
	cJSON
	OBJECT
	"cJSON.c")
target_link_libraries("RealTimeHandTracking" PUBLIC cJSON)

# Add Calibration
add_executable("Calibration" "calibration.c")
set_property(TARGET "Calibration" PROPERTY FOLDER "HAI ONR")
target_link_libraries("Calibration" PUBLIC libExampleConnection)
if (NOT WIN32)
	target_link_libraries("Calibration" PUBLIC m)
endif()
