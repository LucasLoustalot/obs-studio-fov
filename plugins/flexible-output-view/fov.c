/**
 * @file fov.c
 * @author The FOV Team
 * @brief Entry point for flexible output view
 * @version 0.1
 * @date 2025-09-13
 */

#include <obs-frontend-api.h>
#include "callback/signal.h"
#include "obs.h"
#include "util/base.h"
#include "util/bmem.h"
#include "util/platform.h"
#include <stdlib.h>
#include <obs-module.h>
#include <time.h>

#define debug(format, ...) blog(LOG_DEBUG, "FOV: "format, ##__VA_ARGS__)
#define info(format, ...) blog(LOG_INFO, "FOV: "format, ##__VA_ARGS__)
#define warn(format, ...) blog(LOG_WARNING, "FOV: "format, ##__VA_ARGS__)

static struct fov_system {
	obs_output_t *fov_out;
	obs_service_t *fov_service;
} fov_app;

typedef struct fov_service_internal_s {
	obs_service_t *obs_service_ref;
	obs_output_t *obs_output_ref;

	void (*test_callback)(void);
} fov_service_internal_t;

typedef struct fov_output_internal_s {
	obs_output_t *obs_output_ref;
	obs_service_t *obs_associated_service_ref;
} fov_output_internal_t;

OBS_DECLARE_MODULE()
// OBS_MODULE_USE_DEFAULT_LOCALE("flexible-output-view", "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "The flexible output view system";
}

const char *fov_service_get_name(void *type_data)
{
	(void)type_data;
	return ("Flexible Output View Service");
}

void test_service_callback_from_output()
{
	debug("FOV service callback called from output receive packet");
}

void *fov_service_create(obs_data_t *settings, obs_service_t *service)
{
	(void)settings;
	debug("Creation du service FOV");

	fov_service_internal_t *fov_service = bzalloc(sizeof(fov_service_internal_t));
	fov_service->obs_service_ref = service;
	fov_service->test_callback = test_service_callback_from_output;
	return (fov_service);
}

void fov_service_destroy(void *data)
{
	debug("Destruction du service FOV");
	bfree(data);
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

static obs_properties_t *fov_source_properties(void *data)
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

bool fov_service_initialize(void *data, obs_output_t *output)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(output);

	debug("FOV service is initializing with output: %s", obs_output_get_name(output));

	debug("FOV service is ready to stream");
	return (true);
}

bool fov_service_can_connect(void *data)
{
	UNUSED_PARAMETER(data);
	debug("FOV service can stream");
	return (true);
}

void fov_service_activate(void *data, obs_data_t *settings)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(settings);
	debug("FOV service is activated");
}
void fov_service_deactivate(void *data)
{
	UNUSED_PARAMETER(data);
	debug("FOV service is deactivated");
}

static const char *fov_service_get_username(void *data)
{
	(void)data;
	debug("FOV service get username");
	return "stream1";
}

static const char *fov_service_get_password(void *data)
{
	(void)data;
	debug("FOV service get password");
	return "stream1";
}

static const char *fov_service_get_connect_info(void *data, uint32_t type)
{
	debug("FOV service get connect info");

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

static void fov_service_apply_encoder_settings(void *data, obs_data_t *video_settings, obs_data_t *audio_settings)
{
	(void)data;
	(void)video_settings;
	(void)audio_settings;
	debug("FOV service apply encoder");
}

struct obs_service_info fov_service = {.id = "flexible_output_view_service",
				       .get_name = fov_service_get_name,
				       .create = fov_service_create,
				       .destroy = fov_service_destroy,
				       .update = fov_service_update,
				       .get_properties = fov_source_properties,
				       .get_protocol = fov_service_get_protocol,
				       .get_url = fov_service_get_url,
				       .can_try_to_connect = fov_service_can_connect,
				       .get_connect_info = fov_service_get_connect_info,
				       .get_username = fov_service_get_username,
				       .get_password = fov_service_get_password,
				       .activate = fov_service_activate,
				       .deactivate = fov_service_deactivate,
				       .apply_encoder_settings = fov_service_apply_encoder_settings,
				       .initialize = fov_service_initialize,
				       .get_key = fov_service_get_key};

bool fov_output_start(void *fov_out_internal)
{
	fov_output_internal_t *fov_out = (fov_output_internal_t *)fov_out_internal;
	obs_output_t *output = fov_out->obs_output_ref;

	fov_out->obs_associated_service_ref = obs_output_get_service(fov_out->obs_output_ref);
	obs_output_begin_data_capture(output, 0);

	debug("FOV output start");
	return (true);
}

void fov_output_stop(void *fov_out_internal, uint64_t ts)
{
	(void)ts;
	fov_output_internal_t *fov_out = (fov_output_internal_t *)fov_out_internal;

	obs_output_signal_stop(fov_out->obs_output_ref, OBS_OUTPUT_SUCCESS);
	debug("FOV output stop");
}

void *fov_output_create(obs_data_t *config, obs_output_t *output)
{
	(void)config;
	debug("FOV output create");

	fov_output_internal_t *fovout = bzalloc(sizeof(fov_output_internal_t));
	fovout->obs_output_ref = output;
	return (fovout);
}

uint64_t fov_output_get_total_bytes(void *fov_out_internal)
{
	(void)fov_out_internal;
	debug("FOV output get total bytes");
	return (0);
}

void fov_output_encoded_packet(void *fov_out_internal, struct encoder_packet *packet)
{
	(void)packet;
	fov_output_internal_t *fov_out = (fov_output_internal_t *)fov_out_internal;

	(void) fov_out;
	// Comment envoyer les packets au service ?
	// Est-ce le role de l'ouput d'envoyer les packets ou du service ?

	// fov_service_internal_t *fov_service =
		// (fov_service_internal_t *)(fov_out->obs_associated_service_ref);

	// fov_service->test_callback();

	debug("FOV output receives packet !!! ");
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

static void fov_start_output(void *fov_out_internal, calldata_t *calldata)
{
	(void)fov_out_internal;
	(void)calldata;

	debug("FOV output start");
}

static bool fov_list_sources(void *fov_out_internal, obs_source_t *source)
{
	(void)fov_out_internal;
	debug("fov_list_sources FOV Source: %s", obs_source_get_name(source));

	return (true);
}

static void frontend_event(enum obs_frontend_event event, void *data)
{
	static bool created = false;
	(void)data;

	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING || event == OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED) {
		debug("--- FOV Enum sources ---");
		obs_enum_sources(fov_list_sources, NULL);
	}

	if (event == OBS_FRONTEND_EVENT_STUDIO_MODE_ENABLED) {
		debug("--- FOV Studio mode enabled ---");

		// Start FOV
		if (created != true) {
			created = true;

			const char *enc_id = "obs_x264";
			const char *format = "mp4";
			const char *path = "/home/lucas/Desktop";
			const char *filename = "/home/lucas/Desktop/FOVtest.mp4";
			(void)enc_id;
			// Utilisations de l'output standard d'OBS:
			// fov_app.fov_out = obs_output_create("ffmpeg_muxer", "FOV ffmpeg", NULL, NULL);
			// obs_encoder_t *audio_encoder = obs_audio_encoder_create("ffmpeg_aac", "FOVAudio", NULL, 0, NULL);
			// obs_encoder_t *video_encoder = obs_video_encoder_create(enc_id, "FOVSource", NULL,NULL);
			// obs_encoder_update(video_encoder,obs_encoder_defaults("obs_x264"));
			// obs_encoder_set_video(video_encoder, obs_get_video());
			// obs_encoder_set_scaled_size(video_encoder, 1280, 720);
			// obs_encoder_set_frame_rate_divisor(video_encoder, 1);
			// obs_encoder_set_audio(audio_encoder, obs_get_audio());
			// obs_output_set_audio_encoder(fov_app.fov_out, audio_encoder, 0);
			// obs_output_set_video_encoder(fov_app.fov_out, video_encoder);

			fov_app.fov_out = obs_output_create("flexible_output_view_output", "FOV Out", NULL, NULL);
			obs_output_set_service(fov_app.fov_out, fov_app.fov_service);
			obs_encoder_t *audio_encoder =
				obs_audio_encoder_create("ffmpeg_aac", "FOVAudio", NULL, 0, NULL);
			obs_encoder_t *video_encoder = obs_video_encoder_create(enc_id, "FOVSource", NULL, NULL);
			obs_encoder_update(video_encoder, obs_encoder_defaults("obs_x264"));
			obs_encoder_set_video(video_encoder, obs_get_video());
			obs_encoder_set_scaled_size(video_encoder, 1280, 720);
			obs_encoder_set_frame_rate_divisor(video_encoder, 1);
			obs_encoder_set_audio(audio_encoder, obs_get_audio());
			obs_output_set_audio_encoder(fov_app.fov_out, audio_encoder, 0);
			obs_output_set_video_encoder(fov_app.fov_out, video_encoder);

			// Configuration du muxer, sortie vers un fichier
			obs_data_t *muxer_settings = obs_data_create();
			obs_data_set_string(muxer_settings, "path", filename);
			obs_data_set_string(muxer_settings, "directory", path);
			obs_data_set_string(muxer_settings, "format", "mp4");
			obs_data_set_string(muxer_settings, "extension", format);
			obs_output_update(fov_app.fov_out, muxer_settings);
			obs_output_initialize_encoders(fov_app.fov_out, 0);
			obs_output_start(fov_app.fov_out);

			// obs_output_begin_data_capture(fov_app.fov_out, 0);
			// obs_source_create
			// obs_view_create()
			// obs_view_add2()
			// obs_view_set_source()
			// obs_source_inc_showing()
			// obs_view_set_source()
			// obs_get_video_info()
		}
	}
}

bool obs_module_load(void)
{
	debug("Le module FOV est charge !");
	obs_register_output(&fov_output);
	obs_register_service(&fov_service);

	fov_app.fov_service = obs_service_create("flexible_output_view_service", "FOV Service", NULL, NULL);

	obs_frontend_add_event_callback(frontend_event, &fov_app);

	return true;
}

void obs_module_unload()
{
	obs_output_end_data_capture(fov_app.fov_out);
	obs_output_signal_stop(fov_app.fov_out, OBS_OUTPUT_SUCCESS);
	obs_output_stop(fov_app.fov_out);
	obs_service_release(fov_app.fov_service);
	obs_output_release(fov_app.fov_out);

	debug("FOV module decharge");
}
