/**
 * @file FOVSystem.hpp
 * @author The FOV Team
 * @brief The FOV System
 * @version 0.1
 * @date 2026-05-03
 */

#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <deque>
#include <set>
#include <obs.hpp>

#include "media-io/video-io.h"
#include "obs.h"

// Output resolution alignment
#define OUT_ALIGN(x, a) (((x)+(a)-1)&~((a)-1))

class FOVSystem {

public:

	explicit FOVSystem();
	FOVSystem(FOVSystem &) = delete;
	FOVSystem &operator=(FOVSystem &) = delete;

	~FOVSystem();

	void addSource(obs_source_t *source);
    bool removeSource(obs_source_t *source);
    void clearSources();

    void initSystem(obs_encoder_t *audioEncoder, obs_output_t *ffmpegMpegtsMuxerOutput, obs_data_t *encoderSettings = nullptr);

    void updateEncoderSettings(obs_data_t *encoderSettings, const std::string &encoderID = "obs_x264");
    void syncSources();

protected:
	struct VideoTrack {
		std::string encoderID;

		video_t *videoContext = nullptr;

		OBSSource source;
		OBSView view;
		OBSEncoder encoder;
        OBSData encoderSettings;
        struct obs_video_info ovi {0};

		VideoTrack(obs_source_t *rawSource, obs_data_t *videoSettings, std::string encoderID);
		~VideoTrack();

        void updateEncoderSettings(obs_data_t *videoSettings);
        bool setSource(obs_source_t *source);
        bool refreshVideoSettings();
		bool changeEncoderType(const std::string &encoderID);
	};

private:
	bool isInit = false;
	std::string encoderID = "obs_x264";

	obs_encoder_t *audioEncoder = nullptr;
	obs_output_t *ffmpegMpegtsMuxerOutput = nullptr;

    obs_data_t *videoSettings;

	OBSEncoderGroup encoderGroup;
	std::deque<std::unique_ptr<VideoTrack>> videoTracks;

    void updateServiceTracks();
    void updateEncoderGroup();
};

inline bool checkIsFOV(const obs_service_t *service)
{
	if (service == nullptr) {
		return false;
	}
	return (strcmp(obs_service_get_id(service), "fov_service") == 0);
}
