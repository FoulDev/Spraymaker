cmake_minimum_required(VERSION 3.16)

project(Spraymaker VERSION 2.0.0 LANGUAGES CXX)
set(CMAKE_INSTALL_PREFIX dist/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR})

add_compile_definitions(SPRAYMAKER_VERSION_MAJOR=${PROJECT_VERSION_MAJOR})
add_compile_definitions(SPRAYMAKER_VERSION_MINOR=${PROJECT_VERSION_MINOR})
add_compile_definitions(SPRAYMAKER_VERSION_PATCH=${PROJECT_VERSION_PATCH})

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(PkgConfig REQUIRED)

find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools)
qt_standard_project_setup(
    I18N_SOURCE_LANGUAGE en
    I18N_TRANSLATED_LANGUAGES es de fr)

file(GLOB TS_FILES Spraymaker_*.ts)

set(PROJECT_SOURCES
        main.cpp
        spraymaker.h
        spraymaker.cpp
        spraymaker.ui
        ${TS_FILES}
)

qt_add_executable(Spraymaker
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}

    spraymakerexception.h
    vtf_defs.h

    spraymakerapplication.h spraymakerapplication.cpp
    spraymakermodel.h spraymakermodel.cpp
    dropimage.h dropimage.cpp
    imagehelper.h imagehelper.cpp
    sizedisplaylabel.h sizedisplaylabel.cpp
    util.h util.cpp
    imageloader_ffmpeg.h imageloader_ffmpeg.cpp
    customstepspinbox.h customstepspinbox.cpp
    imagemanager.h imagemanager.cpp
    gamespray.h gamespray.cpp
    settings.h settings.cpp

    assets.qrc
)

qt_add_translations(Spraymaker)

target_compile_features(Spraymaker PRIVATE cxx_std_23)
target_include_directories(Spraymaker PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Spraymaker PRIVATE Qt6::Widgets)

# ========== crnlib ==========
target_link_libraries(Spraymaker PRIVATE crn)

# ========== libvips ==========
pkg_search_module(VIPS REQUIRED IMPORTED_TARGET vips-cpp)
target_link_libraries(Spraymaker PRIVATE PkgConfig::VIPS)

# ========== ffmpeg ==========
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavdevice
    libavfilter
    libavformat
    libavcodec
    libswresample
    libswscale
    libavutil
)
target_link_libraries(Spraymaker PRIVATE PkgConfig::LIBAV)

set_target_properties(Spraymaker PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS Spraymaker
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(Spraymaker)
