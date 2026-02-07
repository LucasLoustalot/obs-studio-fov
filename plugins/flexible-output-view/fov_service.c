/**
 * @file fov-service.c
 * @author The FOV Team
 * @brief FOV service
 * @version 0.1
 * @date 2026-01-31
 */

#include "obs-data.h"
#include "obs.h"
#include <stdio.h>
#include <obs-module.h>
#include <time.h>
#include "fov_output.h"
#include <util/curl/curl-helper.h>
#include "util/base.h"
#include "util/bmem.h"
#include "fov_output.h"

typedef struct fov_service_s {
	char *backend_url;
	char *srt_url;
	long long nb_video_encoders;
	bool started;
} fov_service_t;


const char *fov_service_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	blog(LOG_INFO, "FOV Service get name\n");

	return "FOV Service";
}

static void fov_service_update(void *data, obs_data_t *settings)
{
	fov_service_t *fov_service = data;
	blog(LOG_INFO, "FOV Service update\n");

	bfree(fov_service->backend_url);

	fov_service->backend_url = bstrdup(obs_data_get_string(settings, "server"));
	fov_service->srt_url = bstrdup(obs_data_get_string(settings, "srt_endpoint"));
	fov_service->nb_video_encoders = obs_data_get_int(settings, "video_encoder_count");
	//     service->key = bstrdup(obs_data_get_string(settings, "key"));
	//     service->use_auth = obs_data_get_bool(settings, "use_auth");
}

static void fov_service_destroy(void *data)
{
	fov_service_t *fov_service = data;
	blog(LOG_INFO, "FOV Service destroy\n");

	bfree(fov_service->backend_url);
	bfree(fov_service);
}

static void *fov_service_create(obs_data_t *settings, obs_service_t *service)
{
	UNUSED_PARAMETER(service);
	blog(LOG_INFO, "FOV Service create\n");

	fov_service_t *fov_service = bzalloc(sizeof(fov_service_t));
	fov_service->started = false;

	fov_service_update(fov_service, settings);
	return (fov_service);
}

static obs_properties_t *fov_service_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *ppts = obs_properties_create();

	obs_properties_add_text(ppts, "server", "URL", OBS_TEXT_DEFAULT);

	//     obs_properties_add_text(ppts, "key", obs_module_text("StreamKey"), OBS_TEXT_PASSWORD);
	// obs_property_t *p;
	//     p = obs_properties_add_bool(ppts, "use_auth", obs_module_text("UseAuth"));
	return ppts;
}

static const char *fov_service_get_protocol(void *data)
{
	UNUSED_PARAMETER(data);

	blog(LOG_INFO, "FOV Service get protocol\n");

	return "SRT";
}
static void fov_service_apply_settings(void *data, obs_data_t *video_settings, obs_data_t *audio_settings)
{
	UNUSED_PARAMETER(data);

	blog(LOG_INFO, "FOV Service apply settings\n");
	obs_data_set_bool(video_settings, "repeat_headers", true);
	obs_data_set_bool(audio_settings, "set_to_ADTS", true);
}

static const char *fov_service_custom_url(void *data)
{
	UNUSED_PARAMETER(data);

	fov_service_t *fov_service = data;
	blog(LOG_INFO, "FOV Service can try connect, nb_video_tracks: %lld\n", fov_service->nb_video_encoders);

	if (fov_service->backend_url == NULL || fov_service->nb_video_encoders == 0) {
		return false;
	}

	if (fov_service->started) {
		return fov_service->srt_url;
	}

	// Making post request to backend /ffmpeg/start
	char route[512];
	snprintf(route, sizeof(route), "%s/ffmpeg/start", fov_service->backend_url);
	blog(LOG_INFO, "FOV: Making request to backend %s/ffmpeg/start", fov_service->backend_url);

	char json_payload[128];
	snprintf(json_payload, sizeof(json_payload), "{\"tracks\": %lld}", fov_service->nb_video_encoders);

	bool success = false;
	CURL *curl = curl_easy_init();
	if (curl) {
		struct curl_slist *headers = NULL;
		headers = curl_slist_append(headers, "Content-Type: application/json");
		curl_easy_setopt(curl, CURLOPT_URL, route);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

		CURLcode res = curl_easy_perform(curl);
		if (res == CURLE_OK) {
			long response_code;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
			if (response_code == 200) {
				blog(LOG_INFO, "FOV: Backend acknowledged %lld tracks", fov_service->nb_video_encoders);
				success = true;
				fov_service->started = true;
			} else {
				blog(LOG_ERROR, "FOV: Backend returned error %ld", response_code);
			}
		} else {
			blog(LOG_ERROR, "FOV: Curl failed: %s", curl_easy_strerror(res));
		}

		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);
	}

	if (success) {
		return fov_service->srt_url;
	} else {
		return NULL;
	}
}

static const char *fov_service_get_connect_info(void *data, uint32_t type)
{
	blog(LOG_INFO, "FOV Service connect info\n");
	switch ((enum obs_service_connect_info)type) {
	case OBS_SERVICE_CONNECT_INFO_SERVER_URL:
		return fov_service_custom_url(data);
	case OBS_SERVICE_CONNECT_INFO_BEARER_TOKEN:
		return NULL;
	default:
		break;
	}
	

	return NULL;
}

static void fov_service_activate(void *data, obs_data_t *settings)
{
	blog(LOG_INFO, "FOV Service activate\n");
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(settings);
}

static void fov_service_deactivate(void *data)
{
	blog(LOG_INFO, "FOV Service deactivate\n");
	fov_service_t *fov_service = data;

	// Making post request to backend /ffmpeg/stop
	char route[512];
	snprintf(route, sizeof(route), "%s/ffmpeg/stop", fov_service->backend_url);
	blog(LOG_INFO, "FOV: Making request to backend %s/ffmpeg/stop", fov_service->backend_url);
	bool success = false;
	CURL *curl = curl_easy_init();
	if (curl) {
		struct curl_slist *headers = NULL;
		headers = curl_slist_append(headers, "Content-Type: application/json");
		curl_easy_setopt(curl, CURLOPT_URL, route);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);

		CURLcode res = curl_easy_perform(curl);
		if (res == CURLE_OK) {
			long response_code;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
			if (response_code == 200) {
				blog(LOG_INFO, "FOV: Backend acknowledged stopping streaming");
				success = true;
			} else {
				blog(LOG_ERROR, "FOV: Backend returned error %ld", response_code);
			}
		} else {
			blog(LOG_ERROR, "FOV: Curl failed: %s", curl_easy_strerror(res));
		}

		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);
	}
	fov_service->started = false;
}

struct obs_service_info fov_service = {
	.id = "fov_service",
	.get_name = fov_service_get_name,
	.create = fov_service_create,
	.destroy = fov_service_destroy,
	.update = fov_service_update,
	.get_properties = fov_service_properties,
	.get_protocol = fov_service_get_protocol,
	.activate = fov_service_activate,
	.deactivate = fov_service_deactivate,
	.get_url = fov_service_custom_url,
	.get_connect_info = fov_service_get_connect_info,
	.apply_encoder_settings = fov_service_apply_settings,
};
