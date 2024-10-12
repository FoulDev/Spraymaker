add_compile_definitions(SPRAYMAKER_VERSION_MAJOR=${PROJECT_VERSION_MAJOR})
add_compile_definitions(SPRAYMAKER_VERSION_MINOR=${PROJECT_VERSION_MINOR})
add_compile_definitions(SPRAYMAKER_VERSION_PATCH=${PROJECT_VERSION_PATCH})

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(PkgConfig REQUIRED)

set(TS_FILES_DIR ${PROJECT_SOURCE_DIR}/i18n)
file(GLOB TS_FILES ${TS_FILES_DIR}/Spraymaker_*.ts)

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

qt_add_translations(Spraymaker
    TS_FILE_DIR ${TS_FILES_DIR}
)

target_compile_features(Spraymaker PRIVATE cxx_std_23)
target_include_directories(Spraymaker PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Spraymaker PRIVATE Qt6::Widgets)

# ========== crnlib ==========
target_link_libraries(Spraymaker PRIVATE crn)
target_include_directories(Spraymaker PRIVATE ${crunch_SOURCE_DIR})
target_include_directories(Spraymaker PRIVATE ${crunch_SOURCE_DIR}/inc)

# ========== libvips ==========
pkg_search_module(VIPS REQUIRED IMPORTED_TARGET vips-cpp)
target_link_libraries(Spraymaker PRIVATE PkgConfig::VIPS)

# ========== ffmpeg ==========
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavformat
    libavcodec
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

add_custom_command(
    TARGET Spraymaker POST_BUILD
    DEPENDS Spraymaker
    COMMAND $<$<CONFIG:release>:${CMAKE_STRIP}>
    ARGS --strip-all $<TARGET_FILE:Spraymaker>
)

if (WIN32 OR MSYS OR CYGWIN)
    install(TARGETS Spraymaker
        RUNTIME ARCHIVE LIBRARY FRAMEWORK BUNDLE PUBLIC_HEADER RESOURCE
    )

    install(TARGETS Spraymaker
        COMPONENT Spraymaker
        RUNTIME_DEPENDENCIES
        PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-"
        POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
        DIRECTORIES ${DLL_DIRECTORIES}
    )

    install(FILES $<TARGET_RUNTIME_DLLS:Spraymaker> TYPE BIN)
endif()

qt_generate_deploy_app_script(
    TARGET Spraymaker
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
