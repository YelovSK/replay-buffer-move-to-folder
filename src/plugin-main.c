/*
Replay Buffer - move to folder
Copyright (C) 2024 Yelov

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <window-utils.h>
#include <string-utils.h>
#include <common.h>

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/dstr.h>
#include <plugin-support.h>

#include <windows.h>
#include <psapi.h>

// Declarations
int move_file_to_new_location(const wchar_t *source_file_path, const wchar_t *exe_file_path);
void on_frontend_event(enum obs_frontend_event event, void *data);

#define TIMER_START \
	LARGE_INTEGER frequency; \
	LARGE_INTEGER t1, t2; \
	double interval; \
	QueryPerformanceFrequency(&frequency); \
	QueryPerformanceCounter(&t1);

#define TIMER_STOP \
	QueryPerformanceCounter(&t2); \
	interval = (float)(t2.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart;\
	obs_log(LOG_INFO, "Done in %f milliseconds", interval);


OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

bool obs_module_load(void)
{
	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	obs_frontend_add_event_callback(on_frontend_event, NULL);
	return TRUE;
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "plugin unloaded");
	obs_frontend_remove_event_callback(on_frontend_event, NULL);
}

static void on_frontend_event(enum obs_frontend_event event, void *data)
{
	if (event != OBS_FRONTEND_EVENT_REPLAY_BUFFER_SAVED)
		return;

	TIMER_START

	// Replay path
	char *replay_path = obs_frontend_get_last_replay();
	if (!replay_path) {
		obs_log(LOG_ERROR, "Failed to fetch the last replay");
		goto free;
	}

	wchar_t wide_replay_path[MAX_PATH];
	swprintf(wide_replay_path, MAX_PATH, L"%hs", replay_path);
	replace_char(wide_replay_path, L'/', L'\\');

	// Window exe name
	wchar_t window_name[MAX_PATH];
	if (get_active_window_name(window_name, MAX_PATH) == FAILURE) {
		obs_log(LOG_ERROR, "Failed to get active window");
		goto free;
	}

  // Move
	if (move_file_to_new_location(wide_replay_path , window_name) == FAILURE) {
		obs_log(LOG_ERROR, "Failed to move recording");
		goto free;
	}

	TIMER_STOP

free:
	bfree(replay_path);
}

static int move_file_to_new_location(const wchar_t *source_file_path, const wchar_t *window_name)
{
	// 1. Construct the new file path - {source_folder}\{window_name}\{file_name}
	// Get path components
	wchar_t drive[_MAX_DRIVE];
	wchar_t extension[_MAX_EXT];
	wchar_t filename[_MAX_FNAME];
	wchar_t dir[_MAX_DIR];
	_wsplitpath_s(source_file_path, drive, _MAX_DRIVE, dir, _MAX_DIR, filename, _MAX_FNAME, extension, _MAX_EXT);

  // E.g. "C:\Recordings\Game"
	wchar_t new_dir[MAX_PATH];
	swprintf(new_dir, MAX_PATH, L"%ls%ls%ls", drive, dir, window_name);

  // E.g. "C:\Recordings\Game\Video.mp4"
	wchar_t new_file_path[MAX_PATH];
	swprintf(new_file_path, MAX_PATH, L"%ls\\%ls%ls", new_dir, filename, extension);

	// 2. Create the directory if it doesn't exist
	CreateDirectoryW(new_dir, NULL);

	// 3. Move recording
	obs_log(LOG_INFO, "Moving %ls to %ls", source_file_path, new_file_path);
	if (!MoveFileW(source_file_path, new_file_path))
		return FAILURE;

	return SUCCESS;
}
