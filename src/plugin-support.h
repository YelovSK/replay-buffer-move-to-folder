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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <windows.h>
#include <psapi.h>

extern const char *PLUGIN_NAME;
extern const char *PLUGIN_VERSION;

void obs_log(int log_level, const char *format, ...);
extern void blogva(int log_level, const char *format, va_list args);

int get_active_window_name(wchar_t *window_name, int window_name_size);
BOOL is_ignored_path(const wchar_t  *path);
BOOL is_ignored_name(const wchar_t  *name);
int get_executable_path(HWND hwnd, wchar_t *buffer, int buffer_size);
BOOL move_file_to_new_location(const wchar_t *source_file_path, const wchar_t *exe_file_path);
void replace_char(wchar_t *str, wchar_t target, wchar_t replacement);

#ifdef __cplusplus
}
#endif
