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
    local project_location = ""
    if _ACTION then
        project_location = "build/" .._ACTION
    end

    configurations { "Debug", "Release", "Test" }

    architecture "x86_64"

    -- global configuration
    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    filter "action:vs*"
        defines { "_CRT_SECURE_NO_WARNINGS" }

    filter "not action:vs*"
        buildoptions { "-std=c++14" }

    project "lib"
        location(project_location)
        kind "StaticLib"
        language "C++"
        targetdir "lib/%{cfg.buildcfg}"
        targetname "wrenpp"
        if _OPTIONS["include"] then
            includedirs { _OPTIONS["include"] }
        end
        files { "Wren++.cpp", "Wren++.h" }
        includedirs { "src" }

    project "test"
        location(project_location)
        kind "ConsoleApp"
        language "C++"
        targetdir "bin/%{cfg.buildcfg}"
        targetname "test"
        files { "Wren++.cpp", "test/**.cpp", "test/***.h", "test/**.wren" }
        includedirs { "./", "test" }
        if _OPTIONS["include"] then
            includedirs { _OPTIONS["include"] }
        end
        if _OPTIONS["link"] then
            libdirs {
                _OPTIONS["link"]
            }
        end

        prebuildcommands { "{MKDIR} %{cfg.targetdir}" }

        filter "files:**.wren"
            buildcommands { "{COPY} %{file.abspath} %{cfg.targetdir}" }
            buildoutputs { "%{cfg.targetdir}/%{file.name}" }
        filter {}

        filter "configurations:Debug"
            debugdir "bin/%{cfg.buildcfg}"

        filter { "action:vs*", "Debug" }
            links { "lib", "wren_static_d" }

        filter { "action:vs*", "Release"}
            links { "lib", "wren_static" }

        filter { "not action:vs*" }
            links { "lib", "wren" }
