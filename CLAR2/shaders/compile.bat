C:\VulkanSDK\1.3.283.0\Bin\glslc.exe shader.vert -o vert.spv
C:\VulkanSDK\1.3.283.0\Bin\glslc.exe shader.frag -o frag.spv

C:\VulkanSDK\1.3.283.0\Bin\glslc.exe grid.vert -o grid.vert.spv
C:\VulkanSDK\1.3.283.0\Bin\glslc.exe grid.frag -o grid.frag.spv

C:\VulkanSDK\1.3.283.0\Bin\glslc.exe particle.vert -o particle.vert.spv
C:\VulkanSDK\1.3.283.0\Bin\glslc.exe particle.frag -o particle.frag.spv
C:\VulkanSDK\1.3.283.0\Bin\glslc.exe particle.comp -o particle.comp.spv

C:\VulkanSDK\1.3.283.0\Bin\glslc.exe raytrace.rchit -o raytrace.rchit.spv --target-env=vulkan1.3
C:\VulkanSDK\1.3.283.0\Bin\glslc.exe raytrace.rgen -o raytrace.rgen.spv --target-env=vulkan1.3
C:\VulkanSDK\1.3.283.0\Bin\glslc.exe raytrace.rmiss -o raytrace.rmiss.spv --target-env=vulkan1.3
C:\VulkanSDK\1.3.283.0\Bin\glslc.exe raytraceShadow.rmiss -o raytraceShadow.rmiss.spv --target-env=vulkan1.3

C:\VulkanSDK\1.3.283.0\Bin\glslc.exe post.vert -o post.vert.spv
C:\VulkanSDK\1.3.283.0\Bin\glslc.exe post.frag -o post.frag.spv

pause