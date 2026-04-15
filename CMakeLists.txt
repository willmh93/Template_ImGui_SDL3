cmake_minimum_required(VERSION 3.21)
project(CGui LANGUAGES CXX)

set(APP_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

file(GLOB APP_HEADERS CONFIGURE_DEPENDS "${APP_INCLUDE_DIR}/*.h")
file(GLOB APP_SOURCE CONFIGURE_DEPENDS "${APP_INCLUDE_DIR}/*.cpp")

file(GLOB BACKEND_HEADERS CONFIGURE_DEPENDS "${APP_INCLUDE_DIR}/backend/*.h")
file(GLOB BACKEND_SOURCE CONFIGURE_DEPENDS "${APP_INCLUDE_DIR}/backend/*.cpp")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
source_group("backend" FILES ${BACKEND_HEADERS} ${BACKEND_SOURCE})

add_executable(CGui
    ${APP_HEADERS}
    ${APP_SOURCE}
    ${BACKEND_HEADERS}
    ${BACKEND_SOURCE}
)

target_compile_features(CGui PRIVATE cxx_std_23)
target_include_directories(CGui PRIVATE ${APP_INCLUDE_DIR})

find_package(Freetype CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(NanoSVG CONFIG REQUIRED)
find_package(WebP CONFIG REQUIRED)
find_package(Stb REQUIRED)

target_link_libraries(CGui PRIVATE 
    Freetype::Freetype
    SDL3::SDL3
    glad::glad
    imgui::imgui
    NanoSVG::nanosvg NanoSVG::nanosvgrast
    WebP::webpdemux
)

target_include_directories(CGui PRIVATE ${Stb_INCLUDE_DIR})

add_custom_command(TARGET CGui PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/data"
        "$<TARGET_FILE_DIR:CGui>/data"
)

if (MSVC)
	set_target_properties(CGui PROPERTIES WIN32_EXECUTABLE TRUE)
elseif (EMSCRIPTEN)
	set(APP_DATA_DIR "${CMAKE_BINARY_DIR}/app/data")
	message(STATUS "APP_DATA_DIR: ${APP_DATA_DIR}")
	
	target_link_options(CGui PRIVATE "--embed-file=${CMAKE_BINARY_DIR}/data@/data")
	target_link_options(CGui PRIVATE "--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/static/shell.html")
	
	set_target_properties(CGui PROPERTIES
		OUTPUT_NAME "index"
		SUFFIX ".html"
	)
endif()

if (MSVC)
	target_compile_options(CGui PRIVATE /MP) # Multi-threaded compilation
	target_compile_options(CGui PRIVATE /permissive- /W3 /WX /utf-8 /D_CRT_SECURE_NO_WARNINGS)
elseif (EMSCRIPTEN)
	# O3 for WASM
	target_compile_options(CGui PRIVATE 
		-O3
		-sUSE_PTHREADS=1
		-pthread
		-matomics
		-mbulk-memory

		$<$<CONFIG:RelWithDebInfo>:-g2> # keep DWARF + names in release
		$<$<CONFIG:RelWithDebInfo>:-ffunction-sections>
		$<$<CONFIG:RelWithDebInfo>:-fdata-sections>
	)

	target_link_options(CGui PRIVATE
		-sUSE_SDL=3
		-sUSE_WEBGL2=1
		-sFULL_ES3=1
		-sALLOW_MEMORY_GROWTH=1
		-sUSE_PTHREADS=1
		-sPTHREAD_POOL_SIZE_STRICT=0
		-sPTHREAD_POOL_SIZE=32
		-sPTHREADS_DEBUG=1
		-sEXPORTED_RUNTIME_METHODS=[ccall,UTF8ToString,stringToUTF8,lengthBytesUTF8]

		# Prefer this OFF in Release (it bloats logs/binary)
		-sPTHREADS_DEBUG=0

		# Names + demangling for analysis
		$<$<CONFIG:RelWithDebInfo>:--profiling-funcs>

		# GC unreferenced sections
		$<$<CONFIG:RelWithDebInfo>:-Wl>
		$<$<CONFIG:RelWithDebInfo>:--gc-sections>
		$<$<CONFIG:RelWithDebInfo>:-flto>
		$<$<CONFIG:RelWithDebInfo>:-sASSERTIONS>
	)
endif()