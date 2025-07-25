include("${CMAKE_CURRENT_LIST_DIR}/CPM.cmake")

# add package from github.com
function(cppxx_github_package url)
    # Parse input URL of the form "[user:password@]package:repo.git#tag"
    string(REGEX MATCH "([^:]+):([^#]+)#(.+)" _ "${url}")

    # Extract other variables
    set(PACKAGE ${CMAKE_MATCH_1})
    set(REPO_PATH ${CMAKE_MATCH_2})
    set(TAG ${CMAKE_MATCH_3})

    # Construct the full GitLab URL
    set(GIT_REPOSITORY "https://github.com/${REPO_PATH}.git")

    # Capture additional options if provided
    set(options ${ARGN})

    # Call CPMAddPackage with the parsed and constructed values
    CPMAddPackage(
        NAME           ${PACKAGE}
        GIT_REPOSITORY ${GIT_REPOSITORY}
        GIT_TAG        ${TAG}
        ${options}  # Any additional options passed to the function
    )

    set(${PACKAGE}_SOURCE_DIR ${${PACKAGE}_SOURCE_DIR} PARENT_SCOPE)
endfunction()

function(cppxx_assert_directories_exist)
    foreach(dir IN LISTS ARGN)
        if(NOT IS_DIRECTORY "${dir}")
            message(FATAL_ERROR "cppxx: Required directory does not exist: ${dir}")
        endif()
    endforeach()
endfunction()
