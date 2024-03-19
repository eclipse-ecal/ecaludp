mkdir _build
cd _build

cmake .. -A x64 -DCMAKE_PREFIX_PATH="../../_build_static/_install" -DINTEGRATION_TEST_INCLUDE_UDPCAP=ON
cd ..
pause