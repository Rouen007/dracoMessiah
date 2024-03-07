@echo off
pushd build_dir
call cmake ../ -G "Visual Studio 17 2022" -A x64 -DDRACO_TRANSCODER_SUPPORTED=ON -DMESSIAH_ARGUMENT=ON
popd
pause