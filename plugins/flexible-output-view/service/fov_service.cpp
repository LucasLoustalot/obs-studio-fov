/**
 * @file fov_service.cpp
 * @author The FOV Team
 * @brief Implementation of the FOVService class managing backend API signaling and connection states.
 * @version 0.1
 * @date 2026-02-07
 */

#include "fov_service.hpp"
#include "curl_wrapper.hpp"
#include "obs-data.h"
#include "util/base.h"
#include <string>
#include <thread>
#include <vector>

static const char *fov_audio_codecs[] = {"opus", "aac",
					 nullptr};         /**< Array containing supported audio codec string names. */
static const char *fov_video_codecs[] = {"h264", nullptr}; /**< Array containing supported video codec string names. */

/**
 * @brief Converts an obs data array to an nlohmann json structure.
 * @param[in] array OBS data array to parse. Execution returns early if null.
 * @param[out] json_out Target JSON container to populate with parsed array contents.
 * @param[in] objectName Optional JSON key name wrapper. Default behavior passes an unmapped array if left empty.
 * @return bool Returns true if array translation and JSON parsing completed without structural exception errors.
 */
bool obs_array_to_json(obs_data_array_t *array, nlohmann::json &json_out, const std::string &objectName)
{
	json_out.clear();

	if (!array) {
		return false;
	}

	nlohmann::json json_array = nlohmann::json::array();
	const size_t count = obs_data_array_count(array);

	for (size_t i = 0; i < count; ++i) {
		obs_data_t *item = obs_data_array_item(array, i);
		if (!item) {
			return false;
		}

		const char *json_str = obs_data_get_json(item);
		if (!json_str) {
			obs_data_release(item);
			return false;
		}

		try {
			json_array.push_back(nlohmann::json::parse(json_str));
		} catch (const nlohmann::json::parse_error &) {
			obs_data_release(item);
			return false;
		}

		obs_data_release(item);
	}

	if (objectName.empty()) {
		json_out = std::move(json_array);
	} else {
		json_out[objectName] = std::move(json_array);
	}

	return true;
}
/**
 * @brief Construct a new FOVService object and parse initial configuration parameters.
 * @param[in] settings Pointer to the OBS settings data object.
 * @param[in] service Unused pointer to the associated OBS service context object.
 */
FOVService::FOVService(obs_data_t *settings, obs_service_t *) noexcept
	: backendURL(""),
	  srtURL(""),
	  nbVideoTracks(1),
	  nbAudioTracks(1),
	  started(false)
{
	blog(LOG_INFO, "FOV Service created\n");
	update(settings);
}

/**
 * @brief Destroy the FOVService object.
 */
FOVService::~FOVService() noexcept {}

/**
 * @brief Get the display name of the service.
 * @return const char* Pointer to a null-terminated string containing the service name.
 */
const char *FOVService::getName() const noexcept
{
	return "FOV Service";
}

/**
 * @brief Update the internal service configuration parameters with new settings.
 * @param[in] settings Pointer to the OBS settings data object. Execution returns early without modifying state if settings is null. Defaults track counts to 1 if properties evaluate to less than or equal to 0.
 */
void FOVService::update(obs_data_t *settings) noexcept
{
	if (!settings) {
		return;
	}

	size_t newVideoTracks = 0;
	size_t newAudioTracks = 0;

	backendURL = obs_data_get_string(settings, "server");
	streamKey = obs_data_get_string(settings, "key");


	newVideoTracks = obs_data_get_int(settings, "video_encoder_count");
	newAudioTracks = obs_data_get_int(settings, "audio_track_count");

	obs_data_array_t *videoNames = obs_data_get_array(settings, "videoTrackNames");
	obs_data_array_t *audioNames = obs_data_get_array(settings, "audioTrackNames");

	obs_array_to_json(videoNames, videoTrackNames);
	obs_array_to_json(audioNames, audioTrackNames);

	if (videoNames) {
		obs_data_array_release(videoNames);
	}
	if (audioNames) {
		obs_data_array_release(audioNames);
	}

	if (newVideoTracks <= 0) {
		blog(LOG_WARNING, "FOV Service invalid 'video_encoder_count' property, defaulting to 1");
		newVideoTracks = 1;
	}
	if (newAudioTracks <= 0) {
		blog(LOG_WARNING, "FOV Service invalid 'audio_track_count' property, defaulting to 1");
		newAudioTracks = 1;
	}

	this->nbVideoTracks = newVideoTracks;
	this->nbAudioTracks = newAudioTracks;

	blog(LOG_INFO, "FOV Service settings changed: nbVideoTracks:%ld nbAudioTracks:%ld server:%s key:%s\n",
	     nbVideoTracks, nbAudioTracks, backendURL.c_str(), streamKey.c_str());
}

/**
 * @brief Generate the UI property definitions structure for user configuration.
 * @return obs_properties_t* Pointer to the allocated properties object. Returns null if memory allocation fails.
 */
obs_properties_t *FOVService::getProperties(void) noexcept
{
	obs_properties_t *ppts = obs_properties_create();

	obs_properties_add_text(ppts, "server", "URL", OBS_TEXT_DEFAULT);
	obs_properties_add_text(ppts, "key", "Stream Key", OBS_TEXT_DEFAULT);
	obs_properties_add_int(ppts, "nbVideoTracks", "Number of video tracks", 1, 6, 1);
	obs_properties_add_int(ppts, "nbAudioTracks", "Number of audio tracks", 1, 6, 1);

	return ppts;
}

/**
 * @brief Parse encoder settings datasets to extract and cache active video and audio track parameters.
 * @param[in] video_settings Pointer to the video encoder settings data object.
 * @param[in] audio_settings Pointer to the audio encoder settings data object.
 */
void FOVService::applyEncoderSettings(obs_data_t *video_settings, obs_data_t *audio_settings) noexcept
{
	obs_data_set_bool(video_settings, "repeat_headers", true);
	obs_data_set_bool(audio_settings, "set_to_ADTS", true);

	blog(LOG_INFO, "FOV Service applied encoder settings\n");
}

/**
 * @brief Get specific connection credentials or routing metadata by data category type.
 * @param[in] type Numeric identifier code indicating the requested connection parameter data type.
 * @return const char* Pointer to a null-terminated string containing the requested configuration value. Returns null if the type identifier code is invalid or unhandled.
 */
const char *FOVService::getConnectInfo(uint32_t type) noexcept
{
	switch ((enum obs_service_connect_info)type) {
	case OBS_SERVICE_CONNECT_INFO_SERVER_URL:
		return getURL();
	default:
		break;
	}
	return nullptr;
}

/**
 * @brief Get the current target stream or backend server URL. Performs synchronous registration request to the backend API if not already initialized.
 * @return const char* Pointer to a null-terminated string containing the resolved SRT ingest URL. Returns null if the backend server address is empty or if video track tracking is unconfigured. Returns an empty string literal ("\0") if the server responds with a non-200 HTTP code or if a transport exception occurs.
 */
const char *FOVService::getURL(void) noexcept
{
	if (backendURL.empty() || nbVideoTracks == 0) {
		blog(LOG_WARNING, "FOV Service is misconfigured nbVideoTracks:%ld nbAudioTracks:%ld server:%s\n",
		     nbVideoTracks, nbAudioTracks, backendURL.c_str());
		return nullptr;
	}

	if (started) {
		return srtURL.c_str();
	}

	const std::string APIRoute = backendURL + API_FFMPEG_START_ROUTE;
	nlohmann::json jsonPayload;
	jsonPayload["streamId"] = streamKey;
	jsonPayload["tracks"] = nbVideoTracks;
	jsonPayload["videoTrackNames"] = videoTrackNames;
	jsonPayload["audioTracks"] = nbAudioTracks;
	jsonPayload["audioTrackNames"] = audioTrackNames;

	blog(LOG_INFO, "FOV Service making request to backend %s\n", APIRoute.c_str());
	try {
		SimpleCurlRequest request(APIRoute, SimpleCurlRequest::HTTP_POST);

		request.setHeaders({"Content-Type: application/json"});
		request.setRequestPayload(jsonPayload.dump());
		request.performRequest(3);

		if (request.getResponseCode() == 200) {
			blog(LOG_INFO,
			     "FOV Service backend acknowledged %zu video tracks and %zu audio tracks [HTTP %d]: %s\n",
			     nbVideoTracks, nbAudioTracks, request.getResponseCode(),
			     request.getResponseContent().c_str());
			started = true;
			nlohmann::json response = nlohmann::json::parse(request.getResponseContent());
			srtURL = response["srtUrl"].get<std::string>();
			blog(LOG_INFO, "FOV Service got SRT url = %s\n", srtURL.c_str());
			return srtURL.c_str();

		} else {
			blog(LOG_ERROR, "FOV: Backend returned error [HTTP %d]: %s\n", request.getResponseCode(),
			     request.getResponseContent().c_str());
			return "\0";
		}

	} catch (const SimpleCurlException &e) {
		blog(LOG_ERROR, "FOV Service error: %s\n", e.what());
		return "\0";
	}
}

/**
 * @brief Activate the service and transmit track registration metadata to the backend API.
 * @param[in] settings Unused pointer to the OBS settings data object.
 */
void FOVService::activate(obs_data_t *) noexcept
{
	blog(LOG_INFO, "FOV Service activated\n");
}

/**
 * @brief Deactivate the service and notify the backend API to terminate the streaming session. Dispatches the notification request on an isolated asynchronous background worker thread.
 */
void FOVService::deactivate(void) noexcept
{
	blog(LOG_INFO, "FOV Service deactivated\n");

	const std::string APIRoute = backendURL + API_FFMPEG_STOP_ROUTE;
	nlohmann::json jsonPayload;
	jsonPayload["streamId"] = streamKey;

	blog(LOG_INFO, "FOV Service making request to backend %s\n", APIRoute.c_str());

	std::thread([APIRoute, jsonPayload]() {
		try {
			SimpleCurlRequest request(APIRoute, SimpleCurlRequest::HTTP_POST);
			request.setHeaders({"Content-Type: application/json"});
			request.setRequestPayload(jsonPayload.dump());
			request.performRequest();

			blog(LOG_INFO, "FOV: Stop request finished background thread: backend [HTTP %d]: %s\n",
			     request.getResponseCode(), request.getResponseContent().c_str());

		} catch (const std::exception &e) {
			blog(LOG_ERROR, "FOV Service stop error: %s\n", e.what());
		}
	}).detach();

	started = false;
}

extern "C" {
/**
 * @brief Register the custom FOV service module with the OBS framework core.
 */
void registerFOVService(void)
{
	struct obs_service_info info = {};

	info.id = "fov_service";
	info.get_output_type = [](void *) -> const char * {
		return "ffmpeg_mpegts_muxer";
	};
	info.get_name = [](void *priv_data) -> const char * {
		return static_cast<FOVService *>(priv_data)->getName();
	};
	info.create = [](obs_data_t *settings, obs_service_t *service) -> void * {
		return new FOVService(settings, service);
	};
	info.destroy = [](void *priv_data) {
		delete static_cast<FOVService *>(priv_data);
	};
	info.update = [](void *priv_data, obs_data_t *settings) {
		static_cast<FOVService *>(priv_data)->update(settings);
	};
	info.get_properties = [](void *priv_data) -> obs_properties_t * {
		return static_cast<FOVService *>(priv_data)->getProperties();
	};
	info.get_protocol = [](void *) -> const char * {
		return "SRT";
	};
	info.get_url = [](void *priv_data) -> const char * {
		return static_cast<FOVService *>(priv_data)->getURL();
	};
	info.apply_encoder_settings = [](void *priv_data, obs_data_t *video_settings, obs_data_t *audio_settings) {
		static_cast<FOVService *>(priv_data)->applyEncoderSettings(video_settings, audio_settings);
	};
	info.can_try_to_connect = [](void *) -> bool {
		return true;
	};
	info.get_connect_info = [](void *priv_data, uint32_t type) -> const char * {
		return static_cast<FOVService *>(priv_data)->getConnectInfo((enum obs_service_connect_info)type);
	};

	info.activate = [](void *priv_data, obs_data_t *settings) -> void {
		return static_cast<FOVService *>(priv_data)->activate(settings);
	};
	info.deactivate = [](void *priv_data) -> void {
		return static_cast<FOVService *>(priv_data)->deactivate();
	};
	info.get_supported_video_codecs = [](void *) -> const char ** {
		return fov_video_codecs;
	};
	info.get_supported_audio_codecs = [](void *) -> const char ** {
		return fov_audio_codecs;
	};

	obs_register_service(&info);
}
}
