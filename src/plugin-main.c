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

static const char *IGNORES[] = {
	"C:\\Windows\\",
	"C:\\Program Files\\Common Files\\microsoft shared\\"
};

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

void on_frontend_event(enum obs_frontend_event event, void *data)
{
	if (event != OBS_FRONTEND_EVENT_REPLAY_BUFFER_SAVED)
		return;

	TIMER_START

	// Get path of the saved replay buffer file
	char replay_path[MAX_PATH];
	get_last_replay_path(replay_path);
	if (!replay_path)
		return;

	// Get path of the current active fullscreen window executable
	char game_path[MAX_PATH];
	if (get_active_window_path(game_path, MAX_PATH) == FAILURE)
		return;

	// Move into a folder based on the window executable name
	if (move_file_to_new_location(replay_path, game_path) == FAILURE)
		return;

	TIMER_STOP
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

const int get_last_replay_path(char *buffer)
{
	obs_output_t *output = obs_frontend_get_replay_buffer_output();
	if (!output) {
		obs_log(LOG_WARNING, "Failed to get replay buffer output");
		return FAILURE;
	}

	calldata_t *calldata = calldata_create();
	if (!calldata) {
		obs_log(LOG_WARNING, "Failed to create calldata");
		obs_output_release(output);
		return FAILURE;
	}

	proc_handler_t *handler = obs_output_get_proc_handler(output);
	if (!handler) {
		obs_log(LOG_WARNING, "Failed to get proc handler");
		calldata_destroy(calldata);
		obs_output_release(output);
		return FAILURE;
	}

	BOOL is_found = proc_handler_call(handler, "get_last_replay", calldata);
	if (!is_found) {
		obs_log(LOG_WARNING, "Failed to get last replay");
		calldata_destroy(calldata);
		obs_output_release(output);
		return FAILURE;
	}

	strcpy(buffer, calldata_string(calldata, "path"));

	calldata_destroy(calldata);
	obs_output_release(output);

	return SUCCESS;
}

bool is_fullscreen(HWND hwnd)
{
	WINDOWPLACEMENT wp;
	wp.length = sizeof(WINDOWPLACEMENT);

	if (!IsWindowVisible(hwnd)) {
		return false;
	}
	if (IsZoomed(hwnd)) {
		return true;
	}
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

BOOL starts_with(const char *str, const char *prefix)
{
	if (!str || !prefix) {
		return FALSE;
	}

	size_t prefix_len = strlen(prefix);
	size_t str_len = strlen(str);
	if (prefix_len > str_len) {
		return FALSE;
	}

	return strncmp(str, prefix, prefix_len) == 0;
}

int get_active_window_path(char *buffer, int buffer_size)
{
	HWND hwnd = NULL;

	while ((hwnd = FindWindowEx(NULL, hwnd, NULL, NULL)) != NULL) {
		if (!is_fullscreen(hwnd))
			continue;

		if (get_executable_path(hwnd, buffer, buffer_size) != 0)
			continue;

		if (is_ignored_path(buffer))
			continue;

		return SUCCESS;
	}

	return FAILURE;
}

int get_executable_path(HWND hwnd, char *buffer, int buffer_size)
{
	DWORD process_id;
	GetWindowThreadProcessId(hwnd, &process_id);

	HANDLE h_process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process_id);
	if (h_process == NULL) {
		return FAILURE;
	}

	TCHAR executable_path[MAX_PATH];
	if (GetModuleFileNameEx(h_process, NULL, executable_path, MAX_PATH) == 0) {
		CloseHandle(h_process);
		return FAILURE;
	}
	CloseHandle(h_process);

	convert_tchar_to_char(executable_path, buffer, buffer_size);

	return SUCCESS;
}

BOOL is_ignored_path(const char *path)
{
	for (size_t i = 0; i < sizeof(IGNORES) / sizeof(IGNORES[0]); i++) {
		if (starts_with(path, IGNORES[i])) {
			return TRUE;
		}
	}
	return FALSE;
}

int move_file_to_new_location(const char *source_file_path,
			      const char *window_file_path)
{
	// 1. Get executable name, e.g. C:\Games\game.exe -> game
	char window_name[MAX_PATH];
	_splitpath_s(window_file_path, NULL, 0, NULL, 0, window_name, MAX_PATH, NULL, 0);
	char *dot_pos = strrchr(window_name, '.');
	// Remove extension
	if (dot_pos) {
		*dot_pos = '\0';
	}

	// 2. Get the replay buffer base folder
	char source_dir[MAX_PATH];
	get_path_without_file(source_file_path, source_dir);

	// 3. Construct the new file path
	char new_dir[MAX_PATH];
	snprintf(new_dir, MAX_PATH, "%s/%s", source_dir, window_name);
	char new_file_path[MAX_PATH];
	snprintf(new_file_path, MAX_PATH, "%s/%s", new_dir,
		 strrchr(source_file_path, '/') + 1);

	// 4. Create the directory if it doesn't exist
	CreateDirectoryA(new_dir, NULL);

	// 5. Move recording
	obs_log(LOG_INFO, "Moving %s to %s", source_file_path, new_file_path);
	if (!MoveFileA(source_file_path, new_file_path)) {
		return FAILURE;
	}

	return SUCCESS;
}

void get_path_without_file(const char *full_path, char *path_without_file)
{
	// Find the last occurrence of the directory separator
	const char *last_slash = strrchr(full_path, '/');
	if (!last_slash) {
		last_slash = strrchr(full_path, '\\');
	}

	size_t length = last_slash - full_path;
	strncpy(path_without_file, full_path, length);
	path_without_file[length] = '\0'; // Null-terminate the string
}

void convert_tchar_to_char(const TCHAR *source, char *dest, size_t dest_size)
{
#ifdef _UNICODE
	wcstombs_s(NULL, dest, dest_size, source, _TRUNCATE);
#else
	strncpy_s(dest, dest_size, source, _TRUNCATE);
#endif
}
