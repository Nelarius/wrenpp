
newoption {
    trigger     = "include",
    value       = "path",
    description = "The location of wren.h"
}

newoption {
    trigger     = "link",
    value       = "path",
    description = "The location of the wren static lib"
}

workspace "wrenpp"
    if _ACTION then
        -- guard this in case the user is calling `premake5 --help`
        -- in which case there will be no action
        location( "build/" .. _ACTION )
    end
    configurations { "Debug", "Release" }

    -- global configuration
    filter "configurations:Debug"
        defines { "DEBUG" }
        flags { "Symbols" }

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    project "lib"
        kind "StaticLib"
        language "C++"
        targetdir "lib/"
        targetname "wrenpp"
        if _OPTIONS["include"] then
            includedirs { _OPTIONS["include"] }
        end
        flags { "C++14" }
        files { "src/**.cpp", "src/**.h" }
        includedirs { "src" }

    project "test"
        kind "ConsoleApp"
        language "C++"
        targetdir "bin"
        targetname "test"
        flags { "C++14" }
        files { "test/**.cpp", "test/***.h", "test/**.wren" }
        includedirs { "src", "test" }
        if _OPTIONS["include"] then
            includedirs { _OPTIONS["include"] }
        end

        filter "files:**.wren"
            buildcommands { "{COPY} ../../test/%{file.name} ../../bin" }
            buildoutputs { "../../bin/%{file.name}" }
            filter {}

        filter "configurations:Debug"
            debugdir "bin"
            filter {}

        filter { "action:vs*", "Debug" }
            if _OPTIONS["link"] then
                libdirs {
                    _OPTIONS["link"] .. "/Debug"
                }
            end
            links { "lib", "wren_static_d" }

        filter { "action:vs*", "Release"}
            if _OPTIONS["link"] then
                libdirs {
                    _OPTIONS["link"] .. "/Release"
                }
            end
            links { "lib", "wren_static" }

        filter { "action:gmake" }
            if _OPTIONS["link"] then
                libdirs {
                    _OPTIONS["link"]
                }
            end
            links { "wren" }
