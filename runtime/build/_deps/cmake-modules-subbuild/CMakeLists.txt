# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.19.2)

# We name the project and the target for the ExternalProject_Add() call
# to something that will highlight to the user what we are working on if
# something goes wrong and an error message is produced.

project(cmake-modules-populate NONE)

include(ExternalProject)
ExternalProject_Add(cmake-modules-populate
                     "UPDATE_DISCONNECTED" "False" "GIT_REPOSITORY" "https://github.com/abs-tudelft/cmake-modules.git" "GIT_TAG" "master"
                    SOURCE_DIR          "/home/yyunon/thesis_journals/resources/fletcher-oc-accel/runtime/build/_deps/cmake-modules-src"
                    BINARY_DIR          "/home/yyunon/thesis_journals/resources/fletcher-oc-accel/runtime/build/_deps/cmake-modules-build"
                    CONFIGURE_COMMAND   ""
                    BUILD_COMMAND       ""
                    INSTALL_COMMAND     ""
                    TEST_COMMAND        ""
                    USES_TERMINAL_DOWNLOAD  YES
                    USES_TERMINAL_UPDATE    YES
)


