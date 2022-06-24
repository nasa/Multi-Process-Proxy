cmake_minimum_required(VERSION 2.6.4)
project(CFE_PROXY C)

include_directories(fsw/mission_inc)
include_directories(fsw/platform_inc)
include_directories(${proxy_client_MISSION_DIR}/fsw/flat_inc)
include_directories(${proxy_client_MISSION_DIR}/fsw/public_inc)
include_directories(${flat_lib_MISSION_DIR}/include)
include_directories(${nng_lib_MISSION_DIR}/include)

aux_source_directory(fsw/src APP_SRC_FILES)

# Create the app module
add_cfe_app(proxy ${APP_SRC_FILES})
