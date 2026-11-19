cmake_minimum_required(VERSION 3.19)
project(as_vip_management LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(HL2SDK_ROOT "$ENV{HL2SDK_CS2}" CACHE PATH "Path to sourcesdk")
set(MMSOURCE_ROOT "$ENV{MMSOURCE_KHOOK}" CACHE PATH "Path to metamod-source")

set(PLUGIN_DIR "admin_modules" CACHE PATH "Path to plugin folder in addons/")

foreach(dep HL2SDK_ROOT MMSOURCE_ROOT)
  if(NOT ${dep} OR NOT IS_DIRECTORY "${${dep}}")
    message(FATAL_ERROR "${dep} isn`t defined")
  endif()
endforeach()

add_subdirectory("${HL2SDK_ROOT}" sdk)

add_library(${PROJECT_NAME} SHARED
  src/vip_management.cpp
  src/globals.cpp
  src/config.cpp
  src/database_manager.cpp
  src/logger.cpp
  src/menu_manager.cpp
  src/time_formatter.cpp
  ../SCHEMA/schemasystem.cpp
  ../SCHEMA/CCSPlayerController.cpp
  ../SCHEMA/Helper.hpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
  src
  include ../SCHEMA
  ${MMSOURCE_ROOT}/core
  ${MMSOURCE_ROOT}/third_party/khook/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  sourcesdk
)

set_target_properties(${PROJECT_NAME}  PROPERTIES PREFIX "" CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)

target_compile_definitions(${PROJECT_NAME} PRIVATE
  HAVE_STDINT_H GNUC LINUX _LINUX POSIX _FILE_OFFSET_BITS=64
  META_IS_SOURCE2 _GLIBCXX_USE_CXX11_ABI=0
  $<$<CONFIG:Release>:NDEBUG> $<$<CONFIG:Debug>:DEBUG _DEBUG>
)

target_compile_options(${PROJECT_NAME} PRIVATE
  -pipe -frtti -msse -fno-strict-aliasing -fvisibility=hidden -fvisibility-inlines-hidden
  -fno-threadsafe-statics -fno-omit-frame-pointer -ffunction-sections -fdata-sections
  -Wall -Wno-uninitialized -Wno-parentheses -Wno-unused -Wno-switch
  -Wno-non-virtual-dtor -Wno-overloaded-virtual -Wno-register -Wno-invalid-offsetof
  -Wno-delete-non-virtual-dtor -Wno-inconsistent-missing-override
  $<$<NOT:$<CONFIG:Debug>>:-Os>
)

target_link_options(${PROJECT_NAME} PRIVATE -static-libstdc++ -lgcc_eh)
target_link_libraries(${PROJECT_NAME} PRIVATE sourcesdk) 

set(PACKAGE_DIR "${CMAKE_BINARY_DIR}/package" CACHE PATH "Plugin package output")
set(PLUGIN_BIN_DIR "${PACKAGE_DIR}/addons/${PLUGIN_DIR}")

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} --only-keep-debug "$<TARGET_FILE:${PROJECT_NAME}>" "$<TARGET_FILE:${PROJECT_NAME}>.debug"
    COMMAND ${CMAKE_STRIP} --strip-unneeded "$<TARGET_FILE:${PROJECT_NAME}>"
    COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink="$<TARGET_FILE:${PROJECT_NAME}>.debug" "$<TARGET_FILE:${PROJECT_NAME}>"
  )
endif()

file(GENERATE
  OUTPUT "${PACKAGE_DIR}/addons/metamod/${PROJECT_NAME}.vdf"
  CONTENT "\"Metamod Plugin\"\n{\n\t\"file\"\t\"addons/${PLUGIN_DIR}/${PROJECT_NAME}\"\n}\n"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "${PLUGIN_BIN_DIR}"
  COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${PROJECT_NAME}>" "${PLUGIN_BIN_DIR}/"
  COMMENT "Packaging ${PROJECT_NAME}"
)