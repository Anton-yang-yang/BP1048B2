
cd ..\..

cd tools\audioeffect_list_to_parameters_script
copy  source_file\comm_param.c  
copy  ..\..\middleware\audio\inc\audio_effect_library.h audio_effect_library.h
audioeffect_list_to_parameters_script.exe

copy *.c target_file\*.c
copy *.h target_file\*.h

del *.c
del *.h

copy target_file\codec_default_parameters.c codec_default_parameters.c

del target_file\codec_default_parameters.c
del target_file\audio_effect_library.h
del target_file\comm_param.c
del target_file\MM_*.c


