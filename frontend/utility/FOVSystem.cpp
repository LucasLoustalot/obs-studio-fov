/**
 * @file FOVSystem.cpp
 * @author The FOV Team
 * @brief The FOVSystem implementation
 * @version 0.1
 * @date 2026-02-14
 *
 */

#include "FOVSystem.hpp"
#include "obs-data.h"
#include "obs-output.h"
#include "obs-source.h"
#include "obs.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

void FOVSystem::VideoTrack::updateEncoderSettings(obs_data_t *videoSettings)
{
	obs_data_apply(encoderSettings, videoSettings);
	obs_encoder_update(encoder, encoderSettings);
}

bool FOVSystem::VideoTrack::refreshVideoSettings()
{
	if (!this->source)
		return false;

	obs_get_video_info(&ovi);
	ovi.output_width = OUT_ALIGN(obs_source_get_width(this->source), 16);
	ovi.output_height = OUT_ALIGN(obs_source_get_height(this->source), 16);
	ovi.base_width = OUT_ALIGN(obs_source_get_base_width(this->source), 16);
	ovi.base_height = OUT_ALIGN(obs_source_get_base_height(this->source), 16);

	if (ovi.fps_num == 0)
		ovi.fps_num = 30;
	if (ovi.fps_den == 0)
		ovi.fps_den = 1;
	if (ovi.colorspace == VIDEO_CS_DEFAULT)
		ovi.colorspace = VIDEO_CS_709;

	if (obs_encoder_video(encoder) != nullptr) {
		blog(LOG_INFO, "FOV: encoder '%s' has video set, recreating", obs_encoder_get_name(encoder));
		std::string encoderName = obs_encoder_get_name(encoder);
		obs_encoder_release(encoder);
		encoder = obs_video_encoder_create(encoderID.c_str(), encoderName.c_str(), encoderSettings, nullptr);
		if (videoContext) {
			obs_view_remove(view);
			videoContext = nullptr;
		}
	} else {
		obs_encoder_set_video(encoder, nullptr);
		if (videoContext) {
			obs_view_remove(view);
			videoContext = nullptr;
		}
	}

	obs_view_set_source(view, 0, this->source);
	videoContext = obs_view_add2(view, &ovi);

	if (!videoContext) {
		blog(LOG_ERROR, "FOV failed to create mix for %s", obs_source_get_name(this->source));
		return false;
	}

	obs_encoder_set_video(encoder, videoContext);
	return true;
}

bool FOVSystem::VideoTrack::changeEncoderType(const std::string &encoderID)
{
	if (this->encoderID == encoderID)
		return false;

	if (encoder) {
		obs_encoder_set_video(encoder, nullptr);
	}

	std::string encoderName = obs_encoder_get_name(encoder);
	this->encoderID = encoderID;

	obs_encoder_release(encoder);

	encoder = obs_video_encoder_create(this->encoderID.c_str(), encoderName.c_str(), encoderSettings, nullptr);

	if (!encoder) {
		blog(LOG_ERROR, "FOV: Failed to create new encoder of type %s", encoderID.c_str());
		return false;
	}

	if (videoContext) {
		obs_encoder_set_video(encoder, videoContext);
	}

	blog(LOG_INFO, "FOV: Changed encoder type to %s for track %s", encoderID.c_str(), encoderName.c_str());

	return true;
}

bool FOVSystem::VideoTrack::setSource(obs_source_t *rawSource)
{
	if (!rawSource) {
		if (videoContext) {
			obs_view_remove(view);
			obs_view_set_source(view, 0, nullptr);
		}
		obs_encoder_set_video(encoder, obs_get_video());
		this->videoContext = nullptr;
		this->source = nullptr;
		return false;
	}

	this->source = rawSource;

	refreshVideoSettings();
	return true;
}

FOVSystem::VideoTrack::VideoTrack(obs_source_t *rawSource, obs_data_t *videoSettings, std::string encoderid)
	: view(obs_view_create()),
	  encoderSettings(obs_data_create()),
	  encoderID(encoderid)
{
	std::string encoderName = "FOV video track " + std::string(obs_source_get_name(rawSource));

	encoder = obs_video_encoder_create(encoderID.c_str(), encoderName.c_str(), videoSettings, nullptr);
	setSource(rawSource);
	updateEncoderSettings(videoSettings);
}

FOVSystem::VideoTrack::~VideoTrack()
{
	if (encoder) {
		obs_encoder_set_video(encoder, nullptr);
	}
	if (view) {
		obs_view_set_source(view, 0, nullptr);
		if (videoContext) {
			obs_view_remove(view);
			videoContext = nullptr;
		}
	}
}

void FOVSystem::AudioTrack::updateEncoderSettings(obs_data_t *videoSettings)
{
	obs_data_apply(encoderSettings, videoSettings);
	obs_encoder_update(encoder, encoderSettings);
}

bool FOVSystem::AudioTrack::refreshAudioSettings()
{
	if (!this->source)
		return false;

	if (obs_encoder_audio(encoder) != nullptr) {
		blog(LOG_INFO, "FOV: encoder '%s' has audio set, recreating", obs_encoder_get_name(encoder));
		std::string encoderName = obs_encoder_get_name(encoder);
		obs_encoder_release(encoder);

		auto mixerMask = obs_source_get_audio_mixers(source);
		size_t mixerIndex = GetFirstMixerIndex(mixerMask);

		encoder = obs_audio_encoder_create(encoderID.c_str(), encoderName.c_str(), encoderSettings, mixerIndex,
						   nullptr);
	}

	obs_encoder_set_audio(encoder, obs_get_audio());

	return true;
}

bool FOVSystem::AudioTrack::changeEncoderType(const std::string &encoderID)
{
	if (this->encoderID == encoderID)
		return false;

	if (encoder) {
		obs_encoder_set_audio(encoder, nullptr);
	}

	std::string encoderName = obs_encoder_get_name(encoder);
	this->encoderID = encoderID;

	obs_encoder_release(encoder);

	auto mixerMask = obs_source_get_audio_mixers(source);
	size_t mixerIndex = GetFirstMixerIndex(mixerMask);

	encoder = obs_audio_encoder_create(this->encoderID.c_str(), encoderName.c_str(), encoderSettings, mixerIndex,
					   nullptr);

	if (!encoder) {
		blog(LOG_ERROR, "FOV: Failed to create new encoder of type %s", encoderID.c_str());
		return false;
	}

	obs_encoder_set_audio(encoder, obs_get_audio());

	blog(LOG_INFO, "FOV: Changed encoder type to %s for track %s", encoderID.c_str(), encoderName.c_str());

	return true;
}

bool FOVSystem::AudioTrack::setSource(obs_source_t *rawSource)
{
	if (!rawSource) {
		obs_encoder_set_audio(encoder, obs_get_audio());
		this->source = nullptr;
		return false;
	}

	this->source = rawSource;

	refreshAudioSettings();
	return true;
}

FOVSystem::AudioTrack::AudioTrack(obs_source_t *rawSource, obs_data_t *audioSettings, std::string encoderid)
	: encoderSettings(obs_data_create()),
	  encoderID(encoderid)
{
	std::string encoderName = "FOV audio track " + std::string(obs_source_get_name(rawSource));

	auto mixerMask = obs_source_get_audio_mixers(rawSource);
	size_t mixerIndex = GetFirstMixerIndex(mixerMask);

	encoder = obs_audio_encoder_create(encoderID.c_str(), encoderName.c_str(), audioSettings, mixerIndex, nullptr);

	setSource(rawSource);
	updateEncoderSettings(audioSettings);
}

FOVSystem::AudioTrack::~AudioTrack()
{
	if (encoder) {
		obs_encoder_set_audio(encoder, nullptr);
	}
}

FOVSystem::FOVSystem()
	: videoSettings(obs_data_create()),
	  audioSettings(obs_data_create()),
	  encoderGroup(obs_encoder_group_create())
{
	return;
}

FOVSystem::~FOVSystem() {}

void FOVSystem::initSystem(obs_output_t *ffmpegMpegtsMuxerOutput, obs_data_t *videoSettings, obs_data_t *audioSettings)
{
	if (isInit)
		return;

	if (ffmpegMpegtsMuxerOutput == nullptr) {
		blog(LOG_ERROR, "FOV failed to init: missing output");
		return;
	}
	this->ffmpegMpegtsMuxerOutput = ffmpegMpegtsMuxerOutput;
	if (videoSettings != nullptr) {
		obs_data_apply(this->videoSettings, videoSettings);
	}
	if (audioSettings != nullptr) {
		obs_data_apply(this->audioSettings, audioSettings);
	}
	isInit = true;
}

void FOVSystem::addSource(obs_source_t *source)
{
	if (!isInit)
		return;
	if (source == nullptr) {
		return;
	}
	if (obs_source_get_output_flags(source) & OBS_SOURCE_VIDEO) {
		videoTracks.emplace_back(std::make_unique<VideoTrack>(source, videoSettings, videoEncoderID));
	}
	if (obs_source_get_output_flags(source) & OBS_SOURCE_AUDIO) {
		audioTracks.emplace_back(std::make_unique<AudioTrack>(source, audioSettings, audioEncoderID));
	}
	updateEncoderGroup();
	updateServiceTracks();
}

bool FOVSystem::removeSource(obs_source_t *source)
{
	if (!isInit)
		return false;
	bool found = false;

	if (source == nullptr) {
		return false;
	}

	if (obs_source_get_output_flags(source) & OBS_SOURCE_VIDEO) {
		for (size_t i = 0; i < videoTracks.size(); i++) {
			if (videoTracks[i].get()->source == source) {
				videoTracks.erase(videoTracks.begin() + i);
				found = true;
				break;
			}
		}
	}

	if (obs_source_get_output_flags(source) & OBS_SOURCE_AUDIO) {
		for (size_t i = 0; i < audioTracks.size(); i++) {
			if (audioTracks[i].get()->source == source) {
				audioTracks.erase(audioTracks.begin() + i);
				found = true;
				break;
			}
		}
	}

	updateEncoderGroup();
	updateServiceTracks();
	return found;
}

void FOVSystem::clearSources()
{
	if (!isInit)
		return;
	videoTracks.clear();
	audioTracks.clear();
	updateEncoderGroup();
	updateServiceTracks();
}

void FOVSystem::updateVideoEncoderSettings(obs_data_t *encoderSettings, const std::string &encoderID)
{
	if (encoderSettings == nullptr) {
		return;
	}

	obs_data_apply(this->videoSettings, encoderSettings);
	for (auto &i : videoTracks) {
		if (encoderID != this->videoEncoderID) {
			i.get()->changeEncoderType(encoderID);
		}
		i.get()->updateEncoderSettings(encoderSettings);
		i.get()->refreshVideoSettings();
	}
	this->videoEncoderID = encoderID;

	if (ffmpegMpegtsMuxerOutput) {
		auto service = obs_output_get_service(ffmpegMpegtsMuxerOutput);
		if (service) {
			obs_service_apply_encoder_settings(service, this->videoSettings, this->audioSettings);
		}
	}

	updateEncoderGroup();
}

void FOVSystem::updateAudioEncoderSettings(obs_data_t *encoderSettings, const std::string &encoderID)
{
	if (encoderSettings == nullptr) {
		return;
	}

	obs_data_apply(this->audioSettings, encoderSettings);
	for (auto &i : audioTracks) {
		if (encoderID != this->audioEncoderID) {
			i.get()->changeEncoderType(encoderID);
		}
		i.get()->updateEncoderSettings(encoderSettings);
		i.get()->refreshAudioSettings();
	}
	this->audioEncoderID = encoderID;

	if (ffmpegMpegtsMuxerOutput) {
		auto service = obs_output_get_service(ffmpegMpegtsMuxerOutput);
		if (service) {
			obs_service_apply_encoder_settings(service, this->videoSettings, this->audioSettings);
		}
	}

	updateEncoderGroup();
}

void FOVSystem::updateEncoderGroup()
{
	if (!isInit)
		return;

	size_t i = 0;
	obs_encoder_group_t *newGroup = obs_encoder_group_create();

	if (ffmpegMpegtsMuxerOutput != nullptr) {
		obs_output_set_video_encoder(ffmpegMpegtsMuxerOutput, nullptr);
		for (size_t j = 0; j < MAX_OUTPUT_VIDEO_ENCODERS; j++) {
			obs_output_set_video_encoder2(ffmpegMpegtsMuxerOutput, nullptr, j);
		}
		for (size_t j = 0; j < MAX_OUTPUT_AUDIO_ENCODERS; j++) {
			obs_output_set_audio_encoder(ffmpegMpegtsMuxerOutput, nullptr, j);
		}
	}

	for (const auto &track : videoTracks) {
		if (track->encoder) {
			obs_encoder_set_group(track->encoder, newGroup);
			if (ffmpegMpegtsMuxerOutput != nullptr) {
				obs_output_set_video_encoder2(ffmpegMpegtsMuxerOutput, track->encoder, i);
				i++;
			}
		}
	}

	i = 0;
	for (const auto &track : audioTracks) {
		if (track->encoder) {
			obs_encoder_set_group(track->encoder, newGroup);
			if (ffmpegMpegtsMuxerOutput != nullptr) {
				obs_output_set_audio_encoder(ffmpegMpegtsMuxerOutput, track->encoder, i);
				i++;
			}
		}
	}

	encoderGroup = newGroup;
}

void FOVSystem::updateServiceTracks()
{
	if (!isInit)
		return;
	auto service = obs_output_get_service(ffmpegMpegtsMuxerOutput);
	if (service != nullptr) {
		OBSDataAutoRelease data = obs_service_get_settings(service);
		obs_data_set_int(data, "video_encoder_count", (long long)videoTracks.size());
		obs_data_set_int(data, "audio_track_count", (long long)audioTracks.size());
		obs_service_update(service, data);
	}
}

void FOVSystem::syncSources()
{
	if (!isInit)
		return;
	clearSources();

	std::set<obs_source_t *> currentObsSources;
	obs_enum_sources(
		[](void *data, obs_source_t *source) {
			auto *set = static_cast<std::set<obs_source_t *> *>(data);
			set->insert(source);
			return true;
		},
		&currentObsSources);

	std::set<obs_source_t *> validSources;
	for (auto source : currentObsSources) {
		uint32_t flags = obs_source_get_output_flags(source);
		if (flags & OBS_SOURCE_VIDEO || flags & OBS_SOURCE_AUDIO) {
			validSources.insert(source);
		}
	}

	for (obs_source_t *source : validSources) {
		this->addSource(source);
	}

	updateEncoderGroup();
	updateServiceTracks();
}
