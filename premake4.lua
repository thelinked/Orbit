solution "Orbit"
   configurations { "Debug", "Release" }

   project "Orbit"
      kind "StaticLib"
      language "C++"
      files { "src/**.hpp", "src/**.cpp" }
 
      configuration "Debug"
         targetdir "bin/debug"
         flags { "Symbols" }
 
      configuration "Release"
         targetdir "bin/release"
         flags { "Optimize" }  
         
