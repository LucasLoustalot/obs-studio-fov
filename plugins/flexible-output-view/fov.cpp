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

	return true;
}

void obs_module_unload(void)
{
	debug("FOV Module Unloaded");
}

}
