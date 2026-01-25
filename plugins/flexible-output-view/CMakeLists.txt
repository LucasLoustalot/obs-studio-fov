cmake_minimum_required(VERSION 3.28)

add_library(flexible-output-view MODULE)
add_library(OBS::flexible-output-view ALIAS flexible-output-view)

target_sources(flexible-output-view PRIVATE fov.c fov_output.c)

target_include_directories(flexible-output-view PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../obs-ffmpeg"
)

target_link_libraries(flexible-output-view INTERFACE OBS::ffmpeg)
target_link_libraries(flexible-output-view PUBLIC
    OBS::libobs
    $<$<PLATFORM_ID:Windows>:OBS::w32-pthreads>
    OBS::frontend-api
    OBS::media-playback
    OBS::opts-parser
    FFmpeg::avcodec
    FFmpeg::avfilter
    FFmpeg::avformat
    FFmpeg::avdevice
    FFmpeg::avutil
    FFmpeg::swscale
    FFmpeg::swresample
    $<$<PLATFORM_ID:Windows>:OBS::w32-pthreads>
    $<$<PLATFORM_ID:Windows>:ws2_32>
    $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:Libva::va>
    $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:Libva::drm>
    $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:Libpci::pci>
    $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:Libdrm::Libdrm>
    Librist::Librist
    Libsrt::Libsrt
)

set_target_properties_obs(flexible-output-view PROPERTIES FOLDER plugins PREFIX "")