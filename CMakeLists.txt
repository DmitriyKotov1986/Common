cmake_minimum_required(VERSION 3.14)

#Настройка==================================================================
project(Common LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Sql Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Sql Network)

#сборка=======================================================================
set(HEADER_FILES
    regcheck.h
    tdbloger.h
    tdbquery.h
    thttpquery.h
    common.h
)

set(SOURCE_FILES
    regcheck.cpp
    tdbloger.cpp
    tdbquery.cpp
    thttpquery.cpp
    common.cpp
)

add_library(${PROJECT_NAME} STATIC
    ${HEADER_FILES}
    ${SOURCE_FILES}
)

target_link_libraries(${PROJECT_NAME} Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Sql Qt${QT_VERSION_MAJOR}::Network)

#Установка==============================================================================
#создаем папки
#install(TARGETS ${PROJECT_NAME}
#    LIBRARY DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/../Lib
#    ARCHIVE DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/../Lib
#    INCLUDES DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/../Headers
#)

#копируем папки
#install(FILES ${HEADER_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/../Headers)

