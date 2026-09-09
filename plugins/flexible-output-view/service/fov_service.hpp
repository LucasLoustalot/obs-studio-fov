/**
 * @file fov_service.hpp
 * @author The FOV Team
 * @brief Header file for the FOVService class managing backend API signaling and connection states.
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

#ifdef DEBUG_SRT_STREAM
     #define API_FFMPEG_START_ROUTE "/ffmpeg/register/debug" /** Debug API URL */
#else
     #define API_FFMPEG_START_ROUTE "/ffmpeg/register"  /**< API route used to initiate multi-track streaming. */
#endif

#define API_FFMPEG_STOP_ROUTE "/ffmpeg/stop"       /**< API route used to terminate multi-track streaming. */

extern "C" {
/**
 * @brief Register the custom FOV service module with the OBS framework core.
 */
void registerFOVService(void);
}

/**
 * @class FOVService
 * @brief Custom OBS service class managing backend API signaling and multi-track connection configurations.
 */
class FOVService {
public:
	/**
     * @brief Construct a new FOVService object and parse initial configuration parameters.
     * @param[in] settings Pointer to the OBS settings data object.
     * @param[in] service Pointer to the associated OBS service context object.
     */
	FOVService(obs_data_t *settings, obs_service_t *service) noexcept;

	/**
     * @brief Destroy the FOVService object.
     */
	~FOVService() noexcept;

	/**
     * @brief Get the display name of the service.
     * @return const char* Pointer to a null-terminated string containing the service name.
     */
	const char *getName() const noexcept;

	/**
     * @brief Update the internal service configuration parameters with new settings.
     * @param[in] settings Pointer to the OBS settings data object. Execution returns early without modifying state if settings is null.
     */
	void update(obs_data_t *settings) noexcept;

	/**
     * @brief Generate the UI property definitions structure for user configuration.
     * @return obs_properties_t* Pointer to the allocated properties object. Returns null if memory allocation fails.
     */
	obs_properties_t *getProperties(void) noexcept;

	/**
     * @brief Activate the service and transmit track registration metadata to the backend API.
     * @param[in] settings Pointer to the OBS settings data object. Activation fails and aborts early if settings is null, if the backend URL is empty, or if the HTTP registration request fails.
     */
	void activate(obs_data_t *settings) noexcept;

	/**
     * @brief Deactivate the service and notify the backend API to terminate the streaming session.
     */
	void deactivate(void) noexcept;

	/**
     * @brief Get the current target stream or backend server URL.
     * @return const char* Pointer to a null-terminated string containing the URL. Returns an empty string if unconfigured.
     */
	const char *getURL(void) noexcept;

	/**
     * @brief Parse encoder settings datasets to extract and cache active video and audio track parameters.
     * @param[in] video_settings Pointer to the video encoder settings data object.
     * @param[in] audio_settings Pointer to the audio encoder settings data object.
     */
	void applyEncoderSettings(obs_data_t *video_settings, obs_data_t *audio_settings) noexcept;

	/**
     * @brief Get specific connection credentials or routing metadata by data category type.
     * @param[in] type Numeric identifier code indicating the requested connection parameter data type.
     * @return const char* Pointer to a null-terminated string containing the requested configuration value. Returns null if the type identifier code is invalid or unhandled.
     */
	const char *getConnectInfo(uint32_t type) noexcept;

private:
	std::string backendURL; /**< Configured base address for backend HTTP API orchestration. */
	std::string srtURL;     /**< Resolved target ingestion endpoint URL for the multi-track stream output. */
	std::string streamKey;  /**< Authentication token string assigned to validate the active streaming session. */
	size_t nbVideoTracks;   /**< Cached count tracking the number of active concurrent video pipelines. */
	size_t nbAudioTracks;   /**< Cached count tracking the number of active concurrent audio buses. */
	nlohmann::json videoTrackNames;
	nlohmann::json audioTrackNames;
	bool started;           /**< Internal state tracking flag confirming if backend session signaling has executed successfully. */
};

bool obs_array_to_json(obs_data_array_t *array, nlohmann::json &json_out, const std::string &objectName = "");
