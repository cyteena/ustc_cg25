project(common)
file(GLOB source
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
  "${INCLUDE_DIR}/common/*.h"
)
add_library(${PROJECT_NAME} ${source})
set_target_properties(${PROJECT_NAME} PROPERTIES 
  DEBUG_POSTFIX "_d"
  RUNTIME_OUTPUT_DIRECTORY "${BINARY_DIR}"
  LIBRARY_OUTPUT_DIRECTORY "${LIBRARY_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY "${LIBRARY_DIR}") 
target_link_libraries(${PROJECT_NAME} PUBLIC glfw glad imgui)
target_include_directories(${PROJECT_NAME} 
  PUBLIC ${INCLUDE_DIR} 
  PUBLIC ${THIRD_PARTY_DIR}
)