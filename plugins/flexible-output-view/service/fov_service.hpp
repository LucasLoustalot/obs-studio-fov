/**
 * @file fov_service.hpp
 * @author The FOV Team
 * @brief FOV custom service implementation, this service is used together with fov_output
 * @version 0.1
 * @date 2026-02-07
 */

#pragma once

#include <stdexcept>
#include <stdio.h>

#include <nlohmann/json.hpp>

#include <obs.h>
#include <obs-module.h>
#include <obs-data.h>
#include <obs-properties.h>
#include <string>

#include "curl_wrapper.hpp"

#define API_FFMPEG_START_ROUTE "/ffmpeg/register"
#define API_FFMPEG_STOP_ROUTE "/ffmpeg/stop"

extern "C" {
void registerFOVService(void);
}

class FOVService {
public:


	FOVService(obs_data_t *settings, obs_service_t *) noexcept;
	~FOVService() noexcept;

	const char *getName() const noexcept;
	void update(obs_data_t *settings) noexcept;
	obs_properties_t *getProperties(void) noexcept;
	void activate(obs_data_t *settings) noexcept;
	void deactivate(void) noexcept;
	const char *getURL(void) noexcept;
	void applyEncoderSettings(obs_data_t *video_settings, obs_data_t *audio_settings) noexcept;
	const char *getConnectInfo(uint32_t type) noexcept;

private:
	std::string backendURL;
	std::string srtURL;
	std::string streamKey;
	size_t nbVideoTracks;
	size_t nbAudioTracks;
	bool started;
};
