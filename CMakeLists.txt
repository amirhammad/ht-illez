cmake_minimum_required(VERSION 2.4)
SET(CMAKE_CXX_COMPILER g++-4.8)
SET(CMAKE_C_COMPILER gcc-4.8)
SET(outputname out.elf)
project(${outputname})
SET(CMAKE_BUILD_TYPE Debug)
aux_source_directory(src out_SOURCES )
aux_source_directory(include out_HEADERS )
aux_source_directory("external/qcustomplot" QCustomPlot_SOURCES)
FILE(GLOB QCustomPlot_HEADERS "external/qcustomplot/*.h")
FILE(GLOB out_HEADERS "include/*.h")
#FILE(GLOB out_SOURCES src/*.cpp)
#message("ahoj" + ${out_SOURCES})
SET(OPENNI_DIR /home/amir/Downloads/OpenNI/OpenNI-Linux-x64-2.2)
include_directories(${OPENNI_DIR}/Include)
include_directories(external/qcustomplot)
link_directories(${OPENNI_DIR}/Redist)
include_directories(include)
SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11") 
SET(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-std=c++11 ")
# Open CV
find_package(OpenCV REQUIRED)

# libfreenect
find_package(libfreenect REQUIRED)
message("HELLO: " ${QCustomPlot_SOURCES} ${FREENECT_INCLUDE_DIRS})
include_directories(${FREENECT_INCLUDE_DIRS})
link_directories(${FREENECT_LIBRARY_DIRS})

# libusb

# find_package(libusb REQUIRED)
include_directories(/usr/include/libusb-1.0)

# QT
find_package(Qt COMPONENTS QtCore QtOpenGL REQUIRED)
SET(QT_USE_QTOPENGL TRUE)
INCLUDE(${QT_USE_FILE})
include_directories(${QT_INCLUDES})
QT4_WRAP_CPP(out_HEADERS_MOC ${out_HEADERS} ${QCustomPlot_HEADERS})
ADD_DEFINITIONS(${QT_DEFINITIONS})

add_executable(${outputname} ${QCustomPlot_SOURCES} ${out_SOURCES} ${out_HEADERS_MOC} ${out_HEADERS})
target_link_libraries(${outputname} ${QT_LIBRARIES} glut GLU armadillo  OpenNI2 -pthread)
target_link_libraries(${outputname} ${OpenCV_LIBRARIES})
target_link_libraries(${outputname} ${FREENECT_LIBRARIES})

