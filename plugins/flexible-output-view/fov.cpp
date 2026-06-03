/**
 * @file fov.cpp
 * @author The FOV Team
 * @brief Implementation of the module entry point and lifecycle management functions for the Flexible Output View (FOV) plugin.
 * @version 0.2
 * @date 2025-09-13
 */

#include <obs-frontend-api.h>

#include "fov_service.hpp"

extern "C" {

OBS_DECLARE_MODULE();

/**
 * @brief Get the description string of the OBS module.
 * @return const char* Pointer to a null-terminated string containing the module description text.
 */
MODULE_EXPORT const char *obs_module_description(void)
{
	return "The flexible output view system";
}

/**
 * @brief Initialize and load the OBS module upon plugin activation.
 * @return bool Returns true if internal module initialization and custom service registration succeeded.
 */
bool obs_module_load(void)
{
	registerFOVService();

	return true;
}

/**
 * @brief Unload the OBS module and release allocated resources prior to framework shutdown.
 */
void obs_module_unload(void)
{
	return;
}
}
