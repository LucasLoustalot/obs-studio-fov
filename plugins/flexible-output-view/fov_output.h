/**
 * @file fov_output.h
 * @author The FOV Team
 * @brief Definition of the FOV Output
 * The FOV output allows for a multi-track video stream over SRT or RIST
 * @version 0.1
 * @date 2026-01-01
*/

#define debug(format, ...) blog(LOG_DEBUG, "FOV: " format, ##__VA_ARGS__)
#define info(format, ...) blog(LOG_INFO, "FOV: " format, ##__VA_ARGS__)
#define warn(format, ...) blog(LOG_WARNING, "FOV: " format, ##__VA_ARGS__)
#define OUT_ALIGN(x, a) (((x)+(a)-1)&~((a)-1))


extern struct obs_service_info fov_service;


// Extern, declared in plugins/obs-ffmpeg/fov_output.c
extern struct obs_output_info fov_output_info;

