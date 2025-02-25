# 定义项目名称（会生成变量PROJECT_NAME=1_MiniDraw_Ex）
project(1_MiniDraw_Ex)

# 自动收集源代码文件（注意：GLOB在新增文件时需要重新生成）
file(GLOB source
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.h" 
  "${CMAKE_CURRENT_SOURCE_DIR}/shapes/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/shapes/*.h" 
)

# ${CMAKE_CURRENT_SOURCE_DIR} : 因为当前的CMakeLists.txt在 D:\ustc_cg25\Framework2D\src\assignments\1_MiniDraw_Ex\CMakeLists.txt
# ${CMAKE_CURRENT_SOURCE_DIR} = D:\ustc_cg25\Framework2D\src\assignments\1_MiniDraw_Ex

# 创建可执行文件目标
add_executable(${PROJECT_NAME} ${source})

# 设置头文件搜索路径（PUBLIC表示依赖本目标的其他目标也会继承该路径）
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 配置目标属性
set_target_properties(${PROJECT_NAME} PROPERTIES 
  DEBUG_POSTFIX "_d"          # 调试版本添加_d后缀（如MiniDraw_d.exe）
  RUNTIME_OUTPUT_DIRECTORY "${BINARY_DIR}"       # 可执行文件输出路径
  LIBRARY_OUTPUT_DIRECTORY "${LIBRARY_DIR}"      # 动态库输出路径
  ARCHIVE_OUTPUT_DIRECTORY "${LIBRARY_DIR}")     # 静态库输出路径

# 链接公共依赖库（common应该是在其他CMakeLists中定义的库目标）
target_link_libraries(${PROJECT_NAME} PUBLIC common) 