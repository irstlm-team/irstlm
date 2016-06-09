# Install script for directory: /Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/inst")
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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES
    "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src/cmd.h"
    "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src/cplsa.h"
    "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src/crc.h"
    "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src/cswam.h"
    "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src/dictionary.h"
    "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src/doc.h"
    "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src/gzfilebuf.h"
    "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src/htable.h"
    "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src/index.h"
    "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src/interplm.h"
    "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src/linearlm.h"
    "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src/lmclass.h"
    "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src/lmContainer.h"
    "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src/lmInterpolation.h"
    "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src/lmmacro.h"
    "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src/lmtable.h"
    "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src/mdiadapt.h"
    "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src/mempool.h"
    "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src/mfstream.h"
    "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src/mixture.h"
    "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src/n_gram.h"
    "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src/ngramcache.h"
    "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src/ngramtable.h"
    "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src/normcache.h"
    "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src/shiftlm.h"
    "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src/thpool.h"
    "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src/timer.h"
    "/Users/nicolabertoldi/Work/Software/IRSTLM/GitHubRepository/irstlm_CLION/src/util.h"
    )
endif()

