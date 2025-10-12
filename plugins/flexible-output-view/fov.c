/**
 * @file fov.c
 * @author The FOV Team
 * @brief Entry point for flexible output view
 * @version 0.1
 * @date 2025-09-13
 */

#include "obs/obs-frontend-api.h"
#include "callback/signal.h"
#include "obs.h"
#include "util/base.h"
#include "util/bmem.h"
#include "util/platform.h"
#include <stdlib.h>
#include <obs-module.h>
#include <time.h>

static struct fov {
	obs_output_t *fov_out;
	obs_service_t *fov_service;
} fov_app;

// #define blog(log_level, format, ...) blog(log_level, "[FOV: '%s'] " format, ##__VA_ARGS__)

#define debug(format, ...) blog(LOG_DEBUG, "FOV: "format, ##__VA_ARGS__)
#define info(format, ...) blog(LOG_INFO, "FOV: "format, ##__VA_ARGS__)
#define warn(format, ...) blog(LOG_WARNING, "FOV: "format, ##__VA_ARGS__)

OBS_DECLARE_MODULE()
// OBS_MODULE_USE_DEFAULT_LOCALE("flexible-output-view", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "The flexible output view system";
}

// extern struct obs_service_info fov_service;
// extern struct obs_output_info fov_output;

const char *fov_service_get_name(void *type_data)
{
	(void)type_data;
	return ("FlexibleOutputViewService");
}

void *fov_service_create(obs_data_t *settings, obs_service_t *service)
{
	(void)settings;
	(void)service;
	debug("Creation du service FOV");
	return (bzalloc(10)); // Allouer la structure interne au module ici
}

void fov_service_destroy(void *data)
{
	bfree(data);
	debug("Destruction du service FOV");
}

const char *fov_service_get_url(void *data)
{
	(void)data;
	debug("FOV address");
	return ("rtmp://localhost");
}

const char *fov_service_get_key(void *data)
{
	(void)data;
	debug("FOV get stream key");
	return ("stream0");
}

void fov_service_update(void *data, obs_data_t *settings)
{
	(void)data;
	(void)settings;
	debug("UPDATE FOV");
}

const char *fov_service_get_output_type(void *data)
{
	(void)data;
	return ("flexible_output_view_output");
}

const char *fov_service_get_protocol(void *data)
{
	(void)data;
	debug("FOV get protocol");
	return ("RTMP");
}

const char **get_supported_video_codecs(void *data)
{
	(void)data;
	static const char *video_codecs[] = {"h264", NULL};
	debug("FOV supported video");
	return (video_codecs);
}

const char **get_supported_audio_codecs(void *data)
{
	(void)data;
	static const char *audio_codecs[] = {"opus", NULL};
	debug("FOV supported audio");
	return (audio_codecs);
}

bool fov_property_modified_t(obs_properties_t *props, obs_property_t *property, obs_data_t *settings)
{
	(void)props;
	(void)property;
	(void)settings;
	debug("FOV refresh UI");

	return (true);
}

static obs_properties_t *my_source_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *ppts = obs_properties_create();
	obs_property_t *p;

	obs_properties_add_text(ppts, "server", "URL", OBS_TEXT_DEFAULT);

	obs_properties_add_text(ppts, "key", "StreamKey", OBS_TEXT_PASSWORD);

	p = obs_properties_add_bool(ppts, "use_auth", "UseAuth");
	obs_properties_add_text(ppts, "username", "Username", OBS_TEXT_DEFAULT);
	obs_properties_add_text(ppts, "password", "Password", OBS_TEXT_PASSWORD);
	obs_property_set_modified_callback(p, fov_property_modified_t);
	return ppts;
}

bool fov_initialize(void *data, obs_output_t *output)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(output);
	debug("FOV is ready to stream");
	return (true);
}

bool fov_can_connect(void *data)
{
	UNUSED_PARAMETER(data);
	debug("FOV is can stream");
	return (true);
}

void fov_activate(void *data, obs_data_t *settings)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(settings);
	debug("FOV is activated");
}
void fov_deactivate(void *data)
{
	UNUSED_PARAMETER(data);
	debug("FOV is deactivated");
}

static const char *fov_get_username(void *data)
{
	(void)data;
	return "stream1";
}

static const char *fov_get_password(void *data)
{
	(void)data;
	return "stream1";
}

static const char *fov_get_connect_info(void *data, uint32_t type)
{
	debug("FOV Get connect info");

	(void)data;
	switch ((enum obs_service_connect_info)type) {
	case OBS_SERVICE_CONNECT_INFO_SERVER_URL:
		return "rtmp://localhost";
	case OBS_SERVICE_CONNECT_INFO_STREAM_ID:
		return "stream1";
	case OBS_SERVICE_CONNECT_INFO_USERNAME:
		return "stream1";
	case OBS_SERVICE_CONNECT_INFO_PASSWORD:
		return "stream1";
	case OBS_SERVICE_CONNECT_INFO_ENCRYPT_PASSPHRASE: {
		break;
	}
	case OBS_SERVICE_CONNECT_INFO_BEARER_TOKEN:
		return NULL;
	}

	return NULL;
}

static void fov_apply_encoder_settings(void *data, obs_data_t *video_settings, obs_data_t *audio_settings)
{
	(void)data;
	(void)video_settings;
	(void)audio_settings;
	debug("FOV Apply encoder");
}

struct obs_service_info fov_service = {.id = "flexible_output_view_service",
				       .get_name = fov_service_get_name,
				       .create = fov_service_create,
				       .destroy = fov_service_destroy,
				       .update = fov_service_update,
				       .get_properties = my_source_properties,
				       .get_protocol = fov_service_get_protocol,
				       .get_url = fov_service_get_url,
				       .can_try_to_connect = fov_can_connect,
				       .get_connect_info = fov_get_connect_info,
				       .get_username = fov_get_username,
				       .get_password = fov_get_password,
				       .apply_encoder_settings = fov_apply_encoder_settings,
				       .initialize = fov_initialize,
				       .get_key = fov_service_get_key};

bool fov_output_start(void *a)
{
	(void)a;

	debug("FOV output start");
	return (true);
}

void fov_output_stop(void *a, uint64_t b)
{
	(void)a;
	(void)b;

	debug("FOV output stop");
}

void *fov_output_create(obs_data_t *a, obs_output_t *b)
{
	(void)a;
	(void)b;
	debug("FOV output create");
	return (bzalloc(10));
}

uint64_t fov_output_get_total_bytes(void *a)
{
	(void)a;
	return (0);
}

void fov_output_encoded_packet(void *data, struct encoder_packet *packet)
{
	(void)data;
	(void)packet;

	debug("FOV packet");
	return;
}

static const char *fov_output_protocols[] = {"RTMP", NULL};
struct obs_output_info fov_output = {.id = "flexible_output_view_output",
				     .flags = OBS_OUTPUT_AV | OBS_OUTPUT_ENCODED | OBS_OUTPUT_SERVICE |
					      OBS_OUTPUT_MULTI_TRACK_AV,
				     .get_name = fov_service_get_name,
				     .create = fov_output_create,
				     .destroy = fov_service_destroy,
				     .start = fov_output_start,
				     .stop = fov_output_stop,
				     .encoded_packet = fov_output_encoded_packet,
				     .get_total_bytes = fov_output_get_total_bytes,
				     .protocols = (const char *)fov_output_protocols,
				     .encoded_video_codecs = "h264",
				     .encoded_audio_codecs = "aac"};

static void fov_start_output(void *data, calldata_t *calldata)
{
	(void)data;
	(void)calldata;

	debug("Debut de l'ouput FOV");
}

static bool fov_list_sources(void *data, obs_source_t *source)
{
	(void)data;
	debug("fov_list_sources FOV Source: %s", obs_source_get_name(source));

	return (true);
}

static void frontend_event(enum obs_frontend_event event, void *data)
{
	static bool created = false;
	(void)data;
	(void)event;
	// if (event == OBS_FRONTEND_EVENT_STREAMING_STARTING || event == OBS_FRONTEND_EVENT_STREAMING_STARTED ||
	//     event == OBS_FRONTEND_EVENT_STREAMING_STOPPING || event == OBS_FRONTEND_EVENT_STREAMING_STOPPED ||
	//     event == OBS_FRONTEND_EVENT_RECORDING_STARTING || event == OBS_FRONTEND_EVENT_RECORDING_STARTED ||
	//     event == OBS_FRONTEND_EVENT_RECORDING_STOPPING || event == OBS_FRONTEND_EVENT_RECORDING_STOPPED ||
	//     event == OBS_FRONTEND_EVENT_VIRTUALCAM_STARTED || event == OBS_FRONTEND_EVENT_VIRTUALCAM_STOPPED ||
	//     event == OBS_FRONTEND_EVENT_RECORDING_PAUSED || event == OBS_FRONTEND_EVENT_RECORDING_UNPAUSED) {
	// 	debug( "FOV Callback frontend debug");
	// }

	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING || event == OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED) {
		debug("--- FOV Enum sources ---");
		obs_enum_sources(fov_list_sources, NULL);
	}

	if (event == OBS_FRONTEND_EVENT_STUDIO_MODE_ENABLED) {
		debug("--- FOV Studio mode enabled ---");

		// Start FOV
		if (created != true) {
			const char *enc_id = "obs_x264";
			created = true;
			obs_encoder_t *encoder = obs_video_encoder_create(enc_id, "FOVSource", NULL,NULL);
			obs_encoder_set_video(encoder, obs_get_video());
			obs_encoder_set_scaled_size(encoder, 1280, 720);
			obs_output_set_video_encoder(fov_app.fov_out, encoder);

			// TO File
			obs_data_t *s = obs_data_create();
			const char *format = "mp4";
			const char *path = "/home/lucas/Desktop";

			char *filename = "/home/lucas/Desktop/FOVtest.mp4";
			obs_data_set_string(s, "path", filename);
			obs_data_set_string(s, "directory", path);
			obs_data_set_string(s, "format", "hybrid_mp4");
			obs_data_set_string(s, "extension", format);
			obs_output_update(fov_app.fov_out, s);

			obs_output_start(fov_app.fov_out);
		}
	}
}

bool obs_module_load(void)
{

	debug("Le module FOV est chargé !");

	obs_register_output(&fov_output);
	obs_register_service(&fov_service);

	fov_app.fov_service = obs_service_create("flexible_output_view_service", "FOV Service", NULL, NULL);
	fov_app.fov_out = obs_output_create("flexible_output_view_output", "FOV Output", NULL, NULL);

	obs_output_set_service(fov_app.fov_out, fov_app.fov_service);
	obs_frontend_add_event_callback(frontend_event, &fov_app);

	signal_handler_t *handler = obs_output_get_signal_handler(fov_app.fov_out);
	signal_handler_connect(handler, "start", fov_start_output, NULL);

	obs_output_initialize_encoders(fov_app.fov_out, 0);

	if (obs_output_can_begin_data_capture(fov_app.fov_out, 0)) {
		debug("L'output FOV peut commencer la capture :)");
		obs_output_begin_data_capture(fov_app.fov_out, 0);
		obs_output_start(fov_app.fov_out);
	} else {
		debug("L'output FOV ne peut pas commencer la capture !");
	}

	return true;
}

void obs_module_unload()
{
	obs_output_end_data_capture(fov_app.fov_out);
	obs_output_signal_stop(fov_app.fov_out, OBS_OUTPUT_SUCCESS);
	obs_output_stop(fov_app.fov_out);
	obs_service_release(fov_app.fov_service);
	obs_output_release(fov_app.fov_out);

	debug("Unload FOV");
}
