@echo off
del ..\Release\output\*.img
del ..\Release\output\main_merge.mva
..\tools\merge_script\merge.exe
..\tools\merge_script\Andes_MVAGenerate.exe
if not exist ..\Release\output\BT_Audio_APP.bin (del ..\Release\output\main_merge.mva)
del ..\Release\output\noSDKData.bin