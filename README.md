# Description

Moves replay buffer recordings into a folder based on the active window.

**Works only on Windows!**

# Install

Download and run the installer from [releases](https://github.com/YelovSK/ReplayBufferMoveOnSave/releases/latest). Windows Defender might complain.

Alternatively download the .zip and move contents into the obs-studio directory.

# Compile

Run `./.github/scripts/Build-Windows.ps1`.

GitHub Actions should automatically build the project and create a release when a tagged commit is pushed.

Gets `Windows.h` and `Psapi.h` from `C:/Program Files (x86)/Windows Kits/10/Include/<version>/um` (see [CMakeLists.txt](CMakeLists.txt)). I am sure there's a better way, feel free to make a PR, I am not familiar with cmake (or C in general). Does compile without it, but then Visual Studio intellisense does not work.
