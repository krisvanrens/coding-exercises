add_definitions(-DENABLE_DOCTEST_IN_LIBRARY)
include(FetchContent)

# v2.4.11
FetchContent_Declare(DocTest GIT_REPOSITORY "https://github.com/doctest/doctest" GIT_TAG "ae7a13539fb71f270b87eb2e874fbac80bc8dda2")
FetchContent_MakeAvailable(DocTest)

include_directories(${DOCTEST_INCLUDE_DIR})
