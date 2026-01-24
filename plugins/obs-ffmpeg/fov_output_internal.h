/**
 * @file fov_output_internal.h
 * @author The FOV Team
 * @brief FOV output internal structures definition
 * @version 0.1
 * @date 2026-01-10
 */

#pragma once

#define FOV_NEW_MPEGTS_OUTPUT


#include <obs-module.h>
#include <util/deque.h>
#include <util/threading.h>
#include <util/dstr.h>
#include <util/darray.h>
#include <util/platform.h>

#include "obs-ffmpeg-output.h"
#include "obs-ffmpeg-formats.h"
#include "obs-ffmpeg-compat.h"
#include "obs-ffmpeg-rist.h"
#include "obs-ffmpeg-srt.h"
#include <libavutil/channel_layout.h>
#include <libavutil/mastering_display_metadata.h>

#define do_log(level, format, ...) \
	blog(level, "[obs-fov: '%s']: " format, obs_output_get_name(stream->output), ##__VA_ARGS__)

#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)
#define error(format, ...) do_log(LOG_ERROR, format, ##__VA_ARGS__)

struct fov_ffmpeg_cfg {
	const char *url;
	const char *format_name;
	const char *format_mime_type;
	const char *muxer_settings;
	const char *protocol_settings; // not used yet for SRT nor RIST

	const char *video_encoder;
	int video_encoder_id;
    int video_tracks; // Video multi-track

	const char *audio_encoder;
	int audio_bitrate;
	int audio_encoder_id;
	int audio_bitrates[MAX_AUDIO_MIXES]; // multi-track
	int audio_mix_count;
    int frame_size; // audio frame size
	const char *audio_stream_names[MAX_AUDIO_MIXES];

	const char *username;
	const char *password;
	const char *stream_id;
	const char *encrypt_passphrase;

    bool is_srt;
	bool is_rist;
	int srt_pkt_size;
};

struct fov_ffmpeg_audio_info {
	AVStream *stream;
	AVCodecContext *ctx;
};

struct fov_ffmpeg_data {
	AVStream **videos;
	AVCodecContext **videos_ctx;
    int num_video_tracks;       // Support for multiple video tracks

	struct fov_ffmpeg_audio_info *audio_infos;
	AVFormatContext *output;

	int frame_size;

	uint32_t audio_samplerate;
	enum audio_format audio_format;
	size_t audio_planes;
	size_t audio_size;
	int num_audio_streams;

	/* audio_tracks is a bitmask storing the indices of the mixes */
	struct deque excess_frames[MAX_AUDIO_MIXES][MAX_AV_PLANES];
	uint8_t *samples[MAX_AUDIO_MIXES][MAX_AV_PLANES];
	AVFrame *aframe[MAX_AUDIO_MIXES];

	struct fov_ffmpeg_cfg config;

	bool initialized;

	char *last_error;
};

struct fov_ffmpeg_output {
	obs_output_t *output;
	volatile bool active;
	struct fov_ffmpeg_data ff_data;

	pthread_t start_thread;

	uint64_t total_bytes;

	uint64_t audio_start_ts;
	uint64_t video_start_ts;
	uint64_t stop_ts;
	volatile bool stopping;

	bool write_thread_active;
	pthread_mutex_t write_mutex;
	pthread_t write_thread;
	os_sem_t *write_sem;
	os_event_t *stop_event;

	DARRAY(AVPacket *) packets;

	/* used for SRT & RIST */
	URLContext *h;
	AVIOContext *s;
	bool got_headers;

	volatile bool running;

	pthread_t start_stop_thread;
	pthread_mutex_t start_stop_mutex;
	volatile bool start_stop_thread_active;
	bool has_connected;
};

struct fov_mpegts_cmd {
	enum mpegts_cmd_type type;
	bool signal_stop;
	struct fov_ffmpeg_output *stream;
	uint64_t ts;
};

