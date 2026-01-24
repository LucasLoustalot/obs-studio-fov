/**
 * @file fov.c
 * @author The FOV Team
 * @brief Entry point for flexible output view
 * @version 0.1
 * @date 2025-09-13
 */

#include <obs-frontend-api.h>
#include "media-io/video-io.h"
#include "obs-data.h"
#include "obs-source.h"
#include "obs.h"
#include <stdlib.h>
#include <obs-module.h>
#include <time.h>

#define debug(format, ...) blog(LOG_DEBUG, "FOV: " format, ##__VA_ARGS__)
#define info(format, ...) blog(LOG_INFO, "FOV: " format, ##__VA_ARGS__)
#define warn(format, ...) blog(LOG_WARNING, "FOV: " format, ##__VA_ARGS__)
#define OUT_ALIGN(x, a) (((x)+(a)-1)&~((a)-1))

static struct fov_system {
	obs_output_t *fov_out;
	obs_service_t *fov_service;
	obs_encoder_group_t *encoder_group;
	obs_view_t **source_views;
	obs_source_t **source_refs;
	video_t **source_video_context;
	int nb_sources;
} fov_app;

// static const int nbVideoEncodeurs = 3;
static const int nbAudioEncodeurs = 1;
static const char *v_enc_id = "obs_x264";
static const char *a_enc_id = "ffmpeg_aac";
// static const char *format = "mp4";
// static const char *path = "/home/lucas/Desktop";
// static const char *filename = "/home/lucas/Desktop/FOVtest.mp4";
static const char *rtmp_url = "srt://127.0.0.1:9999?mode=listener";
// static const char *rtmp_url = "srt://127.0.0.1:8890?streamid=publish:mystream";
// publish:mystream
static const char *rtmp_stream_key = "publish:mystream";
// static const int default_width = 1920;
// static const int default_height = 1080;
static const int debug_framerate = 30;
// static const int debug_bitrate = 5000;

typedef struct fov_output_internal_s {
	obs_output_t *obs_output_ref;
} fov_output_internal_t;

OBS_DECLARE_MODULE()

MODULE_EXPORT const char *obs_module_description(void)
{
	return "The flexible output view system";
}

static bool fov_setup_source_view(void *fov_out_internal, obs_source_t *source)
{
	(void)fov_out_internal;
	debug("FPV trying to setup: %s", obs_source_get_name(source));

	if (source != NULL && obs_source_get_type(source) == OBS_SOURCE_TYPE_INPUT) {

		if (obs_source_get_output_flags(source) & OBS_SOURCE_VIDEO) {

			struct obs_video_info ovi = {0};
			obs_get_video_info(&ovi);
			ovi.output_width = OUT_ALIGN(obs_source_get_width(source), 16);
			ovi.output_height = OUT_ALIGN(obs_source_get_height(source), 16);
			ovi.base_width = OUT_ALIGN(obs_source_get_base_width(source), 16);
			ovi.base_height = OUT_ALIGN(obs_source_get_base_height(source), 16);
			ovi.fps_den = 1;
			ovi.fps_num = debug_framerate;
			ovi.colorspace = VIDEO_CS_DEFAULT;
			ovi.range = VIDEO_RANGE_DEFAULT;

			debug("FOV setup video source views: %s", obs_source_get_name(source));

			fov_app.nb_sources += 1;
			int current_idx = fov_app.nb_sources - 1;

			fov_app.source_views =
				realloc(fov_app.source_views, (sizeof(obs_view_t *) * fov_app.nb_sources));
			fov_app.source_video_context =
				realloc(fov_app.source_video_context, (sizeof(video_t *) * fov_app.nb_sources));
			fov_app.source_refs =
				realloc(fov_app.source_refs, (sizeof(obs_source_t *) * fov_app.nb_sources));

			if (!fov_app.source_views || !fov_app.source_video_context || !fov_app.source_refs) {
				debug("FOV: Failed to realloc for %s. Out of memory.", obs_source_get_name(source));

				// TODO handle error
				return (false);
			}

			debug("FOV creating view for : %s", obs_source_get_name(source));
			fov_app.source_refs[current_idx] = obs_source_get_ref(source);

			fov_app.source_views[current_idx] = obs_view_create();
			if (fov_app.source_views[current_idx] == NULL) {
				debug("FOV: Failed to create source view for %s", obs_source_get_name(source));
				return (false);
			}

			debug("FOV view set source for  %s", obs_source_get_name(source));
			obs_view_set_source(fov_app.source_views[current_idx], current_idx, source);

			debug("FOV add view to rendering pipeline for source %s", obs_source_get_name(source));
			fov_app.source_video_context[current_idx] =
				obs_view_add2(fov_app.source_views[current_idx], &ovi);
		}
	}

	return (true);
}

static void frontend_event(enum obs_frontend_event event, void *data)
{
	static bool created = false;
	(void)data;

	if (event == OBS_FRONTEND_EVENT_STUDIO_MODE_ENABLED) {
		debug("--- FOV Studio mode enabled ---");
		debug("--- FOV Setup sources ---");
		obs_enum_sources(fov_setup_source_view, NULL);

		// Start FOV
		if (created != true) {
			created = true;

			fov_app.fov_out = obs_output_create("fov_output", "rtmp multitrack video", NULL, NULL);

			obs_data_t *service_data = obs_data_create();
			obs_data_set_string(service_data, "server", rtmp_url);
			obs_data_set_string(service_data, "url", rtmp_url);
			obs_data_set_string(service_data, "key", rtmp_stream_key);
			// obs_data_set_string(service_data, "bearer_token", rtmp_stream_key);

			fov_app.fov_service = obs_service_create("rtmp_custom", "multitrack video service", service_data, NULL);
			obs_service_update(fov_app.fov_service, service_data);
			obs_output_set_service(fov_app.fov_out, fov_app.fov_service);
			obs_data_release(service_data);

			// Configuration du muxer, sortie vers un fichier
			obs_data_t *muxer_settings = obs_data_create();
			obs_data_set_string(muxer_settings, "url", rtmp_url);
			obs_data_set_string(muxer_settings, "path", rtmp_url);
			obs_data_set_int(muxer_settings, "video_track_count", fov_app.nb_sources);
			// obs_data_set_string(muxer_settings, "directory", path);
			// obs_data_set_string(muxer_settings, "format", "fmp4");
			// obs_data_set_string(muxer_settings, "extension", format);
			obs_output_update(fov_app.fov_out, muxer_settings);

			// Creation d'un groupe d'encodeur
			fov_app.encoder_group = obs_encoder_group_create();

			// Pour chaque source
			for (int i = 0; i < fov_app.nb_sources; i++) {
				char encoder_name[128];
				obs_data_t *videoEncoderSettings = obs_encoder_defaults(v_enc_id);
				// obs_data_set_int(videoEncoderSettings, "keyint", 60);
				// obs_data_set_string(videoEncoderSettings, "rate_control", "CBR");
				// obs_data_set_int(videoEncoderSettings, "bitrate", debug_bitrate);
				// obs_data_set_bool(videoEncoderSettings, "disable_scenecut", true);

				// uint32_t width = obs_source_get_base_width(fov_app.source_refs[i]);
				// uint32_t height = obs_source_get_base_height(fov_app.source_refs[i]);
				// if (width == 0 || height == 0) {
				// 	width = default_width;
				// 	height = default_height;
				// }

				// debug("FOV: Setting source %s encoder parameters to %dx%d",
				//       obs_source_get_name(fov_app.source_refs[i]), width, height);

				// obs_data_set_int(videoEncoderSettings, "width", width);
				// obs_data_set_int(videoEncoderSettings, "height", height);
				// obs_data_set_int(videoEncoderSettings, "range", VIDEO_RANGE_DEFAULT);
				// obs_data_set_int(videoEncoderSettings, "colorspace", VIDEO_CS_DEFAULT);
				// obs_data_set_int(videoEncoderSettings, "framerate", debug_framerate);
				// obs_data_set_int(videoEncoderSettings, "profile", 77); // AV_PROFILE_H264_MAIN
				// obs_data_set_bool(videoEncoderSettings, "disable_scenecut", true);
				// obs_data_set_int(videoEncoderSettings, "track_index", i);
				// obs_data_set_bool(videoEncoderSettings, "repeat_headers", true);
				// obs_data_set_string(videoEncoderSettings, "header_type", "annexb");

				snprintf(encoder_name, sizeof(encoder_name), "FOV Track %d - %s",i, obs_source_get_name(fov_app.source_refs[i]));
				obs_encoder_t *v_encoder =
				obs_video_encoder_create(v_enc_id, encoder_name, videoEncoderSettings, NULL);
				if (!v_encoder) {
					debug("FOV: Failed to create video encoder %d", i);
				}
				// Passer le video_t de la source
				// obs_encoder_set_scaled_size(v_encoder, width, height);
				obs_encoder_set_video(v_encoder, fov_app.source_video_context[i]);


				obs_encoder_set_frame_rate_divisor(v_encoder, 1);

				// if (i == 0) {
				// 	obs_output_set_video_encoder(fov_app.fov_out, v_encoder);
				// } else {
					obs_encoder_set_group(v_encoder, fov_app.encoder_group);
					obs_output_set_video_encoder2(fov_app.fov_out, v_encoder, i);
				// }



				// static bool isfirst = true;
				// if (isfirst) {
				// 	isfirst = false;
				// 	obs_output_set_video_encoder(fov_app.fov_out, v_encoder);
				// }

				obs_data_release(videoEncoderSettings);
			}

			for (int i = 0; i < nbAudioEncodeurs; i++) {
				obs_data_t *audioEncoderSettings = NULL;
				char encoder_name[128];
				audioEncoderSettings = obs_encoder_defaults(a_enc_id);

				snprintf(encoder_name, sizeof(encoder_name), "FOV Audio Encoder %d", i);
				obs_encoder_t *a_encoder =
					obs_audio_encoder_create(a_enc_id, encoder_name, audioEncoderSettings, i, NULL);
				if (!a_encoder) {
					debug("FOV: Failed to create audio encoder %d", i);
				}
				obs_encoder_set_audio(a_encoder, obs_get_audio());
				obs_output_set_audio_encoder(fov_app.fov_out, a_encoder, i);

				obs_data_release(audioEncoderSettings);
			}
			// Démarrage des encodeurs et de l'output

			obs_output_set_reconnect_settings(fov_app.fov_out, 10, 10);
			obs_output_initialize_encoders(fov_app.fov_out, 0);
			if (obs_output_start(fov_app.fov_out)) {
				obs_output_begin_data_capture(fov_app.fov_out, OBS_OUTPUT_MULTI_TRACK_VIDEO | OBS_OUTPUT_AUDIO);
			} else {
				const char *error = obs_output_get_last_error(fov_app.fov_out);
				debug("FOV failed to start output: %s", error);
			}
		}
	}
}

bool obs_module_load(void)
{
	debug("Le module FOV - test 44 est charge !");

	memset(&fov_app, 0, sizeof(fov_app));
	obs_frontend_add_event_callback(frontend_event, &fov_app);

	return true;
}

void obs_module_unload()
{
	for (int i = 0; i < fov_app.nb_sources; i++) {
		if (fov_app.source_refs[i])
			obs_source_release(fov_app.source_refs[i]);
		if (fov_app.source_views[i])
			obs_view_destroy(fov_app.source_views[i]);
	}
	bfree(fov_app.source_refs);
	bfree(fov_app.source_views);
	bfree(fov_app.source_video_context);

	// Stop output safely
	obs_output_force_stop(fov_app.fov_out);


	debug("FOV module decharge");
}