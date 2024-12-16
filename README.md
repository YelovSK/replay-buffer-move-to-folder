# Description

Moves replay buffer recordings into a folder based on the active window.

**Works only on Windows!**

# Install

Download the .dll from [releases](https://github.com/YelovSK/ReplayBufferMoveOnSave/releases/latest) and place it in `obs-studio\obs-plugins\64bit`.

# Compile

Run `./.github/scripts/Build-Windows.ps1`.

Gets `Windows.h` and `Psapi.h` from `C:/Program Files (x86)/Windows Kits/10/Include/<version>/um` (see [CMakeLists.txt](CMakeLists.txt)). I am sure there's a better way, feel free to make a PR, I am not familiar with cmake. Does compile without it, but then Visual Studio intellisense does not work.
