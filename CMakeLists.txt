cmake_minimum_required(VERSION 3.30)

include(CheckIPOSupported)
check_ipo_supported(RESULT HAS_IPO)
if(HAS_IPO)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

project(Spraymaker VERSION 2.0.2 LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools)

qt_standard_project_setup(
    I18N_SOURCE_LANGUAGE en
    I18N_TRANSLATED_LANGUAGES es de fr)

add_subdirectory(dependencies)
add_subdirectory(src)
