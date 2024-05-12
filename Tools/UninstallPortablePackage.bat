@echo off

:: Clean key value in regedit
.\MiniThing.exe --clean

:: Clean log & database files
del /s /q MiniThing.log
del /s /q MiniThing.db

:: Clean app
del /s /q MiniThing.exe
del /s /q Logo.ico
del /s /q vc_redist.x64.exe

:: Clean libs
rd /s /q iconengines
rd /s /q imageformats
rd /s /q platforms
rd /s /q styles
rd /s /q translations

del /s /q D3Dcompiler_47.dll
del /s /q libEGL.dll
del /s /q libGLESV2.dll
del /s /q opengl32sw.dll
del /s /q Qt5Core.dll
del /s /q Qt5Gui.dll
del /s /q Qt5Svg.dll
del /s /q Qt5Widgets.dll

pause