project(3_PoissonImageEditing)
file(GLOB source
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.h" 
  "${CMAKE_CURRENT_SOURCE_DIR}/shapes/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/shapes/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/CloneMethods/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/CloneMethods/*.cpp"
)
add_executable(${PROJECT_NAME} ${source})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${INCLUDE_DIR}/common)
set_target_properties(${PROJECT_NAME} PROPERTIES 
  DEBUG_POSTFIX "_d"
  RUNTIME_OUTPUT_DIRECTORY "${BINARY_DIR}"
  LIBRARY_OUTPUT_DIRECTORY "${LIBRARY_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY "${LIBRARY_DIR}") 
target_link_libraries(${PROJECT_NAME} PUBLIC common) 
target_compile_definitions(${PROJECT_NAME} PRIVATE -DDATA_PATH="${FRAMEWORK2D_DIR}/../Homeworks/3_poisson_image_editing/data")
