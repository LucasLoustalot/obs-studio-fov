/**
 * @file fov.c
 * @author The FOV Team
 * @brief Entry point for flexible output view
 * @version 0.1
 * @date 2025-09-13
 */

#include "obs.h"
#include <obs-module.h>
#include <time.h>

#define blog(log_level, format, ...) blog(log_level, "[flexible-output-view: '%s'] " format, ##__VA_ARGS__)

#define debug(format, ...) blog(LOG_DEBUG, format, ##__VA_ARGS__)
#define info(format, ...) blog(LOG_INFO, format, ##__VA_ARGS__)
#define warn(format, ...) blog(LOG_WARNING, format, ##__VA_ARGS__)

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("flexible-output-view", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "The flexible output view system";
}

// extern struct obs_service_info fov_service;
// extern struct obs_output_info fov_output;

const char *fov_service_get_name(void *type_data)
{
	(void)type_data;
	return ("Flexible Output View Service");
}

void *fov_service_create(obs_data_t *settings, obs_service_t *service)
{
	(void)settings;
	(void)service;
	debug("", "Creation du service FOV");
	return (NULL); // Allouer la structure interne au module ici
}

void fov_service_destroy(void *data)
{
	(void)data;
	debug("", "Destruction du service FOV");
}

const char *fov_service_get_url(void *data)
{
	(void)data;
	return ("rtmp://localhost");
}

const char *fov_service_get_key(void *data)
{
	(void)data;
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
	return ("fov-custom");
}

const char *fov_service_get_protocol(void *data)
{
	(void) data;
	return ("rtmp");
}

const char **get_supported_video_codecs(void *data)
{
	(void) data;
	static const char *video_codecs[] = {"h264", NULL};
	return (video_codecs);
}

const char **get_supported_audio_codecs(void *data)
{
	(void) data;
	static const char *audio_codecs[] = {"opus", NULL};
	return (audio_codecs);
}

static obs_properties_t *my_source_properties(void *data)
{
	debug("", "FOV properties");
	obs_properties_t *ppts = obs_properties_create();
	obs_properties_add_bool(ppts, "my_bool",
					obs_module_text("MyBool"));
	UNUSED_PARAMETER(data);
	return ppts;
}

struct obs_service_info fov_service = {.id = "flexible_output_view_service",
					      .get_name = fov_service_get_name,
					      .create = fov_service_create,
					      .destroy = fov_service_destroy,
					      .update = fov_service_update,
						  .get_properties = my_source_properties,
					      .get_url = fov_service_get_url,
						  .get_protocol = fov_service_get_protocol,
						  .get_output_type = fov_service_get_output_type,
						  .get_supported_video_codecs = get_supported_video_codecs,
						  .get_supported_audio_codecs = get_supported_audio_codecs,
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
	return (NULL);
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

struct obs_output_info fov_output = {
        .id                   = "flexible_output_view_output",
        .flags                = OBS_OUTPUT_AV | OBS_OUTPUT_ENCODED,
        .get_name             = fov_service_get_name,
        .create               = fov_output_create,
        .destroy              = fov_service_destroy,
        .start                = fov_output_start,
        .stop                 = fov_output_stop,
        .encoded_packet       = fov_output_encoded_packet,
        .get_total_bytes      = fov_output_get_total_bytes,
        .encoded_video_codecs = "h264",
        .encoded_audio_codecs = "aac"
};

bool obs_module_load(void)
{
	debug("", "Le module FOV est chargé !");

	obs_register_service(&fov_service);
	obs_register_output(&fov_output);

	debug("", "Le module FOV est chargé 2 !");
	// obs_enum_sources(bool (*enum_proc)(void *, obs_source_t *), void *param)

	// obs_output_create(const char *id, const char *name, obs_data_t *settings, obs_data_t *hotkey_data)

	return true;
}
