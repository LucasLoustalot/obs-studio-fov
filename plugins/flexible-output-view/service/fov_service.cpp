/**
 * @file fov_service.cpp
 * @author The FOV Team
 * @brief FOV custom service implementation, this service is used together with fov_output
 * @version 0.1
 * @date 2026-02-07
 */

#include "fov_service.hpp"
#include "curl_wrapper.hpp"
#include "util/base.h"
#include <string>
#include <thread>
#include <vector>

FOVService::FOVService(obs_data_t *settings, obs_service_t *) noexcept
	: backendURL(""),
	  srtURL(""),
	  nbVideoTracks(1),
	  started(false)
{
	blog(LOG_INFO, "FOV Service created\n");
	update(settings);
}

FOVService::~FOVService() noexcept {}

const char *FOVService::getName() const noexcept
{
	return "FOV Service";
}

void FOVService::update(obs_data_t *settings) noexcept
{
	backendURL = obs_data_get_string(settings, "server");
	srtURL = obs_data_get_string(settings, "srt_endpoint");
	nbVideoTracks = obs_data_get_int(settings, "video_encoder_count");

	blog(LOG_INFO, "FOV Service settings changed\n");
}

obs_properties_t *FOVService::getProperties(void) noexcept
{
	obs_properties_t *ppts = obs_properties_create();

	obs_properties_add_text(ppts, "server", "URL", OBS_TEXT_DEFAULT);
	obs_properties_add_text(ppts, "srt_endpoint", "SRT url for the obs output", OBS_TEXT_DEFAULT);

	return ppts;
}

void FOVService::applyEncoderSettings(obs_data_t *video_settings, obs_data_t *audio_settings) noexcept
{
	obs_data_set_bool(video_settings, "repeat_headers", true);
	obs_data_set_bool(audio_settings, "set_to_ADTS", true);

	blog(LOG_INFO, "FOV Service applied encoder settings\n");
}

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

const char *FOVService::getURL(void) noexcept
{
	if (backendURL.empty() || nbVideoTracks == 0) {
		return nullptr;
	}

	if (started) {
		return srtURL.c_str();
	}

	// Making post request to backend
	const std::string APIRoute = backendURL + API_FFMPEG_START_ROUTE;
	nlohmann::json jsonPayload;
	jsonPayload["tracks"] = nbVideoTracks;

	blog(LOG_INFO, "FOV Service making request to backend %s\n", APIRoute.c_str());
	try {
		SimpleCurlRequest request(APIRoute, SimpleCurlRequest::HTTP_POST);

		request.setHeaders({"Content-Type: application/json"});
		request.setRequestPayload(jsonPayload.dump());
		request.performRequest(3);

		if (request.getResponseCode() == 200) {
			blog(LOG_INFO, "FOV Service backend acknowledged %zu tracks [HTTP %d]: %s\n", nbVideoTracks,
			     request.getResponseCode(), request.getResponseContent().c_str());
			started = true;
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

void FOVService::activate(obs_data_t *) noexcept
{
	blog(LOG_INFO, "FOV Service activated\n");
}

void FOVService::deactivate(void) noexcept
{
	blog(LOG_INFO, "FOV Service deactivated\n");

	// Making post request to backend
	const std::string APIRoute = backendURL + API_FFMPEG_STOP_ROUTE;

	blog(LOG_INFO, "FOV Service making request to backend %s\n", APIRoute.c_str());

	std::thread([APIRoute]() {
		try {
			SimpleCurlRequest request(APIRoute, SimpleCurlRequest::HTTP_POST);
			request.setHeaders({"Content-Type: application/json"});
			request.performRequest();

			blog(LOG_INFO, "FOV: Stop request finished background thread.");

		} catch (const std::exception &e) {
			blog(LOG_ERROR, "FOV Service stop error: %s\n", e.what());
		}
	}).detach();

	started = false;
}

extern "C" {
void registerFOVService(void)
{
	struct obs_service_info info = {};

	info.id = "fov_service";
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


	obs_register_service(&info);
}
}