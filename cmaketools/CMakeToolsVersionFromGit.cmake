# Distributed under the MIT License.
# See https://github.com/brobeson/cmake-tools/blob/main/license for details.

#[=[.rst:
CMakeToolsVersionFromGit
------------------------

This module attempts to determine a project version from a Git tag.
It searches for the most recent tag that can be reached from the current commit.
This includes annotated and lightweight tags.

Result Variables
^^^^^^^^^^^^^^^^

The module defines these variables:

  .. variable:: CMAKE_TOOLS_GIT_TAG

    The project version derived from the most recent Git tag.
    The module removes any prefix and suffix from the tag.
    For example, if the tag is ``v1.5.3``, the value of this variable is ``1.5.2``.

  .. variable:: CMAKE_TOOLS_GIT_TAG_MAJOR

    An integer for the major component of the project version.
    The module assumes this is in the tag.

  .. variable:: CMAKE_TOOLS_GIT_TAG_MINOR

    An integer for the minor component of the project version.
    If this component is not in the Git tag, this variable is set to an empty string.

  .. variable:: CMAKE_TOOLS_GIT_TAG_PATCH

    An integer for the patch component of the project version.
    If this component is not in the Git tag, this variable is set to an empty string.

  .. variable:: CMAKE_TOOLS_GIT_DISTANCE

    An integer with the number of commits between the current commit and the tag.
    If the tag points to the current commit, this variable is set to 0.

Using This Module
^^^^^^^^^^^^^^^^^

All you have to do is include this module.
Then use the appropriate variables where you would define a project version.

.. code-block:: cmake

  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake-tools")
  include(CMakeToolsVersionFromGit)

  # Using the tag as the project version:
  project(MyProject VERSION ${CMAKE_TOOLS_GIT_TAG})

  # Using the distance as the project version's tweak component:
  project(MyProject VERSION ${CMAKE_TOOLS_GIT_TAG}.${CMAKE_TOOLS_GIT_DISTANCE})

This module locates Git via ``find_package(Git)``; see the `FindGit documentation <https://cmake.org/cmake/help/latest/module/FindGit.html>`_ for details.
If this module does not find Git, it issues a warning, leaves the above variables undefined, and allows CMake to continue.

#]=]

find_package(Git)
if(NOT GIT_FOUND)
  message(
    WARNING
    "I could not find Git. Ensure it is installed and available in your path, "
    "or do not include CMakeToolsVersionFromGit."
  )
  return()
endif()

execute_process(
  COMMAND ${GIT_EXECUTABLE} describe --tags
  RESULT_VARIABLE git_result
  OUTPUT_VARIABLE git_output
  OUTPUT_STRIP_TRAILING_WHITESPACE
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  ERROR_QUIET
)

if(NOT git_result STREQUAL "0")
  set(CMAKE_TOOLS_GIT_TAG_MAJOR 0)
  set(CMAKE_TOOLS_GIT_TAG_MINOR 0)
  set(CMAKE_TOOLS_GIT_TAG_PATCH 0)
  set(CMAKE_TOOLS_GIT_DISTANCE 0)
else()
  # First, remove any nondigit prefix from the output.
  string(REGEX REPLACE "^[^0-9]+" "" git_output "${git_output}")

  # Next, extract the tag's version data and remove it from the output.
  string(REGEX MATCH "^[0-9]+(\\.[0-9]+)?(\\.[0-9]+)?" git_version "${git_output}")
  string(REPLACE "${git_version}" "" git_output "${git_output}")

  # Extract the version components.
  string(REPLACE "." ";" git_version "${git_version}")
  list(POP_FRONT git_version CMAKE_TOOLS_GIT_TAG_MAJOR)
  list(POP_FRONT git_version CMAKE_TOOLS_GIT_TAG_MINOR)
  list(POP_FRONT git_version CMAKE_TOOLS_GIT_TAG_PATCH)

  # If there is any text left, extract the distance. Otherwise, the distance is
  # 0.
  if(git_output)
    string(REPLACE "-" "" git_output "${git_output}")
    string(REGEX MATCH "^[0-9]+" CMAKE_TOOLS_GIT_DISTANCE "${git_output}")
  else()
    set(CMAKE_TOOLS_GIT_DISTANCE 0)
  endif()
endif()

# Put together the CMAKE_TOOLS_GIT_TAG variable.
if(NOT CMAKE_TOOLS_GIT_TAG_PATCH STREQUAL "")
  set(
    CMAKE_TOOLS_GIT_TAG
    ${CMAKE_TOOLS_GIT_TAG_MAJOR}.${CMAKE_TOOLS_GIT_TAG_MINOR}.${CMAKE_TOOLS_GIT_TAG_PATCH}
  )
elseif(NOT CMAKE_TOOLS_GIT_TAG_MINOR STREQUAL "")
  set(
    CMAKE_TOOLS_GIT_TAG
    ${CMAKE_TOOLS_GIT_TAG_MAJOR}.${CMAKE_TOOLS_GIT_TAG_MINOR}
  )
else()
  set(CMAKE_TOOLS_GIT_TAG ${CMAKE_TOOLS_GIT_TAG_MAJOR})
endif()
