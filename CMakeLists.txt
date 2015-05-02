cmake_minimum_required(VERSION 2.4)
#SET(CMAKE_CXX_COMPILER g++-4.8)
#SET(CMAKE_C_COMPILER gcc-4.8)

execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion
                OUTPUT_VARIABLE GCC_VERSION)
if (GCC_VERSION VERSION_GREATER 4.8 OR GCC_VERSION VERSION_EQUAL 4.8)
        message(STATUS "Version >= 4.8")
else()
#        message(FATAL_ERROR "OLD GCC, must be >= 4.8, is " + ${GCC_VERSION})
endif()
SET(outputname HandTrackerApp)
project(${outputname})
SET(CMAKE_BUILD_TYPE Debug)
aux_source_directory(src out_SOURCES )
aux_source_directory("external/qcustomplot" QCustomPlot_SOURCES)
FILE(GLOB QCustomPlot_HEADERS "external/qcustomplot/*.h")
FILE(GLOB out_HEADERS "include/*.h")

if (OPENNI_DIR)
	if (NOT EXISTS ${OPENNI_DIR})
		message(FATAL_ERROR "provided directory '" ${OPENNI_DIR} "'does not exist")
	endif()
	message("Found OpenNI: " ${OPENNI_DIR})
else()
	message(FATAL_ERROR "please provide path into OPENNI_DIR variable")
endif()

include_directories(${OPENNI_DIR}/Include)
include_directories(external/qcustomplot)
link_directories(${OPENNI_DIR}/Redist)
include_directories(include)
SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11 -gdwarf-3")
SET(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-std=c++11 -gdwarf-3")
# Open CV
find_package(OpenCV REQUIRED)

if (OPENNN_DIR)
	if (NOT EXISTS ${OPENNN_DIR})
		message(FATAL_ERROR "provided directory '" ${OPENNN_DIR} "'does not exist")
	endif()
	message("Found OpenNN: " ${OPENNN_DIR})
else()
	message(FATAL_ERROR "please provide path into OPENNN_DIR variable")
endif()

INCLUDE_DIRECTORIES(${OPENNN_DIR})
LINK_DIRECTORIES(${OPENNN_DIR})

# libfreenect
find_package(libfreenect REQUIRED)
message("HELLO: " ${QCustomPlot_SOURCES} ${FREENECT_INCLUDE_DIRS})
include_directories(${FREENECT_INCLUDE_DIRS})
link_directories(${FREENECT_LIBRARY_DIRS})

# libusb

# find_package(libusb REQUIRED)
include_directories(/usr/include/libusb-1.0)

# QT
find_package(Qt COMPONENTS QtCore REQUIRED)
INCLUDE(${QT_USE_FILE})
include_directories(${QT_INCLUDES})
QT4_WRAP_CPP(out_HEADERS_MOC ${out_HEADERS} ${QCustomPlot_HEADERS})
ADD_DEFINITIONS(${QT_DEFINITIONS})

add_executable(${outputname} ${QCustomPlot_SOURCES} ${out_SOURCES} ${out_HEADERS_MOC} ${out_HEADERS})
target_link_libraries(${outputname} ${OpenCV_LIBS})
target_link_libraries(${outputname} ${QT_LIBRARIES} OpenNI2 pthread opennn ${FREENECT_LIBRARIES})

#target_link_libraries(${outputname} ${FREENECT_LIBRARIES})



#tests
find_library(gtest REQUIRED)
set(PROJECT_TEST_NAME test)
set(TEST_SRC_FILES src/test/PoseRecognitionTest.cpp)

add_executable(${PROJECT_TEST_NAME} ${TEST_SRC_FILES})
add_dependencies(${PROJECT_TEST_NAME} googletest)
target_link_libraries(${PROJECT_TEST_NAME} ${OpenCV_LIBS} ${QT_LIBRARIES} OpenNI2 opennn)
target_link_libraries(${PROJECT_TEST_NAME} gtest gtest_main pthread)
#add_test(test1 ${PROJECT_TEST_NAME})
