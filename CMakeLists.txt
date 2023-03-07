cmake_minimum_required(VERSION 3.12)

project(serialreader VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets SerialPort Network)

file(GLOB SRC_FILES "*.cpp" "*.h" "*.qrc" "*.ui" "*.rc")
add_executable(serialreader WIN32 ${SRC_FILES})

target_link_libraries(serialreader PRIVATE
	Qt6::Widgets
	Qt6::SerialPort
	Qt::Network
)
