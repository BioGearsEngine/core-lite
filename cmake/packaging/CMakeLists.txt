include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "BioGears Lite")
set(CPACK_PACKAGE_VENDOR "Applied Resaerch Associates")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.txt")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_PACKAGE_HOMEPAGE_URL "http://biogearsengine.com")
set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/biogearslogo_XoQ_icon.ico")

set(CPACK_PACKAGE_INSTALL_DIRECTORY "BioGears/Lite/${CMAKE_VERSION_MAJOR}.${CMAKE_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_MAJOR ${${ROOT_PROJECT_NAME}_VERSION_MAJOR} )      
set(CPACK_PACKAGE_VERSION_MINOR ${${ROOT_PROJECT_NAME}_VERSION_MINOR} )
set(CPACK_PACKAGE_VERSION_PATCH ${${ROOT_PROJECT_NAME}_VERSION_PATCH} )
set(CPACK_PACKAGE_VERSION_TWEAK ${${ROOT_PROJECT_NAME}_VERSION_TWEAK} )


if(WIN32 AND NOT UNIX)
  # There is a bug in NSI that does not handle full UNIX paths properly.
  # Make sure there is at least one set of four backlashes.
  set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}\\\\BioGearsLogo.bmp")
  set(CPACK_NSIS_INSTALLED_ICON_NAME "${CMAKE_CURRENT_SOURCE_DIR}/biogearslogo_XoQ_icon.ico")
  set(CPACK_NSIS_DISPLAY_NAME "BioGears Lite")
  set(CPACK_NSIS_HELP_LINK "http:\\\\\\\\www.biogearsengine.com")
  set(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.biogearsengine.com")
  set(CPACK_NSIS_CONTACT "admin@biogearsengine.com")
  set(CPACK_NSIS_MODIFY_PATH ON)
  set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)

  set(CPACK_WIX_UPGRADE_GUID "ce7d0646-34c3-11eb-adc1-0242ac120002")
  set(CPACK_WIX_PRODUCT_ICON "${CMAKE_CURRENT_SOURCE_DIR}/biogearslogo_XoQ_icon.ico")
  set(CPACK_WIX_PROGRAM_MENU_FOLDER "BioGears Lite")
  set(CPACK_WIX_UI_BANNER "${CMAKE_CURRENT_SOURCE_DIR}/WIX_UI_BANNER.png")
  set(CPACK_WIX_UI_DIALOG "${CMAKE_CURRENT_SOURCE_DIR}/WIX_UI_DIALOG.png")
  set(CPACK_WIX_PROPERTY_ARPCOMMENTS "Command line utilities for BioGears Lite ${${ROOT_PROJECT_NAME}_VERSION_TAG}")
  set(CPACK_WIX_PROPERTY_ARPHELPLINK  "https://biogearsengine.com")
  set(ARPURLINFOABOUT  "https://biogearsengine.com")

  install(CODE "
    include(BundleUtilities)

    function(EXELIST result curdir)
      file(GLOB children RELATIVE \${curdir} \${curdir}/*)
      set(artifactList \"\")
      foreach(child \${children})
        if(NOT IS_DIRECTORY \${curdir}/\${child} AND \${child} MATCHES \".*${CMAKE_EXECUTABLE_SUFFIX}\" )
          list(APPEND artifactList \${curdir}/\${child})
        set(\${result} \${artifactList} PARENT_SCOPE)
        return()
        endif()
      endforeach()
    endfunction()

    string(TOUPPER \${CMAKE_INSTALL_CONFIG_NAME} CONFIG)
    if( NOT UNIX )
      set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY \${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_\${CONFIG}} )
      set( CMAKE_LIBRARY_OUTPUT_DIRECTORY \${CMAKE_LIBRARY_OUTPUT_DIRECTORY_\${CONFIG}} )
      set( CMAKE_RUNTIME_OUTPUT_DIRECTORY \${CMAKE_RUNTIME_OUTPUT_DIRECTORY_\${CONFIG}} )
    endif()
    

    set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})

    foreach(_dir IN LISTS CMAKE_PREFIX_PATH)
      list(APPEND THIRD_PARTY  \"\${_dir}\")
      list(APPEND THIRD_PARTY_LIB \"\${_dir}/lib\")
      list(APPEND THIRD_PARTY_BIN \"\${_dir}/bin\")
    endforeach()
    
    EXELIST(artifacts \${CMAKE_INSTALL_PREFIX}/bin )
    
    foreach( artifact IN LISTS artifacts)
      fixup_bundle(\${artifact}
                      \"\"
                    \"\${THIRD_PARTY_LIB};\${THIRD_PARTY_BIN}\"
        )
    endforeach()
  "         
  )

  install(DIRECTORY 
    ${PROJECT_SOURCE_DIR}/share/xsd 
    DESTINATION bin/
    FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
    )

install(DIRECTORY 
    ${PROJECT_SOURCE_DIR}/share/data/
    DESTINATION bin/
    FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
    FILES_MATCHING 
    PATTERN "*.xml"
    PATTERN "*.xlsx"
    )
install(DIRECTORY 
    ${PROJECT_SOURCE_DIR}/share/Scenarios
    DESTINATION bin/
    FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
    FILES_MATCHING 
    PATTERN "*.xml"
    )

else()
  set(CPACK_STRIP_FILES "")
  set(CPACK_SOURCE_STRIP_FILES "")
endif()
set(CPACK_PACKAGE_EXECUTABLES "bg-cli;Command Line" "bg-scenario;Scenario Driver")

include(CPack)
