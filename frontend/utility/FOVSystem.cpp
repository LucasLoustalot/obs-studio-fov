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
    if (!this->source) return false;

    obs_get_video_info(&ovi);
    ovi.output_width = OUT_ALIGN(obs_source_get_width(this->source), 16);
    ovi.output_height = OUT_ALIGN(obs_source_get_height(this->source), 16);
    ovi.base_width = OUT_ALIGN(obs_source_get_base_width(this->source), 16);
    ovi.base_height = OUT_ALIGN(obs_source_get_base_height(this->source), 16);

    if (ovi.fps_num == 0) ovi.fps_num = 30;
    if (ovi.fps_den == 0) ovi.fps_den = 1;
    if (ovi.colorspace == VIDEO_CS_DEFAULT) ovi.colorspace = VIDEO_CS_709;

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
        blog(LOG_ERROR, "FOV failed to create mix for %s",  obs_source_get_name(this->source));
        return false;
    }

    obs_encoder_set_video(encoder, videoContext);
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
	: view(obs_view_create()), encoderSettings(obs_data_create()), encoderID(encoderid)
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

FOVSystem::FOVSystem() : videoSettings(obs_data_create()), encoderGroup(obs_encoder_group_create())
{
	return;
}

FOVSystem::~FOVSystem()
{
}

void FOVSystem::initSystem(obs_encoder_t *audioEncoder, obs_output_t *ffmpegMpegtsMuxerOutput, obs_data_t *encoderSettings)
{
	if (isInit) return;

	if (audioEncoder == nullptr || ffmpegMpegtsMuxerOutput == nullptr) {
		blog(LOG_ERROR, "FOV failed to init: missing audio encoder or output");
		return;
	}
	this->audioEncoder = audioEncoder;
	this->ffmpegMpegtsMuxerOutput = ffmpegMpegtsMuxerOutput;
	if (encoderSettings != nullptr) {
		obs_data_apply(this->videoSettings, encoderSettings);
	}
	isInit = true;
}

void FOVSystem::addSource(obs_source_t *source)
{
	if (!isInit) return;
	if (source == nullptr) {
		return;
	}

	videoTracks.emplace_back(std::make_unique<VideoTrack>(source, videoSettings, encoderID));
	updateEncoderGroup();
	updateServiceTracks();
}

bool FOVSystem::removeSource(obs_source_t *source)
{
	if (!isInit) return false;
	bool found = false;

	if (source == nullptr) {
		return false;
	}

	for (size_t i = 0; i < videoTracks.size(); i++) {
		if (videoTracks[i].get()->source == source) {
			videoTracks.erase(videoTracks.begin() + i);
			found = true;
			break;
		}
	}
	updateEncoderGroup();
	updateServiceTracks();
	return found;
}

void FOVSystem::clearSources()
{
	if (!isInit) return;
	videoTracks.clear();
	updateEncoderGroup();
	updateServiceTracks();
}

void FOVSystem::updateEncoderSettings(obs_data_t *encoderSettings)
{
	if (encoderSettings == nullptr) {
		return;
	}
	if (ffmpegMpegtsMuxerOutput) {
		auto service = obs_output_get_service(ffmpegMpegtsMuxerOutput);
		if (service) {
			obs_service_apply_encoder_settings(service, this->videoSettings, nullptr);
		}
	}

	obs_data_apply(this->videoSettings, encoderSettings);
	for (auto &i : videoTracks)
	{
		i.get()->updateEncoderSettings(encoderSettings);
		i.get()->refreshVideoSettings();
	}
	updateEncoderGroup();
}

void FOVSystem::updateEncoderGroup()
{
	if (!isInit) return;

	size_t i = 0;
    obs_encoder_group_t *newGroup = obs_encoder_group_create();

    if (audioEncoder) {
        obs_encoder_set_group(audioEncoder, newGroup);
    }
	if (ffmpegMpegtsMuxerOutput != nullptr) {
		obs_output_set_video_encoder(ffmpegMpegtsMuxerOutput, nullptr);
		for (size_t j = 0; j < MAX_OUTPUT_VIDEO_ENCODERS; j++) {
            obs_output_set_video_encoder2(ffmpegMpegtsMuxerOutput, nullptr, j);
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

    encoderGroup = newGroup;
}

void FOVSystem::updateServiceTracks()
{
	if (!isInit) return;
	auto service = obs_output_get_service(ffmpegMpegtsMuxerOutput);
	if (service != nullptr) {
		OBSDataAutoRelease data = obs_service_get_settings(service);
		obs_data_set_int(data, "video_encoder_count", (long long) videoTracks.size());
		obs_service_update(service, data);
	}
}

void FOVSystem::syncSources()
{
	if (!isInit) return;
    std::set<obs_source_t*> activeOBSSources;

    obs_enum_sources(
		[](void *data, obs_source_t *source) {
			auto* set = static_cast<std::set<obs_source_t*>*>(data);
			if (obs_source_get_output_flags(source) & OBS_SOURCE_VIDEO && obs_source_active(source)) {
				set->insert(source);
			}
			return true;
    	},
		&activeOBSSources);

    auto it = videoTracks.begin();
    while (it != videoTracks.end()) {
        if (activeOBSSources.find((*it)->source) == activeOBSSources.end()) {
            it = videoTracks.erase(it);
        } else {
            activeOBSSources.erase((*it)->source);
            ++it;
        }
    }

    for (obs_source_t* newSource : activeOBSSources) {
        this->addSource(newSource);
    }

    updateEncoderGroup();
    updateServiceTracks();
}
