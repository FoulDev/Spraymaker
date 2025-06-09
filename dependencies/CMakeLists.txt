include(FetchContent)

FetchContent_Declare(
    crunch
    GIT_REPOSITORY https://github.com/DaemonEngine/crunch.git
    GIT_TAG        73e74fa7e28093c4802a3034f78edd81d0d8544b # master 5 Mar 2025
    EXCLUDE_FROM_ALL
)

add_subdirectory(crunch)
