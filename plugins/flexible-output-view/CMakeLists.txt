cmake_minimum_required(VERSION 3.28)

add_library(flexible-output-view MODULE)
add_library(OBS::flexible-output-view ALIAS flexible-output-view)


target_sources(flexible-output-view PRIVATE
    fov.cpp
    fov_system.cpp
    service/fov_service.cpp
    service/curl_wrapper.cpp
)

target_include_directories(flexible-output-view PRIVATE
    ${OBS_FRONTEND_API_INCLUDE_DIRS}
    ./service
)

target_link_libraries(flexible-output-view PRIVATE OBS::libobs $<$<PLATFORM_ID:Windows>:OBS::w32-pthreads> OBS::frontend-api CURL::libcurl)

set_target_properties_obs(flexible-output-view PROPERTIES FOLDER plugins PREFIX "")