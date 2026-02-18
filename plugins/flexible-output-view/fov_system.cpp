/**
 * @file fov_system.cpp
 * @author The FOV Team
 * @brief The FOVSystem class implementation
 * @version 0.1
 * @date 2026-02-14
 *
 */

#include "fov.hpp"
#include "obs-data.h"
#include "obs-source.h"
#include "obs.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

FOVSystem::FOVSystem() : hasInit(false), started(false) {}

void FOVSystem::init()
{
	std::unique_lock<std::mutex> lock(mutex);
	debug("--- FOVSystem init ---");

	if (hasInit) {
		return;
	}

	fovOutput.reset(obs_output_create("fov_output", "FOV multitrack stream output", nullptr, nullptr));
	if (!fovOutput) {
		throw std::runtime_error("FOVSystem: Failed to allocate output");
	}
	encoderGroup.reset(obs_encoder_group_create());
	if (!encoderGroup) {
		throw std::runtime_error("FOVSystem: Failed to allocate encoder group");
	}
	fovService.reset(obs_service_create("fov_service", "multitrack video service", nullptr, nullptr));
	if (!fovService) {
		throw std::runtime_error("FOVSystem: Failed to allocate service");
	}

	obs_output_set_service(fovOutput.get(), fovService.get());
	hasInit = true;
}

void FOVSystem::setVideoSettings(const VideoSettings &settings)
{
	std::unique_lock<std::mutex> lock(mutex);
	videoSettings = settings;
}

void FOVSystem::setAudioSettings(const AudioSettings &settings)
{
	std::unique_lock<std::mutex> lock(mutex);
	audioSettings = settings;
}

bool FOVSystem::isStarted() const
{
	return started;
}

bool FOVSystem::isInit() const
{
	return hasInit;
}

void FOVSystem::addSource(obs_source_t *source)
{
	std::unique_lock<std::mutex> lock(mutex);

	if (started) {
		throw std::runtime_error("FOVSystem: cannot add new sources while output is started !");
	}
	if (!hasInit) {
		throw std::runtime_error("FOVSystem: cannot bind source before init !");
	}

	if (obs_source_get_type(source) != OBS_SOURCE_TYPE_INPUT) {
		return;
	}

	uint32_t sourceFlags = obs_source_get_output_flags(source);
	if (sourceFlags & OBS_SOURCE_VIDEO) {
		OBSDataPtr encoderSettings(obs_encoder_defaults(videoSettings.OBSEncoderID.c_str()));
		if (!encoderSettings.get()) {
			throw std::runtime_error("FOVSystem: failed to allocate memory for video encoder settings !");
		}

		videoTracks.push_back(std::make_unique<VideoTrack>(source, encoderSettings, videoSettings));
		if (!videoTracks.back()->encoder.get()) {
			throw std::runtime_error("FOVSystem: failed to allocate video encoder !");
		}

	} else if (sourceFlags & OBS_SOURCE_AUDIO) {
		OBSDataPtr encoderSettings(obs_encoder_defaults(audioSettings.OBSEncoderID.c_str()));
		if (!encoderSettings.get()) {
			throw std::runtime_error("FOVSystem: failed to allocate memory for audio encoder settings !");
		}

		if (!audioEncoder) {
			audioEncoder.reset(obs_audio_encoder_create(audioSettings.OBSEncoderID.c_str(),
								    "FOV audio track", encoderSettings.get(), 0,
								    nullptr));
		}
		if (!audioEncoder) {
			throw std::runtime_error("FOVSystem: failed to allocate audio encoder !");
		}
		obs_encoder_set_audio(audioEncoder.get(), obs_get_audio());
		obs_output_set_audio_encoder(fovOutput.get(), audioEncoder.get(), 0);

		// TODO: handle multiple audio
	}
}

void FOVSystem::start()
{
	std::unique_lock<std::mutex> lock(mutex);

	if (!hasInit) {
		throw std::runtime_error("FOVSystem: was not init !");
	}
	if (started) {
		return;
	}
	if (videoTracks.empty()) {
		throw std::runtime_error("FOVSystem: require at least 1 video source to work !");
	}
	//if (SRTURL.empty() || backendURL.empty()) {
	//	throw std::runtime_error("FOVSystem: the URL was not set for FOVSystem !");
	//}

	// Set fov_output settings
	OBSDataPtr outputSettings(obs_data_create());
	if (!outputSettings) {
		throw std::runtime_error("FOVSystem: cannot allocate output settings !");
	}
	obs_data_set_string(outputSettings.get(), "url", SRTURL.c_str());
	obs_data_set_string(outputSettings.get(), "path", SRTURL.c_str());
	obs_data_set_int(outputSettings.get(), "video_track_count", videoTracks.size());
	obs_output_update(fovOutput.get(), outputSettings.get());

	OBSDataPtr serviceSettings(obs_data_create());
	obs_data_set_string(serviceSettings.get(), "server", backendURL.c_str());
	obs_data_set_string(serviceSettings.get(), "srt_endpoint", SRTURL.c_str());
	obs_data_set_int(serviceSettings.get(), "video_encoder_count", videoTracks.size());
	obs_service_update(fovService.get(), serviceSettings.get());

	size_t i = 0;
	for (const auto &track : videoTracks) {
		obs_encoder_set_video(track->encoder.get(), track->videoContext);
		obs_encoder_set_group(track->encoder.get(), encoderGroup.get());
		obs_output_set_video_encoder2(fovOutput.get(), track->encoder.get(), i);
		i++;
	}
	if (audioEncoder) {
		obs_encoder_set_group(audioEncoder.get(), encoderGroup.get());
	}

	obs_output_set_reconnect_settings(fovOutput.get(), 10, 10);
	obs_output_initialize_encoders(fovOutput.get(), 0);

	debug("FOVSystem: Starting output...");

	if (obs_output_start(fovOutput.get())) {
		if (obs_output_begin_data_capture(fovOutput.get(), OBS_OUTPUT_MULTI_TRACK_VIDEO | OBS_OUTPUT_AUDIO)) {
			started = true;
		} else {
			const char *error = obs_output_get_last_error(fovOutput.get());
			debug("FOVSystem: failed init data capture: %s", error);
			started = false;
		}
	} else {
		const char *error = obs_output_get_last_error(fovOutput.get());
		debug("FOVSystem: failed to start output: %s", error);
		started = false;
	}
}

void FOVSystem::stop()
{
	std::unique_lock<std::mutex> lock(mutex);

	if (!started || !hasInit) {
		return;
	}

	debug("FOVSystem: Stopping output...");

	if (fovOutput) {
		if (audioEncoder) {
			obs_encoder_set_audio(audioEncoder.get(), nullptr);
		}
		for (size_t i = 0; i < videoTracks.size(); i++) {
			obs_output_set_video_encoder2(fovOutput.get(), nullptr, i);
		}

		obs_output_end_data_capture(fovOutput.get());
		obs_output_stop(fovOutput.get());
		obs_output_force_stop(fovOutput.get());
	}

	for (auto &track : videoTracks) {
		if (track->encoder) {
			obs_encoder_set_video(track->encoder.get(), nullptr);
		}
	}

	started = false;
}

void FOVSystem::setBackendURL(const std::string &url)
{
	std::unique_lock<std::mutex> lock(mutex);

	backendURL = url;
}

void FOVSystem::setSRTURL(const std::string &url)
{
	std::unique_lock<std::mutex> lock(mutex);

	SRTURL = url;
}

FOVSystem::~FOVSystem()
{
	stop();
	videoTracks.clear();
	encoderGroup.reset();
}
