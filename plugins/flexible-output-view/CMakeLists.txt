cmake_minimum_required(VERSION 3.28)

add_library(flexible-output-view MODULE)
add_library(OBS::flexible-output-view ALIAS flexible-output-view)

target_sources(flexible-output-view PRIVATE fov.c fov_service.c)

target_link_libraries(flexible-output-view PRIVATE OBS::libobs $<$<PLATFORM_ID:Windows>:OBS::w32-pthreads> OBS::frontend-api CURL::libcurl)

set_target_properties_obs(flexible-output-view PROPERTIES FOLDER plugins PREFIX "")