###################################################
#                                                 #
#                                                 #
#	Tier1 CMake build file                        #
#                                                 #
#                                                 #
###################################################

add_sources(
	bitbuf.cpp
	byteswap.cpp
	CharacterSet.cpp
	checksum_crc.cpp
	checksum_md5.cpp
	ConVar.cpp
	DataManager.cpp
	Diff.cpp
	generichash.cpp
	#interface.cpp
	JobThread.cpp
	KeyValues.cpp
	MemPool.cpp
	MemStack.cpp
	netadr.cpp
	processor_detect.cpp
	RangeCheckedVar.cpp
	StringPool.cpp
	strtools.cpp
	tier1.cpp
	TokenReader.cpp
	UtlBuffer.cpp
	UtlString.cpp
	UtlSymbol.cpp
	xboxstubs.cpp
)

#Included by processor_detect.cpp
#if( UNIX )
#	add_sources(
#		processor_detect_linux.cpp
#	)
#endif()

add_subdirectory( ${CMAKE_SOURCE_DIR}/source_sdk/common source_sdk/common )
add_subdirectory( ${CMAKE_SOURCE_DIR}/source_sdk/public source_sdk/public )

preprocess_sources()

include_directories(
	${CMAKE_SOURCE_DIR}/source_sdk/public
	${CMAKE_SOURCE_DIR}/source_sdk/public/tier1
	${CMAKE_SOURCE_DIR}/source_sdk/common
	${CMAKE_SOURCE_DIR}/common
	${CMAKE_SOURCE_DIR}/public
	${CMAKE_SOURCE_DIR}
)

add_library( tier1 STATIC ${PREP_SRCS} )

target_compile_definitions( tier1 PRIVATE
	${SHARED_DEFS}
	TIER1_STATIC_LIB
	NO_MALLOC_OVERRIDE
)

set_target_properties( tier1 
	PROPERTIES COMPILE_FLAGS "${LINUX_32BIT_FLAG}" 
	LINK_FLAGS "${LINUX_32BIT_FLAG}" )
	
	#No lib prefix
SET_TARGET_PROPERTIES( tier1 PROPERTIES PREFIX "" )

#Create filters
create_source_groups( "${CMAKE_SOURCE_DIR}" )

clear_sources()
