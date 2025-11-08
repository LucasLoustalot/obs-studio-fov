/**
 * @file fov.c
 * @author The FOV Team
 * @brief Entry point for flexible output view
 * @version 0.1
 * @date 2025-09-13
 */

#include <obs-frontend-api.h>
#include "callback/signal.h"
#include "media-io/video-io.h"
#include "obs-data.h"
#include "obs.h"
#include "util/base.h"
#include "util/bmem.h"
#include "util/platform.h"
#include <stdlib.h>
#include <obs-module.h>
#include <time.h>

#define debug(format, ...) blog(LOG_DEBUG, "FOV: " format, ##__VA_ARGS__)
#define info(format, ...) blog(LOG_INFO, "FOV: " format, ##__VA_ARGS__)
#define warn(format, ...) blog(LOG_WARNING, "FOV: " format, ##__VA_ARGS__)

static struct fov_system {
	obs_output_t *fov_out;
	obs_encoder_group_t *encoder_group;
} fov_app;

typedef struct fov_output_internal_s {
	obs_output_t *obs_output_ref;
} fov_output_internal_t;

OBS_DECLARE_MODULE()

MODULE_EXPORT const char *obs_module_description(void)
{
	return "The flexible output view system";
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

			static const int nbVideoEncodeurs = 3;
			static const int nbAudioEncodeurs = 1;
			static const char *v_enc_id = "obs_x264";
			static const char *a_enc_id = "ffmpeg_aac";
			static const char *format = "mp4";
			static const char *path = "/home/lucas/Desktop";
			static const char *filename = "/home/lucas/Desktop/FOVtest.mp4";
			static const int width = 1920;
			static const int height = 1080;

			(void)v_enc_id;

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

			fov_app.fov_out = obs_output_create("mp4_output", "FOV mp4 multitrack video", NULL, NULL);
			// Utiliser RTMP: obs_output_create("rtmp_output", "FOV Multitrack Video Out", NULL, NULL);


			// Configuration du muxer, sortie vers un fichier
			obs_data_t *muxer_settings = obs_data_create();
			obs_data_set_string(muxer_settings, "path", filename);
			obs_data_set_string(muxer_settings, "directory", path);
			obs_data_set_string(muxer_settings, "format", "mp4");
			obs_data_set_string(muxer_settings, "extension", format);
			obs_output_update(fov_app.fov_out, muxer_settings);

			// Creation d'un groupe d'encodeur
			fov_app.encoder_group = obs_encoder_group_create();

			for (int i = 0; i < nbVideoEncodeurs; i++) {
				char encoder_name[128];
				obs_data_t *videoEncoderSettings = NULL;
				videoEncoderSettings = obs_data_create();

				obs_data_set_bool(videoEncoderSettings, "disable_scenecut", true);
				// obs_data_set_string(encoderSettings, "type", "???");
				obs_data_set_int(videoEncoderSettings, "width", width);
				obs_data_set_int(videoEncoderSettings, "height", height);
				obs_data_set_int(videoEncoderSettings, "range", VIDEO_RANGE_DEFAULT);
				obs_data_set_int(videoEncoderSettings, "colorspace", VIDEO_CS_DEFAULT);

				snprintf(encoder_name, sizeof(encoder_name), "FOV Multitrack Video Encoder %d", i);
				obs_encoder_t *v_encoder = obs_video_encoder_create(
					v_enc_id, "FOV Multitrack video encoder", videoEncoderSettings, NULL);
				if (!v_encoder) {
					debug("FOV: Failed to create video encoder %d", i);
				}
				obs_encoder_set_video(v_encoder, obs_get_video());

				obs_encoder_set_scaled_size(v_encoder, 1920, 1080);
				obs_encoder_set_frame_rate_divisor(v_encoder, 1);
				obs_encoder_set_group(v_encoder, fov_app.encoder_group);
				obs_output_set_video_encoder2(fov_app.fov_out, v_encoder, i);

				obs_data_release(videoEncoderSettings);
			}

			for (int i = 0; i < nbAudioEncodeurs; i++) {
				obs_data_t *audioEncoderSettings = NULL;
				char encoder_name[128];

				snprintf(encoder_name, sizeof(encoder_name), "FOV Audio Encoder %d", i);
				obs_encoder_t *a_encoder = obs_audio_encoder_create(a_enc_id, "FOV Audio Encoder", audioEncoderSettings, i, NULL);
				if (!a_encoder) {
					debug("FOV: Failed to create audio encoder %d", i);
				}
				obs_encoder_set_audio(a_encoder, obs_get_audio());
				obs_output_set_audio_encoder(fov_app.fov_out, a_encoder, i);

				obs_data_release(audioEncoderSettings);
			}

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
	debug("Le module FOV - test 44 est charge !");

	obs_frontend_add_event_callback(frontend_event, &fov_app);

	return true;
}

void obs_module_unload()
{
	obs_output_end_data_capture(fov_app.fov_out);
	obs_output_signal_stop(fov_app.fov_out, OBS_OUTPUT_SUCCESS);
	obs_output_stop(fov_app.fov_out);

	debug("FOV module decharge");
}
