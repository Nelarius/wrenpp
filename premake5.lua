
newoption {
    trigger     = "wren",
    value       = "path",
    description = "The location of wren.h"
}

workspace "wrenly"
    if _ACTION then
        -- guard this in case the user is calling `premake5 --help`
        -- in which case there will be no action
        location( "build/" .. _ACTION )
    end
    configurations { "Debug", "Release" }
    
    project "lib"
        kind "StaticLib"
        language "C++"
        targetdir "lib/"
        targetname "wrenly"
        if _OPTIONS["wren"] then
            includedirs { _OPTIONS["wren"] }
        end

        files { "src/**.cpp", "src/**.h" }
        includedirs { "src" }

        filter "configurations:Debug"
            defines { "DEBUG" }
            flags { "Symbols" }

        filter "configurations:Release"
            defines { "NDEBUG" }
            optimize "On"

        filter "action:gmake"
            buildoptions { "-std=gnu++14" }
