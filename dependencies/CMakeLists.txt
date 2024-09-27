include(FetchContent)

FetchContent_Declare(
  crunch
  GIT_REPOSITORY https://github.com/DaemonEngine/crunch.git
  GIT_TAG        0e2e5537b8ab8e4b6fa156edad2cf31b4ed9bda5 # master 21 Jul 2024
)

add_subdirectory(crunch)
