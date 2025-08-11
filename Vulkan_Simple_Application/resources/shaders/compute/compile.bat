slangc.exe shader_compute.slang -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name -entry vertMain -entry fragMain -entry compMain -o slang_compute.spv

PAUSE