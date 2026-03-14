#pragma once

#include "BasicOutputHandler.hpp"
#include "obs.h"

#define MAX_VIDEO_TRACKS 10
#define OUT_ALIGN(x, a) (((x)+(a)-1)&~((a)-1))

struct SimpleOutput : BasicOutputHandler {
	OBSEncoder audioStreaming;
	OBSEncoder videoStreaming[MAX_VIDEO_TRACKS];
	OBSEncoder audioRecording;
	OBSEncoder audioArchive;
	OBSEncoder videoRecording;
	OBSEncoder audioTrack[MAX_AUDIO_MIXES];

	std::string videoEncoder;
	std::string videoQuality;
	bool usingRecordingPreset = false;
	bool recordingConfigured = false;
	bool ffmpegOutput = false;
	bool lowCPUx264 = false;

	bool isFOV = false;
	OBSEncoderGroup encoderGroup;
	size_t videoTracks = 1;
	obs_service_t *service = nullptr;
	obs_view_t *views[MAX_VIDEO_TRACKS] {0};
	void fovAddVidTrack(obs_source_t *source);
	void fovAddAudioTrack(obs_source_t *source);
	video_t *videoContext;

	SimpleOutput(OBSBasic *main_);

	int CalcCRF(int crf);

	void UpdateRecordingSettings_x264_crf(int crf);
	void UpdateRecordingSettings_qsv11(int crf, bool av1);
	void UpdateRecordingSettings_nvenc(int cqp);
	void UpdateRecordingSettings_nvenc_hevc_av1(int cqp);
	void UpdateRecordingSettings_amd_cqp(int cqp);
	void UpdateRecordingSettings_apple(int quality);
#ifdef ENABLE_HEVC
	void UpdateRecordingSettings_apple_hevc(int quality);
#endif
	void UpdateRecordingSettings();
	void UpdateRecordingAudioSettings();
	virtual void Update() override;

	void SetupOutputs() override;
	int GetAudioBitrate() const;

	void LoadRecordingPreset_Lossy(const char *encoder);
	void LoadRecordingPreset_Lossless();
	void LoadRecordingPreset();

	void LoadStreamingPreset_Lossy(const char *encoder);

	void UpdateRecording();
	bool ConfigureRecording(bool useReplayBuffer);

	bool IsVodTrackEnabled(obs_service_t *service);
	void SetupVodTrack(obs_service_t *service);

	virtual std::shared_future<void> SetupStreaming(obs_service_t *service,
							SetupStreamingContinuation_t continuation) override;
	virtual bool StartStreaming(obs_service_t *service) override;
	virtual bool StartRecording() override;
	virtual bool StartReplayBuffer() override;
	virtual void StopStreaming(bool force) override;
	virtual void StopRecording(bool force) override;
	virtual void StopReplayBuffer(bool force) override;
	virtual bool StreamingActive() const override;
	virtual bool RecordingActive() const override;
	virtual bool ReplayBufferActive() const override;
};
