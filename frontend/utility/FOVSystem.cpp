/**
 * @file FOVSystem.cpp
 * @author The FOV Team
 * @brief Implementation of the FOVSystem class managing isolated multi-track video and audio encoders.
 * @version 1.0
 * @date 2026-02-14
 */

#include "FOVSystem.hpp"
#include "obs-data.h"
#include "obs-output.h"
#include "obs-source.h"
#include "obs.h"
#include "obs.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

/**
 * @brief Get the index of the first active mixer track from a mixer mask.
 * @param[in] mixerMask Bitmask representing active audio mixers.
 * @return size_t Index of the first active mixer track. Returns 0 if no bits are set.
 */
static size_t GetFirstMixerIndex(uint32_t mixerMask)
{
	for (size_t i = 0; i < 6; i++) {
		if (mixerMask & (1 << i)) {
			return i;
		}
	}
	return 0;
}

/**
 * @brief Apply new settings to the video encoder.
 * @param[in] videoSettings Pointer to the settings data object.
 */
void FOVSystem::VideoTrack::updateEncoderSettings(obs_data_t *videoSettings)
{
	obs_data_apply(encoderSettings, videoSettings);
	obs_encoder_update(encoder, encoderSettings);
}

/**
 * @brief Recreate the video context and view matching the current source properties.
 * @return true True on success.
 * @return false False if this->source is null, or if obs_view_add2 fails to allocate the video context.
 */
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

/**
 * @brief Change the encoder type used for this video track.
 * @param[in] encoderID Identifier string of the FOV encoder.
 * @return true True on success.
 * @return false False if encoderID matches the current type, or if obs_video_encoder_create fails to instantiate the encoder.
 */
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

/**
 * @brief Set the underlying FOV source for this video track.
 * @param[in] rawSource Pointer to the FOV source. Pass nullptr to unbind and clear the track.
 * @return true True on success.
 * @return false False if rawSource is null, or if refreshVideoSettings fails during execution.
 */
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

/**
 * @brief Construct a new VideoTrack object.
 * @param[in] rawSource Pointer to the FOV source.
 * @param[in] videoSettings Pointer to the settings data object.
 * @param[in] encoderid Identifier string of the FOV encoder.
 */
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

/**
 * @brief Apply new settings to the audio encoder.
 * @param[in] audioSettings Pointer to the settings data object.
 */
void FOVSystem::AudioTrack::updateEncoderSettings(obs_data_t *audioSettings)
{
	obs_data_apply(encoderSettings, audioSettings);
	obs_encoder_update(encoder, encoderSettings);
}

/**
 * @brief Recreate the audio encoder, matching the current source properties.
 * @return true True on success.
 * @return false False if this->source is null.
 */
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

/**
 * @brief Change the encoder type used for this audio track.
 * @param[in] encoderID Identifier string of the FOV encoder.
 * @return true True on success.
 * @return false False if encoderID matches the current type, or if obs_audio_encoder_create fails to instantiate the encoder.
 */
bool FOVSystem::AudioTrack::changeEncoderType(const std::string &encoderID)
{
	if (this->encoderID == encoderID)
		return false;

	if (encoder) {
		obs_encoder_set_audio(encoder, nullptr);
	}

	std::string encoderName = "FOV audio track " + std::string(obs_source_get_name(source));
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

/**
 * @brief Set the underlying FOV source for this audio track.
 * @param[in] rawSource Pointer to the FOV source. Pass nullptr to unbind and clear the track.
 * @return true True on success.
 * @return false False if rawSource is null.
 */
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

/**
 * @brief Construct a new AudioTrack object.
 * @param[in] rawSource Pointer to the FOV source.
 * @param[in] audioSettings Pointer to the settings data object.
 * @param[in] encoderid Identifier string of the FOV encoder.
 * @param[in] registeredMixes Count of already registered audio mixes. Used to programmatically assign the mixer ID bitmask.
 */
FOVSystem::AudioTrack::AudioTrack(obs_source_t *rawSource, obs_data_t *audioSettings, std::string encoderid,
				  int registeredMixes)
	: encoderSettings(obs_data_create()),
	  encoderID(encoderid)
{
	std::string encoderName = "FOV audio track " + std::string(obs_source_get_name(rawSource));

	size_t mixerIndex = 0;
	if (registeredMixes <= -1) {
		auto mixerMask = obs_source_get_audio_mixers(rawSource);
		mixerIndex = GetFirstMixerIndex(mixerMask);
	} else {
		obs_source_set_audio_mixers(rawSource, 1 << registeredMixes);
		mixerIndex = (size_t)registeredMixes;
	}

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

/**
 * @brief Construct a new FOVSystem object.
 */
FOVSystem::FOVSystem()
	: videoSettings(obs_data_create()),
	  audioSettings(obs_data_create()),
	  encoderGroup(obs_encoder_group_create())
{
}

FOVSystem::~FOVSystem()
{
	obs_data_release(videoSettings);
	obs_data_release(audioSettings);
}

/**
 * @brief Initialize the FOVSystem with a specific muxer output and default settings.
 * @param[in] ffmpegMpegtsMuxerOutput Pointer to the MPEG-TS muxer output.
 * @param[in] videoSettings Pointer to the settings data object.
 * @param[in] audioSettings Pointer to the settings data object.
 */
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

/**
 * @brief Add a new source to the FOVSystem and create its corresponding tracks.
 * @param[in] source Pointer to the FOV source.
 */
void FOVSystem::addSource(obs_source_t *source)
{
	if (!isInit || source == nullptr)
		return;

	if (obs_source_get_output_flags(source) & OBS_SOURCE_VIDEO) {
		videoTracks.emplace_back(std::make_unique<VideoTrack>(source, videoSettings, videoEncoderID));
	}
	if (obs_source_get_output_flags(source) & OBS_SOURCE_AUDIO) {
		audioTracks.emplace_back(
			std::make_unique<AudioTrack>(source, audioSettings, audioEncoderID, (int)audioTracks.size()));
	}
	updateEncoderGroup();
	updateServiceTracks();
}

/**
 * @brief Remove a source and its tracks from the FOVSystem.
 * @param[in] source Pointer to the FOV source.
 * @return true True on success.
 * @return false False if isInit is false, if source is null, or if the source is not found in the tracking deques.
 */
bool FOVSystem::removeSource(obs_source_t *source)
{
	if (!isInit || source == nullptr)
		return false;

	bool found = false;

	if (obs_source_get_output_flags(source) & OBS_SOURCE_VIDEO) {
		for (size_t i = 0; i < videoTracks.size(); i++) {
			if (videoTracks[i]->source == source) {
				videoTracks.erase(videoTracks.begin() + i);
				found = true;
				break;
			}
		}
	}

	if (obs_source_get_output_flags(source) & OBS_SOURCE_AUDIO) {
		for (size_t i = 0; i < audioTracks.size(); i++) {
			if (audioTracks[i]->source == source) {
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

/**
 * @brief Remove all active sources and tracks from the FOVSystem.
 */
void FOVSystem::clearSources()
{
	if (!isInit)
		return;
	videoTracks.clear();
	audioTracks.clear();
	updateEncoderGroup();
	updateServiceTracks();
}

/**
 * @brief Update settings and type for all active video encoders.
 * @param[in] encoderSettings Pointer to the settings data object.
 * @param[in] encoderID Identifier string of the FOV encoder.
 */
void FOVSystem::updateVideoEncoderSettings(obs_data_t *encoderSettings, const std::string &encoderID)
{
	if (encoderSettings == nullptr) {
		return;
	}

	obs_data_apply(this->videoSettings, encoderSettings);
	for (auto &i : videoTracks) {
		if (encoderID != this->videoEncoderID) {
			i->changeEncoderType(encoderID);
		}
		i->updateEncoderSettings(encoderSettings);
		i->refreshVideoSettings();
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

/**
 * @brief Update settings and type for all active audio encoders.
 * @param[in] encoderSettings Pointer to the settings data object.
 * @param[in] encoderID Identifier string of the FOV encoder.
 */
void FOVSystem::updateAudioEncoderSettings(obs_data_t *encoderSettings, const std::string &encoderID)
{
	if (encoderSettings == nullptr) {
		return;
	}

	obs_data_apply(this->audioSettings, encoderSettings);
	for (auto &i : audioTracks) {
		if (encoderID != this->audioEncoderID) {
			i->changeEncoderType(encoderID);
		}
		i->updateEncoderSettings(encoderSettings);
		i->refreshAudioSettings();
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

/**
 * @brief Rebuild the encoder group and bind active track encoders to the output muxer.
 */
void FOVSystem::updateEncoderGroup()
{
	if (!isInit)
		return;

	size_t i = 0;
	OBSEncoderGroup newGroup = obs_encoder_group_create();

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

	encoderGroup = std::move(newGroup);
}

/**
 * @brief Update the track counts in the active service configuration settings.
 */
void FOVSystem::updateServiceTracks()
{
	if (!isInit) {
		return;
	}

	obs_service_t *service = obs_output_get_service(ffmpegMpegtsMuxerOutput);
	if (!service) {
		return;
	}

	OBSDataAutoRelease data = obs_service_get_settings(service);
	if (!data) {
		return;
	}

	obs_data_set_int(data, "video_encoder_count", static_cast<long long>(videoTracks.size()));
	obs_data_set_int(data, "audio_track_count", static_cast<long long>(audioTracks.size()));

	OBSDataArrayAutoRelease videoArray = obs_data_array_create();
	for (const auto &track : videoTracks) {
		if (track && track->source) {
			const char *sourceName = obs_source_get_name(track->source);
			OBSDataAutoRelease item = obs_data_create();
			obs_data_set_string(item, "name", sourceName ? sourceName : "");
			obs_data_array_push_back(videoArray, item);
		}
	}
	obs_data_set_array(data, "videoTrackNames", videoArray);

	OBSDataArrayAutoRelease audioArray = obs_data_array_create();
	for (const auto &track : audioTracks) {
		if (track && track->source) {
			const char *sourceName = obs_source_get_name(track->source);
			OBSDataAutoRelease item = obs_data_create();
			obs_data_set_string(item, "name", sourceName ? sourceName : "");
			obs_data_array_push_back(audioArray, item);
		}
	}
	obs_data_set_array(data, "audioTrackNames", audioArray);

	obs_service_update(service, data);
}

/**
 * @brief Synchronize active sources by scanning all available FOV audio and video sources.
 */
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
		if (flags & OBS_SOURCE_VIDEO) {
			if (obs_source_showing(source) && obs_source_active(source) && !obs_source_is_hidden(source))
				validSources.insert(source);
		}
		if (flags & OBS_SOURCE_AUDIO) {
			if (obs_source_active(source)) {
				validSources.insert(source);
			}
		}
	}

	for (obs_source_t *source : validSources) {
		this->addSource(source);
	}

	updateEncoderGroup();
	updateServiceTracks();
}
