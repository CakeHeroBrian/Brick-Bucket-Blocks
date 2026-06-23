IF NOT EXIST "assets\shaders\bin\" MKDIR "assets\shaders\bin\"

%VULKAN_SDK%\Bin\glslc.exe assets\shaders\simple.vert -o assets\shaders\bin\simple.vert.spv
%VULKAN_SDK%\Bin\glslc.exe assets\shaders\simple.frag -o assets\shaders\bin\simple.frag.spv

pause