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

int get_last_replay_path(char *buffer);
int get_active_window_path(char *buffer, int buffer_size);
BOOL is_ignored_path(const char *path);
int get_executable_path(HWND hwnd, char *buffer, int buffer_size);
BOOL move_file_to_new_location(const char *source_file_path, const char *exe_file_path);
void get_path_without_file(const char *full_path, char *path_without_file);
void convert_tchar_to_char(const TCHAR *source, char *dest, size_t dest_size);

#ifdef __cplusplus
}
#endif
