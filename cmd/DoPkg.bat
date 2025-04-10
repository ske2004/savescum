@echo off
if not exist pkg mkdir pkg
if not exist pkg\windows mkdir pkg\windows
rmdir /s /q pkg\windows
mkdir pkg\windows
robocopy data pkg\windows\data /S /E
robocopy src pkg\windows\src /S /E
robocopy umbox pkg\windows\umbox /S /E
copy main_game.um pkg\windows\main.um
copy umbox\tophat\tophat.exe pkg\windows
