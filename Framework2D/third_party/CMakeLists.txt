# glfw
add_subdirectory(glfw)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set_target_properties(glfw update_mappings PROPERTIES FOLDER "third_party/GLFW3")

# glad
set(glad_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/glad" CACHE STRING "")
file(GLOB source
  "${glad_SOURCE_DIR}/src/*.c"
)
add_library(glad ${source})
target_include_directories(glad
  PUBLIC "${glad_SOURCE_DIR}/include"
)
set_target_properties(glad PROPERTIES FOLDER "third_party")

# imgui
set(imgui_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui" CACHE STRING "")
set(imgui_file_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ImGuiFileDialog" CACHE STRING "")
file(GLOB source
    "${imgui_SOURCE_DIR}/*.h"
    "${imgui_SOURCE_DIR}/*.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.*"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.*"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3_loader.*"
    "${imgui_file_SOURCE_DIR}/*.h"
    "${imgui_file_SOURCE_DIR}/*.cpp"
)
add_library(imgui ${source})
target_include_directories(imgui
    PUBLIC "${imgui_SOURCE_DIR}"
    PUBLIC "${imgui_SOURCE_DIR}/backends"
    PUBLIC "${imgui_file_SOURCE_DIR}"
)
target_link_libraries(imgui PUBLIC glfw)
set_target_properties(imgui PROPERTIES FOLDER "third_party")


# dlib配置
set(dlib_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dlib-19.24" CACHE STRING "")
add_subdirectory(${dlib_SOURCE_DIR} "${CMAKE_CURRENT_BINARY_DIR}/dlib" EXCLUDE_FROM_ALL)

# 禁用不需要的
set(DLIB_NO_GUI_SUPPORT ON CACHE BOOL "Disable GUI components" FORCE)
set(DLIB_PNG_SUPPORT OFF CACHE BOOL "Disable PNG support" FORCE)
set(DLIB_JPEG_SUPPORT OFF CACHE BOOL "Disable JPEG support" FORCE)