###############################################################################
# Policy adjustments
###############################################################################

###############################################################################
# Options
###############################################################################

option (${ROOT_PROJECT_NAME}_THROW_READONLY_EXCEPTIONS "Toggle the throwing CommonDataModelExceptions when a ReadOnly SEScalar is Modified." OFF)
option (${ROOT_PROJECT_NAME}_THROW_NAN_EXCEPTIONS "Toggle the throwing CommonDataModelExceptions when an  SEScalar is NaNExceptions." OFF)
option (${ROOT_PROJECT_NAME}_USE_OVERRIDE_CONTROL       "When enabled Biogears will control the ranges of override"  OFF)
option (${ROOT_PROJECT_NAME}_BUILD_CIRCUIT_TESTS       "Toggle on the Circuit Test Library which is a dependeny of libbiogears_jni"  ON)

###############################################################################
# Base Variables
###############################################################################
set(PROJECT_NAME biogears_lite)
set(PREFIX lite)

set(${PREFIX}_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libBiogearsLite/include" )
set(${PREFIX}_PRIVATE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libBiogearsLite/src" )
set(${PREFIX}_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}" )
set(${PREFIX}_UNIT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/unit" PARENT_SCOPE)

###############################################################################
# Requirments
###############################################################################
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
if(WIN32)
  set(Boost_USE_STATIC_LIBS ON) 
endif()
verify_package(Threads REQUIRED)
verify_package(Eigen3 REQUIRED)
verify_package(CodeSynthesis REQUIRED)

###############################################################################
#Code Generation
###############################################################################
file(GLOB_RECURSE biogears_xsd_list "${PROJECT_SOURCE_DIR}/share/xsd/biogears/*.xsd")
file(GLOB_RECURSE cdm_xsd_list "${PROJECT_SOURCE_DIR}/share/xsd/cdm/*.xsd")

set (BIOGEARS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (BIOGEARS_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

REGISTER_XSD_DIR(  "${PROJECT_SOURCE_DIR}/share/xsd/"
                    CONFIG "${PROJECT_SOURCE_DIR}/share/xsd/BioGearsDataModel.cfg"
                    PROJECT "biogears" COMPONENT "schema" RESOURCE_FOLDER "resources/schema"
                    TARGETS XSD_TARGETS VERBOSITY 1)

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/libBiogearsLite/src/version.cpp.in  ${CMAKE_CURRENT_BINARY_DIR}/libBiogearsLite/src/version.cpp NEWLINE_STYLE LF)
###############################################################################
#Sorce and Header Defines
###############################################################################
message(STATUS "")
message(STATUS "Configuring ${PROJECT_NAME}")

message(STATUS CMAKE_GENERATOR=${CMAKE_GENERATOR})
###############################################################################
#Define Logic
###############################################################################
if(${PREFIX}_BUILD_STATIC)
  ###############################################################################
  # libBiogears_st
  ###############################################################################
  add_subdirectory(libBiogearsLite)
else()
  ###############################################################################
  # libBiogears_cdm
  ###############################################################################
  add_subdirectory(libCDM)
  
  ###############################################################################
  # libBiogears
  ###############################################################################
  add_subdirectory(libBiogearsLite)
  
  ###############################################################################
  # libBiogears_circuit_test
  ###############################################################################
  if( ${ROOT_PROJECT_NAME}_BUILD_CIRCUIT_TESTS)
    add_subdirectory(libCircuitTest)
  endif()
  ###############################################################################
  # libBiogears_jni
  ###############################################################################
  if(Java_FOUND AND ${ROOT_PROJECT_NAME}_BUILD_JAVATOOLS AND ${ROOT_PROJECT_NAME}_BUILD_CIRCUIT_TESTS)
    add_subdirectory(libJNI)
  endif()
endif() #NOT Biogears_BUILD_STATIC

######################################################################################################
# INSTALL LOGIC
######################################################################################################
install(DIRECTORY 
    libBiogearsLite/include/biogears
    libCDM/include/biogears
    libJNI/include/biogears
    libCircuitTest/include/biogears
	  ${CMAKE_CURRENT_BINARY_DIR}/biogears
    DESTINATION include/
    FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
  )
  
install(DIRECTORY 
    ${PROJECT_SOURCE_DIR}/share/xsd 
    DESTINATION share/biogears/${${ROOT_PROJECT_NAME}_VERSION_TAG}
    FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
  )

install(DIRECTORY 
    ${PROJECT_SOURCE_DIR}/share/data
    DESTINATION share/biogears/${${ROOT_PROJECT_NAME}_VERSION_TAG}
    FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
	  FILES_MATCHING 
	    PATTERN "*.xml"
	    PATTERN "log4j.properties"
	    PATTERN "*.csv"
	    PATTERN "*.xlsx"
  )

install(DIRECTORY 
    ${PROJECT_SOURCE_DIR}/share/Scenarios
    DESTINATION share/biogears/${${ROOT_PROJECT_NAME}_VERSION_TAG}/data/
    FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
  	FILES_MATCHING 
	   PATTERN "*.xml"
 )
message(STATUS "")

