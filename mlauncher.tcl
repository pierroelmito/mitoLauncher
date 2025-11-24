
set romDir "$::env(HOME)/Games/Emul/roms"
set coreDir {/usr/lib/libretro}

# ======================================================
# Key bindings
# ======================================================

bind -action Back -key ESCAPE
bind -action Activate -key ENTER
bind -action Toggle -key F
bind -action Up -key UP
bind -action Down -key DOWN
bind -action PageUp -key LEFT
bind -action PageDown -key RIGHT

# ======================================================
# Themes
# ======================================================

proc make_fnt_args { fntName } {
	set fntPath "/usr/share/fonts/TTF/$fntName"
	if { [file exists $fntPath] } {
		return [list -fn $fntPath]
	} else {
		return [list]
	}
}

proc sz_theme { basename args } {
	theme -name "${basename} Small"  -rh 20 -th 18 {*}$args
	theme -name "${basename} Large"  -rh 60 -th 45 {*}$args
	theme -name "${basename} Medium"  -rh 35 -th 27 {*}$args
}

#set fntName {SauceCodeProNerdFontMono-Medium.ttf}
set fntName {Roboto-BlackItalic.ttf}

sz_theme Basic -bg #00001f -fg #ffff7f -mid #7f007f -border #001f3f {*}[make_fnt_args $fntName]
sz_theme Yolo -bg #09373e -fg #dbc6ad -mid #4f3f57 -border #001f83 {*}[make_fnt_args $fntName]
sz_theme Default

# ======================================================
# Platforms & Directories
# ======================================================

proc ra_core { name } {
	global coreDir
	set corePath [file join $coreDir "${name}_libretro.so"]
	return "retroarch -L $corePath \"%P\""
}

proc ra_plaform_dir { id label core ext } {
	global romDir
	platform -name $id -label $label -command [ra_core $core]
	directory -platform $id -dir [file join $romDir $id] -ext $ext
}

if { ![file exist $romDir] } {
	error "$romDir directory not found"
}

# Nintendo
ra_plaform_dir nes {Nintendo - NES} nestopia {nes zip}
ra_plaform_dir snes {Nintendo - Super NES} snes9x {sfc zip}
ra_plaform_dir nds {Nintendo - DS} melonds {nds zip}
ra_plaform_dir gb {Nintendo - Gameboy} gambatte {gb zip}
ra_plaform_dir gbc {Nintendo - Gameboy Color} gambatte {gbc zip}
ra_plaform_dir gba {Nintendo - Gameboy Advance} mgba {gba zip}
# Sega
ra_plaform_dir megadrive {Sega - Megadrive} picodrive {md bin zip}
ra_plaform_dir mastersystem {Sega - Master System} picodrive {ms zip}
ra_plaform_dir gg {Sega - Game Gear} picodrive {gg zip}
ra_plaform_dir dreamcast {Sega - Dreamcast} flycast {chd}
# Sony
ra_plaform_dir psx {Sony - PlayStation} mednafen_psx {chd m3u}
ra_plaform_dir psp {Sony - PlayStation Portable} ppsspp {cso}
# 3DO
ra_plaform_dir 3do {3DO} opera {chd}

# Others
platform -name dos -label {DOS} -command {cd $(dirname %P) && dosbox --conf %P}
directory -platform dos -dir [file join $romDir dos] -ext {conf}
platform -name wasm4 -label {WASM-4} -command {w4 run-native %P}
directory -platform wasm4 -dir "$::env(HOME)/Dev/git/wasm4/site/static/carts" -ext wasm -image {%d/%f.png} 
platform -name tic80 -label {TIC80} -command {tic80 %P}
directory -platform tic80 -dir [file join $romDir tic80] -ext tic

