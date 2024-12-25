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

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>
#include <util/dstr.h>

#define SUCCESS 0
#define FAILURE 1

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

// Would ideally use regex, but don't feel like setting up PCRE as a dependency.
static const wchar_t *IGNORED_NAMES[] = {
	L"GameBar",
	L"NVIDIA Overlay",
	L"Widgets",
	L"TabTip",
};

static const wchar_t *IGNORED_PATHS[] = {
	L"C:\\Windows\\",
	L"C:\\Program Files\\Common Files\\microsoft shared\\",
};

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

void on_frontend_event(enum obs_frontend_event event, void *data)
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

bool is_fullscreen(HWND hwnd)
{
	if (!IsWindowVisible(hwnd)) {
		return false;
	}

	if (IsZoomed(hwnd)) {
		return true;
	}

	// Check window dimensions
	WINDOWPLACEMENT wp;
	wp.length = sizeof(WINDOWPLACEMENT);
	if (!GetWindowPlacement(hwnd, &wp)) {
		return false;
	}

	RECT rc_desktop;
	GetClientRect(GetDesktopWindow(), &rc_desktop);

	return (wp.rcNormalPosition.left <= rc_desktop.left &&
		wp.rcNormalPosition.top <= rc_desktop.top &&
		wp.rcNormalPosition.right >= rc_desktop.right &&
		wp.rcNormalPosition.bottom >= rc_desktop.bottom);
}

int get_active_window_name(wchar_t *window_name, int window_name_size)
{
	wchar_t executable_path[MAX_PATH];

	// Window with the highest Z index
	HWND hwnd = GetTopWindow(GetDesktopWindow());

	// Iterate windows from highest Z index to lowest
	do {
		if (!is_fullscreen(hwnd))
			continue;

		if (get_executable_path(hwnd, executable_path, MAX_PATH) == FAILURE)
			continue;

		if (is_ignored_path(executable_path))
			continue;

		// Window name instead of exe name. Not great because some windows have other info, e.g. firefox has the tab name etc.
		// At the same time exe name is also not great because it's not always descriptive, e.g. The Finals is running from Discovery.exe
		//GetWindowTextA(hwnd, buffer, buffer_size);

		_wsplitpath_s(executable_path, NULL, 0, NULL, 0, window_name, window_name_size, NULL, 0);

		if (is_ignored_name(window_name))
			continue;

		return SUCCESS;
	} while (hwnd = GetWindow(hwnd, GW_HWNDNEXT));

	// Default to foreground (active) window
	obs_log(LOG_INFO, "Defaulting to foreground window");
	HWND foreground = GetForegroundWindow();

	if (!foreground)
		return FAILURE;

	if (get_executable_path(foreground, executable_path, MAX_PATH) == FAILURE)
		return FAILURE;

	_wsplitpath_s(executable_path, NULL, 0, NULL, 0, window_name, window_name_size, NULL, 0);

	if (is_ignored_name(window_name))
		return FAILURE;

	return SUCCESS;
 }

int get_executable_path(HWND hwnd, wchar_t *buffer, int buffer_size)
{
	DWORD process_id;
	GetWindowThreadProcessId(hwnd, &process_id);

	HANDLE h_process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process_id);
	if (h_process == NULL) {
		return FAILURE;
	}

	if (GetModuleFileNameExW(h_process, NULL, buffer, MAX_PATH) == 0) {
		CloseHandle(h_process);
		return FAILURE;
	}

	CloseHandle(h_process);

	return SUCCESS;
}

BOOL is_ignored_path(const wchar_t *path)
{
	for (size_t i = 0; i < sizeof(IGNORED_PATHS) / sizeof(IGNORED_PATHS[0]); i++) {
		if (wstrstri(path, IGNORED_PATHS[i])) {
			return TRUE;
		}
	}

	return FALSE;
}

BOOL is_ignored_name(const wchar_t *name)
{
	for (size_t i = 0; i < sizeof(IGNORED_NAMES) / sizeof(IGNORED_NAMES[0]); i++) {
		if (wstrcmpi(name, IGNORED_NAMES[i]) == 0) {
			return TRUE;
		}
	}

	return FALSE;
}

int move_file_to_new_location(const wchar_t *source_file_path, const wchar_t *window_name)
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

void replace_char(wchar_t *str, wchar_t target, wchar_t replacement)
{
	while (*str) {
		if (*str == target) {
			*str = replacement;
		}
		str++;
	}
}
