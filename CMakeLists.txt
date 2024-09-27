cmake_minimum_required(VERSION 3.16)
project(Spraymaker VERSION 2.0.1 LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools)

qt_standard_project_setup(
    I18N_SOURCE_LANGUAGE en
    I18N_TRANSLATED_LANGUAGES es de fr)

add_subdirectory(dependencies)
add_subdirectory(src)
