@echo off
cd ..
if exist mp2 (rd /s /q mp2)
md mp2
cd remind_res
for %%i in (16K//*.mp3) do (..\tools\remind_script\ffmpeg.exe -i 16K//%%i -ab 24000 -ac 1 -ar 16000 ..\mp2//%%i.mp2)
cd ..
for %%i in (remind_res//*.mp3) do (tools\remind_script\ffmpeg.exe -i remind_res//%%i -ab 48000 -ac 1 mp2//%%i.mp2)
cd mp2
ren *.mp2 *.
cd ..
tools\remind_script\MergeAudio2BinNew.exe -a 0x0 -i ..\..\mp2
rd /s /q mp2
cd tools\sys_parameter_script
copy ..\..\app_src\system_config\parameter.ini parameter.ini
sys_parameter parameter.ini sys_parameter.h sys_parameter.bin
del parameter.ini
cd ..
del ..\Release\output\*.img
del ..\Release\output\main_merge.mva
..\tools\merge_script\merge.exe
..\tools\merge_script\Andes_MVAGenerate.exe
if not exist ..\Release\output\BT_Audio_APP.bin (del ..\Release\output\main_merge.mva)
del ..\Release\output\noSDKData.bin
pause
