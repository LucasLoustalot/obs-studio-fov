/**
 * @file fov.c
 * @author The FOV Team
 * @brief Entry point for flexible output view
 * @version 0.1
 * @date 2025-09-13
 */

#include <obs-module.h>

#define blog(log_level, format, ...) \
	blog(log_level, "[flexible-output-view: '%s'] " format, ##__VA_ARGS__)

#define debug(format, ...) blog(LOG_DEBUG, format, ##__VA_ARGS__)
#define info(format, ...) blog(LOG_INFO, format, ##__VA_ARGS__)
#define warn(format, ...) blog(LOG_WARNING, format, ##__VA_ARGS__)


OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("flexible-output-view", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "The flexible output view system";
}

bool obs_module_load(void)
{
    debug("", "Le module FOV est chargé !");

	// obs_enum_sources(bool (*enum_proc)(void *, obs_source_t *), void *param)

	// obs_output_create(const char *id, const char *name, obs_data_t *settings, obs_data_t *hotkey_data)

	return true;
}
