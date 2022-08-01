include_directories(BEFORE
  ${CMAKE_SOURCE_DIR}/libCERTI 
  ${CMAKE_SOURCE_DIR}/test/libgraphc
  ${CMAKE_SOURCE_DIR}/include/
  ${CMAKE_SOURCE_DIR}/include/ieee1516-2010
  ${CMAKE_BINARY_DIR}/include/ieee1516-2010)
include_directories(${CERTI_SOURCE_DIR}/libHLA)

set(pump_SRCS
    pumpMain.cpp
    pump.cpp
    )

# set(panel_SRCS
# panelMain.cpp
# panel.cpp
#     )

set(changer_SRCS
changerMain.cpp
changer.cpp
    )



add_executable(pump ${pump_SRCS})
# add_executable(panel ${panel_SRCS})
add_executable(changer ${changer_SRCS})

set_target_properties(pump
    PROPERTIES COMPILE_FLAGS "-DX_DISPLAY_MISSING=1")
target_link_libraries(pump
    RTI1516e FedTime1516e)

# set_target_properties(panel
#     PROPERTIES COMPILE_FLAGS "-DX_DISPLAY_MISSING=1")
# target_link_libraries(panel
#     RTI1516e FedTime1516e)

set_target_properties(changer
    PROPERTIES COMPILE_FLAGS "-DX_DISPLAY_MISSING=1")
target_link_libraries(changer
    RTI1516e FedTime1516e)

install(TARGETS pump
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )

# install(TARGETS panel
#     RUNTIME DESTINATION bin
#     LIBRARY DESTINATION lib
#     ARCHIVE DESTINATION lib
#     )

install(TARGETS changer
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )

install(FILES
    Base.xml Interactions.xml Objects.xml
    DESTINATION share/federations)
