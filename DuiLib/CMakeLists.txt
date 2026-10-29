# cmake file for DuiLib

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(RUNTIME_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} RootSrc)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Control ControlSrc)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Core CoreSrc)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Layout LayoutSrc)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Utils UtilsSrc)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Control)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Core)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Layout)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Utils)

add_library(DuiLib SHARED ${ControlSrc} ${CoreSrc} ${LayoutSrc} ${UtilsSrc} ${RootSrc})

add_definitions(-DUILIB_EXPORTS)
target_link_libraries(DuiLib comctl32)
set_target_properties(DuiLib PROPERTIES
    DEBUG_POSTFIX ""
    OUTPUT_NAME "DuiLib" 
)

add_custom_command(TARGET DuiLib POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/DuiLib.dll ${CMAKE_BINARY_DIR}/../bin/DuiLib.dll)

# 复制 WebView2 Loader（运行时需与 exe 同目录；Win10+ 且已安装 WebView2 Runtime 时生效）
# 注意：仓库默认工具集 v100（VS2010）不编译 WebView2 代码（DUILIB_USE_WEBVIEW2 自动关闭），
# 使用 VS2015+ 工具集构建时自动启用 WebView2 支持。
add_custom_command(TARGET DuiLib POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_SOURCE_DIR}/3rd/WebView2/lib/x86/WebView2Loader.dll ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/WebView2Loader.dll)

