# Install script for directory: /Users/marcello/Workspace/software/irstlm/trunk/scripts

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/Users/marcello/Workspace/software/irstlm")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE PROGRAM PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE FILES
    "/Users/marcello/Workspace/software/irstlm/trunk/scripts/add-start-end.sh"
    "/Users/marcello/Workspace/software/irstlm/trunk/scripts/build-lm-qsub.sh"
    "/Users/marcello/Workspace/software/irstlm/trunk/scripts/build-lm.sh"
    "/Users/marcello/Workspace/software/irstlm/trunk/scripts/build-sublm.pl"
    "/Users/marcello/Workspace/software/irstlm/trunk/scripts/goograms2ngrams.pl"
    "/Users/marcello/Workspace/software/irstlm/trunk/scripts/lm-stat.pl"
    "/Users/marcello/Workspace/software/irstlm/trunk/scripts/mdtsel.sh"
    "/Users/marcello/Workspace/software/irstlm/trunk/scripts/merge-sublm.pl"
    "/Users/marcello/Workspace/software/irstlm/trunk/scripts/ngram-split.pl"
    "/Users/marcello/Workspace/software/irstlm/trunk/scripts/rm-start-end.sh"
    "/Users/marcello/Workspace/software/irstlm/trunk/scripts/sort-lm.pl"
    "/Users/marcello/Workspace/software/irstlm/trunk/scripts/split-dict.pl"
    "/Users/marcello/Workspace/software/irstlm/trunk/scripts/split-ngt.sh"
    "/Users/marcello/Workspace/software/irstlm/trunk/scripts/wrapper"
    )
endif()

