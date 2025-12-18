find_package(SDL3 REQUIRED CONFIG)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL3_TTF REQUIRED IMPORTED_TARGET sdl3-ttf)

add_executable(ladybird
	quartz.cpp
	engine.cpp
	ui.cpp
	window.cpp
)

target_link_libraries(ladybird PRIVATE
	AK
	LibCore
	LibFileSystem
	LibGfx
	LibImageDecoderClient
	LibMain
	LibURL
	LibWebView
	SDL3::SDL3
	PkgConfig::SDL3_TTF
)

target_include_directories(ladybird PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

create_ladybird_bundle(ladybird)
