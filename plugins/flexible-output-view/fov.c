/**
 * @file fov.c
 * @author The FOV Team
 * @brief Entry point for flexible output view
 * @version 0.1
 * @date 2025-09-13
 */

#include "obs.h"
#include "util/bmem.h"
#include <stdlib.h>
#include <obs-module.h>
#include <time.h>

#define blog(log_level, format, ...) blog(log_level, "[flexible-output-view: '%s'] " format, ##__VA_ARGS__)

#define debug(format, ...) blog(LOG_DEBUG, format, ##__VA_ARGS__)
#define info(format, ...) blog(LOG_INFO, format, ##__VA_ARGS__)
#define warn(format, ...) blog(LOG_WARNING, format, ##__VA_ARGS__)

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
	debug("", "Creation du service FOV");
	return (bzalloc(10)); // Allouer la structure interne au module ici
}

void fov_service_destroy(void *data)
{
	bfree(data);
	debug("", "Destruction du service FOV");
}

const char *fov_service_get_url(void *data)
{
	(void)data;
	debug("", "FOV address");
	return ("rtmp://localhost");
}

const char *fov_service_get_key(void *data)
{
	(void)data;
	debug("", "FOV get stream key");
	return ("stream0");
}

void fov_service_update(void *data, obs_data_t *settings)
{
	(void) data;
	(void) settings;
	debug("", "UPDATE FOV");
}

const char *fov_service_get_output_type(void *data)
{
	(void) data;
	return ("flexible_output_view_output");
}

const char *fov_service_get_protocol(void *data)
{
	(void) data;
	debug("", "FOV get protocol");
	return ("RTMP");
}

const char **get_supported_video_codecs(void *data)
{
	(void) data;
	static const char *video_codecs[] = {"h264", NULL};
	debug("", "FOV supported video");
	return (video_codecs);
}

const char **get_supported_audio_codecs(void *data)
{
	(void) data;
	static const char *audio_codecs[] = {"opus", NULL};
	debug("", "FOV supported audio");
	return (audio_codecs);
}

static obs_properties_t *my_source_properties(void *data)
{
	debug("", "FOV properties");
	obs_properties_t *ppts = obs_properties_create();
	obs_properties_add_bool(ppts, "my_bool",
					"MyBool");
	UNUSED_PARAMETER(data);
	return (ppts);
}

bool fov_initialize(void *data, obs_output_t *output)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(output);
	debug("", "FOV is ready to stream");
	return (true);
}


bool fov_can_connect(void *data)
{
	UNUSED_PARAMETER(data);
	debug("", "FOV is can stream");
	return (true);
}

void fov_activate(void *data, obs_data_t *settings)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(settings);
	debug("", "FOV is activated");
}
void fov_deactivate(void *data)
{
	UNUSED_PARAMETER(data);
	debug("", "FOV is deactivated");
}

struct obs_service_info fov_service = {.id = "flexible_output_view_service",
					      .get_name = fov_service_get_name,
					      .create = fov_service_create,
					      .destroy = fov_service_destroy,
						  .activate = fov_activate,
						  .deactivate = fov_deactivate,
					      .update = fov_service_update,
						  .get_properties = my_source_properties,
					      .get_url = fov_service_get_url,
						  .get_protocol = fov_service_get_protocol,
						  .get_output_type = fov_service_get_output_type,
						  .get_supported_video_codecs = get_supported_video_codecs,
						  .get_supported_audio_codecs = get_supported_audio_codecs,
						  .can_try_to_connect = fov_can_connect,
						  .initialize = fov_initialize,
					      .get_key = fov_service_get_key};


bool fov_output_start(void *a)
{
	(void) a;

	debug("", "FOV output start");
	return (true);
}

void fov_output_stop(void *a, uint64_t b)
{
	(void) a;
	(void) b;

	debug("", "FOV output stop");
}

void *fov_output_create(obs_data_t *a, obs_output_t *b)
{
	(void) a;
	(void) b;
	debug("", "FOV output create");
	return (bzalloc(10));
}

uint64_t fov_output_get_total_bytes(void *a)
{
	(void) a;
	return (0);
}

void fov_output_encoded_packet(void *data, struct encoder_packet *packet)
{
	(void) data;
	(void) packet;

	debug("", "FOV packet");
	return;
}

static const char *fov_output_protocols[] = {"RTMP", NULL};
struct obs_output_info fov_output = {
        .id                   = "flexible_output_view_output",
        .flags                = OBS_OUTPUT_AV | OBS_OUTPUT_ENCODED | OBS_OUTPUT_SERVICE | OBS_OUTPUT_MULTI_TRACK_AV,
        .get_name             = fov_service_get_name,
        .create               = fov_output_create,
        .destroy              = fov_service_destroy,
        .start                = fov_output_start,
        .stop                 = fov_output_stop,
        .encoded_packet       = fov_output_encoded_packet,
        .get_total_bytes      = fov_output_get_total_bytes,
		.protocols = (const char *) fov_output_protocols,
        .encoded_video_codecs = "h264",
        .encoded_audio_codecs = "aac"
};

obs_service_t *fov = NULL;
obs_output_t *fov_out = NULL;

bool obs_module_load(void)
{
	debug("", "Le module FOV est chargé !");

	obs_register_output(&fov_output);
	obs_register_service(&fov_service);

	fov = obs_service_create("flexible_output_view_service", "FOV Service", NULL, NULL);
	fov_out = obs_output_create("flexible_output_view_output", "FOV Output", NULL, NULL);
	obs_output_set_service(fov_out, fov);


	obs_output_initialize_encoders(fov_out, 0);

	obs_output_set_media(fov_out, obs_get_video(), obs_get_audio());

	// obs_output_set_video_encoder(obs_encoder, obs_encoder_t *encoder)

	if (obs_output_can_begin_data_capture(fov_out, 0)) {
		debug("", "L'output FOV peut commencer la capture :)");
		obs_output_begin_data_capture(fov_out, 0);
		obs_output_start(fov_out);
	} else {
		debug("", "L'output FOV ne peut pas commencer la capture !");
	}


	(void) fov;
	// obs_enum_sources(bool (*enum_proc)(void *, obs_source_t *), void *param)

	// obs_output_create(const char *id, const char *name, obs_data_t *settings, obs_data_t *hotkey_data)

	return true;
}

void obs_module_unload()
{
	obs_output_end_data_capture(fov_out);
	obs_output_signal_stop(fov_out, OBS_OUTPUT_SUCCESS);
	obs_output_stop(fov_out);
	obs_service_release(fov);
	obs_output_release(fov_out);

	debug("", "Unload FOV");
}
