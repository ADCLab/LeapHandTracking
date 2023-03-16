
@echo off

REM Set variables
SET CUR_DIR=%cd%
SET SRC_DIR=%CUR_DIR%/realtimehandtracking/
SET BUILD_TYPE=Release
SET REPOS_BUILD_ROOT=%CUR_DIR%/build
SET REPOS_INSTALL_ROOT=%CUR_DIR%/exe

REM Run cmake
cmake -S %SRC_DIR% -B %REPOS_BUILD_ROOT%/%BUILD_TYPE%/
    
cmake --build %REPOS_BUILD_ROOT%/%BUILD_TYPE%/ -j --config %BUILD_TYPE%    
