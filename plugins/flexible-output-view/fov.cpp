/**
 * @file fov.cpp
 * @author The FOV Team
 * @brief Entry point for flexible output view
 * @version 0.2
 * @date 2025-09-13
 */

#include <obs-frontend-api.h>

#include "fov.hpp"
#include "fov_service.hpp"
#include "util/base.h"

static std::unique_ptr<FOVSystem> fov;

static void frontend_event_callback(enum obs_frontend_event event, void *private_data)
{
	(void)private_data;

	if (event == OBS_FRONTEND_EVENT_EXIT) {
		if (fov) {
			fov->stop();
			return;
		}
	}

	if (event == OBS_FRONTEND_EVENT_STUDIO_MODE_DISABLED || event == OBS_FRONTEND_EVENT_STUDIO_MODE_ENABLED) {
		try {

			if (fov->isInit()) {
				if (fov->isStarted()) {
					info("FOV System stopping...");
					fov->stop();
				} else {
					debug("FOV System starting...");
					fov->start();
				}

			} else {
				fov->setVideoSettings({"obs_x264", 30});
				fov->setAudioSettings({"ffmpeg_aac"});
				fov->setSRTURL("srt://127.0.0.1:9999?mode=caller");
				fov->setBackendURL("http://localhost:8000");

				fov->init();
				blog(LOG_DEBUG,"FOV System init");

				auto add_source_proc = [](void *data, obs_source_t *source) {
					auto *system = static_cast<FOVSystem *>(data);
					system->addSource(source);
					return true;
				};
				obs_enum_sources(add_source_proc, fov.get());

				blog(LOG_DEBUG, "FOV System init successfully with frontend load.");
			}
		} catch (const std::exception &e) {
			blog(LOG_DEBUG,"Failed to start FOV System: %s", e.what());
		}
	}
}

extern "C" {

OBS_DECLARE_MODULE();

MODULE_EXPORT const char *obs_module_description(void)
{
	return "The flexible output view system";
}

bool obs_module_load(void)
{
	debug("FOV Module Loading...");
	registerFOVService();

	obs_frontend_add_event_callback(frontend_event_callback, nullptr);
	fov = std::make_unique<FOVSystem>();
	return true;
}

void obs_module_unload(void)
{
	fov->stop();

	debug("FOV Module Unloaded");
}
}