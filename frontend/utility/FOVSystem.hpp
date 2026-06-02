/**
 * @file FOVSystem.hpp
 * @author The FOV Team
 * @brief Header file for the FOVSystem class managing isolated multi-track video and audio encoders.
 * @version 1.0
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

/**
 * @brief Align rendering dimensions to the nearest bitwise boundary.
 */
#define OUT_ALIGN(x, a) (((x)+(a)-1)&~((a)-1))

/**
 * @class FOVSystem
 * @brief Class managing isolated multi-track video and audio encoders.
 */
class FOVSystem {

public:
	/**
     * @brief Construct a new FOVSystem object.
     */
	explicit FOVSystem();

	FOVSystem(FOVSystem &) = delete;
	FOVSystem &operator=(FOVSystem &) = delete;

	~FOVSystem();

	/**
     * @brief Add a new source to the FOVSystem and create its corresponding tracks.
     * @param[in] source Pointer to the OBS source.
     */
	void addSource(obs_source_t *source);

	/**
     * @brief Remove a source and its tracks from the FOVSystem.
     * @param[in] source Pointer to the OBS source.
     * @return true True on success.
     * @return false False if isInit is false, if source is null, or if the source is not found in the tracking deques.
     */
	bool removeSource(obs_source_t *source);

	/**
     * @brief Remove all active sources and tracks from the FOVSystem.
     */
	void clearSources();

	/**
     * @brief Initialize the FOVSystem with a specific muxer output and default settings.
     * @param[in] ffmpegMpegtsMuxerOutput Pointer to the MPEG-TS muxer output.
     * @param[in] videoSettings Pointer to the settings data object.
     * @param[in] audioSettings Pointer to the settings data object.
     */
	void initSystem(obs_output_t *ffmpegMpegtsMuxerOutput, obs_data_t *videoSettings = nullptr,
			obs_data_t *audioSettings = nullptr);

	/**
     * @brief Update settings and type for all active video encoders.
     * @param[in] encoderSettings Pointer to the settings data object.
     * @param[in] encoderID Identifier string of the OBS encoder.
     */
	void updateVideoEncoderSettings(obs_data_t *encoderSettings, const std::string &encoderID = "obs_x264");

	/**
     * @brief Update settings and type for all active audio encoders.
     * @param[in] encoderSettings Pointer to the settings data object.
     * @param[in] encoderID Identifier string of the OBS encoder.
     */
	void updateAudioEncoderSettings(obs_data_t *encoderSettings, const std::string &encoderID = "ffmpeg_aac");

	/**
     * @brief Synchronize active sources by scanning all available OBS audio and video sources.
     */
	void syncSources();

protected:
	/**
     * @struct VideoTrack
     * @brief Manages an video track for the FOV System.
     */
	struct VideoTrack {
		std::string encoderID;           /**< Identifier string of the OBS encoder. */
		video_t *videoContext = nullptr; /**< Pointer to the internal video render context. */
		OBSSource source;                /**< OBS source. */
		OBSView view;                    /**< Internal OBS view object. */
		OBSEncoder encoder;              /**< Instantiated video encoder object. */
		OBSData encoderSettings;         /**< Local encoder settings data object. */
		struct obs_video_info ovi{0};    /**< OBS Video Info. */

		/**
         * @brief Construct a new VideoTrack object.
         * @param[in] rawSource Pointer to the OBS source.
         * @param[in] videoSettings Pointer to the settings data object.
         * @param[in] encoderID Identifier string of the OBS encoder.
         */
		VideoTrack(obs_source_t *rawSource, obs_data_t *videoSettings, std::string encoderID);

		/**
         * @brief Destroy the VideoTrack object.
         */
		~VideoTrack();

		/**
         * @brief Apply new settings to the video encoder.
         * @param[in] videoSettings Pointer to the settings data object.
         */
		void updateEncoderSettings(obs_data_t *videoSettings);

		/**
         * @brief Set the underlying OBS source for this video track.
         * @param[in] source Pointer to the OBS source. Pass nullptr to unbind and clear the track.
         * @return true True on success.
         * @return false False if source is null, or if refreshVideoSettings fails during execution.
         */
		bool setSource(obs_source_t *source);

		/**
         * @brief Recreate the video context and view matching the current source properties.
         * @return true True on success.
         * @return false False if this->source is null, or if obs_view_add2 fails to allocate the video context.
         */
		bool refreshVideoSettings();

		/**
         * @brief Change the encoder type used for this video track.
         * @param[in] encoderID Identifier string of the OBS encoder.
         * @return true True on success.
         * @return false False if encoderID matches the current type, or if obs_video_encoder_create fails to instantiate the encoder.
         */
		bool changeEncoderType(const std::string &encoderID);
	};

	/**
     * @struct AudioTrack
     * @brief Manages an independent audio processing pipeline.
     */
	struct AudioTrack {
		std::string encoderID;   /**< Identifier string of the OBS encoder. */
		OBSSource source;        /**< OBS source. */
		OBSEncoder encoder;      /**< Instantiated audio encoder object. */
		OBSData encoderSettings; /**< Local encoder settings data object. */

		/**
         * @brief Construct a new AudioTrack object.
         * @param[in] rawSource Pointer to the OBS source.
         * @param[in] audioSettings Pointer to the settings data object.
         * @param[in] encoderID Identifier string of the OBS encoder.
         * @param[in] registeredMixes Count of already registered audio mixes. Used to programmatically assign the mixer ID bitmask.
         */
		AudioTrack(obs_source_t *rawSource, obs_data_t *audioSettings, std::string encoderID,
			   int registeredMixes = -1);

		/**
         * @brief Destroy the AudioTrack object.
         */
		~AudioTrack();

		/**
         * @brief Apply new settings to the audio encoder.
         * @param[in] audioSettings Pointer to the settings data object.
         */
		void updateEncoderSettings(obs_data_t *audioSettings);

		/**
         * @brief Set the underlying OBS source for this audio track.
         * @param[in] source Pointer to the OBS source. Pass nullptr to unbind and clear the track.
         * @return true True on success.
         * @return false False if source is null.
         */
		bool setSource(obs_source_t *source);

		/**
         * @brief Recreate the audio encoder matching the current source properties.
         * @return true True on success.
         * @return false False if this->source is null.
         */
		bool refreshAudioSettings();

		/**
         * @brief Change the encoder type used for this audio track.
         * @param[in] encoderID Identifier string of the OBS encoder.
         * @return true True on success.
         * @return false False if encoderID matches the current type, or if obs_audio_encoder_create fails to instantiate the encoder.
         */
		bool changeEncoderType(const std::string &encoderID);

	protected:
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
	};

private:
	bool isInit = false;                       /**< Initialization flag. */
	std::string videoEncoderID = "obs_x264";   /**< Default video encoder identifier string. */
	std::string audioEncoderID = "ffmpeg_aac"; /**< Default audio encoder identifier string. */

	obs_output_t *ffmpegMpegtsMuxerOutput = nullptr; /**< Pointer to the MPEG-TS muxer output. */

	obs_data_t *videoSettings; /**< Pointer to the default video settings data object. */
	obs_data_t *audioSettings; /**< Pointer to the default audio settings data object. */

	OBSEncoderGroup encoderGroup; /**< Wrapper containing the shared encoder group object. */
	std::deque<std::unique_ptr<VideoTrack>>
		videoTracks; /**< Deque storing unique pointers to active VideoTrack objects. */
	std::deque<std::unique_ptr<AudioTrack>>
		audioTracks; /**< Deque storing unique pointers to active AudioTrack objects. */

	/**
     * @brief Update the track counts in the active service configuration settings.
     */
	void updateServiceTracks();

	/**
     * @brief Rebuild the encoder group and bind active track encoders to the output muxer.
     */
	void updateEncoderGroup();
};

/**
 * @brief Check if the active service is a valid FOV service instance.
 * @param[in] service Pointer to the OBS service object.
 * @return true True if the service identifier matches the FOV service signature.
 * @return false False if service is null, or if the identifier does not match.
 */
inline bool checkIsFOV(const obs_service_t *service)
{
	if (service == nullptr) {
		return false;
	}
	return (strcmp(obs_service_get_id(service), "fov_service") == 0);
}
