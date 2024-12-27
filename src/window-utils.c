#include <common.h>
#include <window-utils.h>

#include <windows.h>
#include <psapi.h>

#include <util/dstr.h>
#include <plugin-support.h>

// Declarations
bool is_ignored_path(const wchar_t *path);
bool is_ignored_name(const wchar_t *name);
int get_executable_path(HWND hwnd, wchar_t *buffer, int buffer_size);
bool is_fullscreen(HWND hwnd);

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

static bool is_fullscreen(HWND hwnd)
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

static int get_executable_path(HWND hwnd, wchar_t *buffer, int buffer_size)
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

static bool is_ignored_path(const wchar_t *path)
{
	for (size_t i = 0; i < sizeof(IGNORED_PATHS) / sizeof(IGNORED_PATHS[0]); i++) {
		if (wstrstri(path, IGNORED_PATHS[i])) {
			return true;
		}
	}

	return false;
}

static bool is_ignored_name(const wchar_t *name)
{
	for (size_t i = 0; i < sizeof(IGNORED_NAMES) / sizeof(IGNORED_NAMES[0]); i++) {
		if (wstrcmpi(name, IGNORED_NAMES[i]) == 0) {
			return true;
		}
	}

	return false;
}
