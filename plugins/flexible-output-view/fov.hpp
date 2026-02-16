/**
 * @file fov.hpp
 * @author The FOV Team
 * @brief Definition of the FOV Output
 * The FOV output allows for a multi-track video stream over SRT or RIST
 * @version 0.1
 * @date 2026-01-01
*/

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "media-io/video-io.h"
#include "obs.h"

#define debug(format, ...) blog(LOG_DEBUG, "FOV: " format, ##__VA_ARGS__)
#define info(format, ...) blog(LOG_INFO, "FOV: " format, ##__VA_ARGS__)
#define warn(format, ...) blog(LOG_WARNING, "FOV: " format, ##__VA_ARGS__)

// Output resolution alignment
#define OUT_ALIGN(x, a) (((x)+(a)-1)&~((a)-1))

template<typename T, void (*Deleter)(T *)>
using OBSPtr = std::unique_ptr<T, std::integral_constant<decltype(Deleter), Deleter>>;

using OBSSourcePtr = OBSPtr<obs_source_t, obs_source_release>;
using OBSServicePtr = OBSPtr<obs_service_t, obs_service_release>;
using OBSOutputPtr = OBSPtr<obs_output_t, obs_output_release>;
using OBSEncoderPtr = OBSPtr<obs_encoder_t, obs_encoder_release>;
using OBSDataPtr = OBSPtr<obs_data_t, obs_data_release>;
using OBSViewPtr = OBSPtr<obs_view_t, obs_view_destroy>;
using OBSEncoderGroupPtr = OBSPtr<obs_encoder_group_t, obs_encoder_group_destroy>;
using OBSVideoPtr = OBSPtr<video_t, video_output_close>;

class FOVSystem {

public:
	struct VideoSettings {
		std::string OBSEncoderID;
		uint32_t framerate;
	};

	struct AudioSettings {
		std::string OBSEncoderID;
	};

	explicit FOVSystem();
	FOVSystem(FOVSystem &) = delete;
	FOVSystem &operator=(FOVSystem &) = delete;

	~FOVSystem();

	void init();

	void start();
	void stop();

	bool isStarted() const;
	bool isInit() const;

	void setVideoSettings(const VideoSettings &settings);
	void setAudioSettings(const AudioSettings &settings);

	void setBackendURL(const std::string &backendURL);
	void setSRTURL(const std::string &srtURL);

	void addSource(obs_source_t *source);

protected:
	struct VideoTrack {
		OBSSourcePtr source;
		obs_view_t *view;
		video_t *videoContext;
		OBSEncoderPtr encoder;

		VideoTrack(obs_source_t *rawSource, const OBSDataPtr &encoderSettings,
			   const VideoSettings &videoSettings)
		{
			std::string encoderName = "FOV video track " + std::string(obs_source_get_name(rawSource));
			struct obs_video_info ovi{0};

			// Video settings
			obs_get_video_info(&ovi);
			ovi.output_width = OUT_ALIGN(obs_source_get_width(rawSource), 16);
			ovi.output_height = OUT_ALIGN(obs_source_get_height(rawSource), 16);
			ovi.base_width = OUT_ALIGN(obs_source_get_base_width(rawSource), 16);
			ovi.base_height = OUT_ALIGN(obs_source_get_base_height(rawSource), 16);
			ovi.fps_den = 1;
			ovi.fps_num = videoSettings.framerate;
			ovi.colorspace = VIDEO_CS_DEFAULT;
			ovi.range = VIDEO_RANGE_DEFAULT;

			// Owning the source and view
			source.reset(obs_source_get_ref(rawSource));
			view = obs_view_create();

			// Creating the encoder
			encoder.reset(obs_video_encoder_create(videoSettings.OBSEncoderID.c_str(), encoderName.c_str(),
							       encoderSettings.get(), nullptr));

			// Creating the dedicated pipeline (separate render thread)
			videoContext = obs_view_add2(view, &ovi);

			obs_view_set_source(view, 0, source.get());
			obs_encoder_set_video(encoder.get(), videoContext);
		}

		~VideoTrack()
		{
			if (encoder) {
				obs_encoder_set_video(encoder.get(), nullptr);
			}
			if (view) {
				obs_view_destroy(view);
			}
		}
	};

private:
    std::mutex mutex;

	bool hasInit;
	bool started;

	OBSOutputPtr fovOutput;
	OBSServicePtr fovService;
	OBSEncoderGroupPtr encoderGroup;

	std::vector<std::unique_ptr<VideoTrack>> videoTracks;
	OBSEncoderPtr audioEncoder;

	VideoSettings videoSettings;
	AudioSettings audioSettings;

	std::string backendURL;
	std::string SRTURL;
};
