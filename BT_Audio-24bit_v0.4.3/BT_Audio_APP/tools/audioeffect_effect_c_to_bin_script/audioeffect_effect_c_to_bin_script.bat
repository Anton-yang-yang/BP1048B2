
cd ..\..

cd tools\audioeffect_effect_c_to_bin_script

audioeffect_effect_c_to_bin_script.exe


copy target_file\audio_effect.bin audio_effect.bin

copy audio_effect.bin ..\merge_script\audio_effect.bin

del target_file\audio_effect.bin


