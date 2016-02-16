# Set output directory
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_INSTALL_PREFIX}/bin)
SET(LIBRARY_OUTPUT_PATH ${CMAKE_INSTALL_PREFIX}/lib)

set(CMAKE_MACOSX_RPATH 1)

ADD_DEFINITIONS("-D_LARGE_FILES")
ADD_DEFINITIONS("-D_FILE_OFFSET_BITS=64")
ADD_DEFINITIONS("-DMYCODESIZE=3")
ADD_DEFINITIONS("-DDEBUG")
#ADD_DEFINITIONS("-DTRACE_LEVEL=1")

if (TRACE_LEVEL)
  ADD_DEFINITIONS("-DTRACE_LEVEL=${TRACE_LEVEL}")
endif()

if (OPTION)
  ADD_DEFINITIONS("-DOPTION_${OPTION}")
endif()

if (SOLUTION)
  ADD_DEFINITIONS("-DSOLUTION_${SOLUTION}")
endif()

if (CXX0)
  MESSAGE( STATUS "HAVE_CXX0=true; hence, variable HAVE_CXX0 is set" )
  SET(STD_FLAG "-std=c++0x")
  ADD_DEFINITIONS("-DHAVE_CXX0")
else()
  MESSAGE( STATUS "HAVE_CXX0=false; hence, variable HAVE_CXX0 is unset" )
  SET(STD_FLAG "")
  ADD_DEFINITIONS("-UHAVE_CXX0")
endif() 

  

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g ${STD_FLAG} -isystem/usr/include -W -Wall -ffor-scope")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")

INCLUDE_DIRECTORIES("${PROJECT_SOURCE_DIR}/src")

SET( LIB_IRSTLM_SRC
        cmd.h cmd.c
        thpool.h thpool.c
        gzfilebuf.h index.h
        dictionary.h dictionary.cpp
        htable.h htable.cpp 
        lmContainer.h lmContainer.cpp 
        lmclass.h lmclass.cpp 
        lmmacro.h lmmacro.cpp
        lmtable.h lmtable.cpp
        lmInterpolation.h lmInterpolation.cpp
        mempool.h mempool.cpp 
        mfstream.h mfstream.cpp 
        n_gram.h n_gram.cpp 
        ngramcache.h ngramcache.cpp
        ngramtable.h ngramtable.cpp
        timer.h timer.cpp 
        util.h util.cpp 
        crc.h crc.cpp 
	interplm.h interplm.cpp
        linearlm.h linearlm.cpp
        mdiadapt.h mdiadapt.cpp 
        mixture.h mixture.cpp 
        normcache.h normcache.cpp 
        shiftlm.h shiftlm.cpp
        cplsa.h cplsa.cpp 
        cswam.h cswam.cpp 
        doc.h doc.cpp
	lmContextDependent.h lmContextDependent.cpp
        context-similarity.h context-similarity.cpp
)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET( IRSTLM_EXE dict ngt tlm dtsel plsa cswa compile-lm interpolate-lm prune-lm quantize-lm score-lm verify-caching context-dependent-evaluation)


if (BUILD_STATIC_LIBS)
  MESSAGE(STATUS "Compiling static library")
  ADD_LIBRARY(irstlmStatic STATIC ${LIB_IRSTLM_SRC})
  set_target_properties(irstlmStatic PROPERTIES OUTPUT_NAME irstlm)
  FOREACH(CMD ${IRSTLM_EXE})
    ADD_EXECUTABLE(${CMD} ${CMD}.cpp)
    TARGET_LINK_LIBRARIES (${CMD} irstlmStatic -lm -lz -lpthread)
  ENDFOREACH()
endif()
if (BUILD_SHARED_LIBS)
  MESSAGE(STATUS "Compiling shared library")
  SET (CMAKE_SHARED_LINKER_FLAGS "-lz")
  ADD_LIBRARY(irstlmShared SHARED ${LIB_IRSTLM_SRC})
  set_target_properties(irstlmShared PROPERTIES OUTPUT_NAME irstlm)

  FOREACH(CMD ${IRSTLM_EXE})
    ADD_EXECUTABLE(${CMD}Shared ${CMD}.cpp)
    TARGET_LINK_LIBRARIES (${CMD}Shared irstlmShared -lm -lz -lpthread)
    set_target_properties(${CMD}Shared PROPERTIES OUTPUT_NAME ${CMD})
  ENDFOREACH()
endif()

#INSTALL INCLUDE FILES
FILE(GLOB includes src *.h)
INSTALL(FILES ${includes} DESTINATION include)

add_custom_command(OUTPUT LNS
                   COMMAND ln -s -f -n lib lib64
		   WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}
                   COMMENT "creating link from lib to lib64")

add_custom_target(LinkToLib64 ALL DEPENDS LNS)

