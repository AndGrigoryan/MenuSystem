@echo off
REM Run first instance
start "" "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe" ^
  "D:\UnrealProjectsDiskD\_MultiplayerCourse\MenuSystem\MenuSystem.uproject" -game ^

  -AUTH_TYPE=developer -AUTH_LOGIN=localhost:7777 -AUTH_PASSWORD=main

REM Run second instance
start "" "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe" ^
  "D:\UnrealProjectsDiskD\_MultiplayerCourse\MenuSystem\MenuSystem.uproject" -game ^
  -AUTH_TYPE=developer -AUTH_LOGIN=localhost:7777 -AUTH_PASSWORD=test 