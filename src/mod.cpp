#include "SigScan.h"
#include "diva.h"
#include <cstdlib>

using namespace diva;

struct WaitScreenKeys {
	std::optional<std::string> key;
	u64 fallback_offset;
};

WaitScreenKeys keys[11] = {
    // Music, Illustrator, Lyrics, Arranger, Manipulator, PV Editor, Guitar Player, ex_info[4]
    {{}, 0}, {{}, 0}, {{}, 0}, {{}, 0}, {{}, 0}, {{}, 0}, {{}, 0}, {{}, 0x1C8}, {{}, 0x208}, {{}, 0x248}, {{}, 0x288},
};

u64 value_offsets[11] = {
    0xE8, 0x108, 0x128, 0x148, 0x168, 0x188, 0x1A8, 0x1E8, 0x228, 0x268, 0x2A8,
};

std::map<u32, std::string> module_designers;

char modsPrefix[MAX_PATH];

SIG_SCAN (sigLoadStrArray, 0x1402397E0, "\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x60\x48\x8B\x05\xCC\xCC\xCC\xCC\x48\x33\xC4\x48\x89\x44\x24\x50\x48", "xxxxxxxxxxxxx????xxxxxxxxx");

HOOK (void, DisplayWaitScreen, 0x140655B40, u64 a1) {
	if (!*(bool *)(a1 + 0x2D1) || *(u32 *)(a1 + 0x9C) == 0) return;
	AetComposition comp;
	GetComposition (&comp, *(u32 *)(a1 + 0x9C));

	FontInfo font;
	GetLangFont (&font, FontId::BOLD36_DIVA_36_38, true);

	DrawParams params;
	params.font           = &font;
	params.layer          = 0x1D;
	params.resolutionMode = RESOLUTION_MODE_FHD;

	if (auto p_song = comp.find (string ("p_rights_song_lt"))) {
		auto song = (string *)(a1 + 0xC8);

		SetFontSize(&font, p_song.value()->height * p_song.value()->matrix.x.x, p_song.value()->height * p_song.value()->matrix.y.y);
		params.unk_0x00       = p_song.value()->width;
		params.colour[0]      = (u8)(p_song.value()->color[0]);
		params.colour[1]      = (u8)(p_song.value()->color[1]);
		params.colour[2]      = (u8)(p_song.value()->color[2]);
		params.colour[3]      = (u8)(p_song.value()->opacity * 255.0);
		params.lineOrigin     = Vec2 (p_song.value()->position.x - p_song.value()->anchor.x, p_song.value()->position.y - p_song.value()->anchor.y);
		params.textCurrent    = Vec2 (p_song.value()->position.x - p_song.value()->anchor.x, p_song.value()->position.y - p_song.value()->anchor.y);
		diva::DrawTextA(&params, 0x01, song->c_str());
	}

	i32 j = 1;
	char buf[64];
	for (i32 i = 0; i < 11; i++) {
		auto str = (string *)(a1 + value_offsets[i]);
		if (!str->length) continue;

		sprintf (buf, "p_rights_base%02d_c", j);
		if (auto base = comp.find (string (buf))) {
			const char *key_str = nullptr;
			if (keys[i].key.has_value ()) key_str = keys[i].key->c_str ();
			else key_str = ((string *)(a1 + keys[i].fallback_offset))->c_str ();

			SetFontSize(&font, base.value()->height * base.value()->matrix.x.x, base.value()->height * base.value()->matrix.y.y);
			params.unk_0x00       = base.value()->width;
			params.colour[0]      = (u8)(base.value()->color[0]);
			params.colour[1]      = (u8)(base.value()->color[1]);
			params.colour[2]      = (u8)(base.value()->color[2]);
			params.colour[3]      = (u8)(base.value()->opacity * 255.0);
			params.lineOrigin     = Vec2 (base.value()->position.x - base.value()->anchor.x, base.value()->position.y - base.value()->anchor.y);
			params.textCurrent    = Vec2 (base.value()->position.x - base.value()->anchor.x, base.value()->position.y - base.value()->anchor.y);
			diva::DrawTextA(&params, 0x01, key_str);
		}

		sprintf (buf, "p_rights_name%02d_lt", j);
		if (auto name = comp.find (string (buf))) {
			SetFontSize(&font, name.value()->height * name.value()->matrix.x.x, name.value()->height * name.value()->matrix.y.y);
			params.unk_0x00       = name.value()->width;
			params.colour[0]      = (u8)(name.value()->color[0]);
			params.colour[1]      = (u8)(name.value()->color[1]);
			params.colour[2]      = (u8)(name.value()->color[2]);
			params.colour[3]      = (u8)(name.value()->opacity * 255.0);
			params.lineOrigin     = Vec2 (name.value()->position.x - name.value()->anchor.x, name.value()->position.y - name.value()->anchor.y);
			params.textCurrent    = Vec2 (name.value()->position.x - name.value()->anchor.x, name.value()->position.y - name.value()->anchor.y);
			diva::DrawTextA(&params, 0x01, str->c_str());
		}
		j++;
	}
}

HOOK (void, DisplayModuleSelect, 0x1406929A0, u64 a1) {
	originalDisplayModuleSelect (a1);

	i32 selectedModuleIndex = *(i32 *)(a1 + 0x110);
	auto modules            = *(vector<ModuleData *> **)(a1 + 0x1E8 + 0xF0);
	auto ppmodule           = modules->at (selectedModuleIndex);
	auto module             = **ppmodule;
	if (!module_designers.contains (module->id) || !CheckModuleUnlocked (FindModule (GetSaveData (), module->id))) return;

	u64 layer_hash = *(u64 *)(a1 + 0x90);
	auto layers    = (map<u64, AetLayerArgs *> *)(*(u64 *)(*(u64 *)(a1 + 0x08) + 0x958) + 0x10);
	auto pplayer   = layers->find (layer_hash);
	if (!pplayer.has_value ()) return;
	auto layer = **pplayer;

	AetComposition comp;
	GetComposition (&comp, layer->id);
	auto pplaceholder = comp.find ("p_designer_name_c");
	if (!pplaceholder.has_value ()) return;
	auto placeholder = *pplaceholder;

	FontInfo font;
	GetLangFont (&font, FontId::FNT_36_DIVA_36_38, true);

	DrawParams params;
	params.unk_0x00       = placeholder->width;
	params.font           = &font;
	params.layer          = 0x10;
	params.resolutionMode = RESOLUTION_MODE_FHD;
	params.colour[0]      = 0xFF;
	params.colour[1]      = 0xFF;
	params.colour[2]      = 0xFF;
	params.colour[3]      = (u8)(placeholder->opacity * 255.0);
	params.lineOrigin     = Vec2 (placeholder->position.x, placeholder->position.y);
	params.textCurrent    = Vec2 (placeholder->position.x, placeholder->position.y);

	diva::DrawTextFmt (&params, 0x10028, "Designed by %s", module_designers[module->id].c_str ());
}

HOOK (void, LoadStrArray, sigLoadStrArray ()) {
	originalLoadStrArray ();

	for (auto it = romDirs->begin (); it != romDirs->end (); it++) {
		if (strncmp (it->c_str (), modsPrefix, strlen (modsPrefix))) continue;

		char buf[MAX_PATH];
		sprintf (buf, "%s/rom/lang2/mod_str_array.toml", it->c_str ());
		auto str_array = toml::parse_file_ex (buf);
		if (!str_array.ok ()) continue;

		if (!keys[0].key.has_value ()) {
			auto data = str_array.get ({GetLang ().GetName (), "music"})->as_str ();
			if (data.has_value ()) keys[0].key = data.value ();
			else {
				data = str_array.get ({"music"})->as_str ();
				if (data.has_value ()) keys[0].key = data.value ();
			}
		}
		if (!keys[1].key.has_value ()) {
			auto data = str_array.get ({GetLang ().GetName (), "illustrator"})->as_str ();
			if (data.has_value ()) keys[1].key = data.value ();
			else {
				data = str_array.get ({"illustrator"})->as_str ();
				if (data.has_value ()) keys[1].key = data.value ();
			}
		}
		if (!keys[2].key.has_value ()) {
			auto data = str_array.get ({GetLang ().GetName (), "lyrics"})->as_str ();
			if (data.has_value ()) keys[2].key = data.value ();
			else {
				data = str_array.get ({"lyrics"})->as_str ();
				if (data.has_value ()) keys[2].key = data.value ();
			}
		}
		if (!keys[3].key.has_value ()) {
			auto data = str_array.get ({GetLang ().GetName (), "arranger"})->as_str ();
			if (data.has_value ()) keys[3].key = data.value ();
			else {
				data = str_array.get ({"arranger"})->as_str ();
				if (data.has_value ()) keys[3].key = data.value ();
			}
		}
		if (!keys[4].key.has_value ()) {
			auto data = str_array.get ({GetLang ().GetName (), "manipulator"})->as_str ();
			if (data.has_value ()) keys[4].key = data.value ();
			else {
				data = str_array.get ({"manipulator"})->as_str ();
				if (data.has_value ()) keys[4].key = data.value ();
			}
		}
		if (!keys[5].key.has_value ()) {
			auto data = str_array.get ({GetLang ().GetName (), "pv_editor"})->as_str ();
			if (data.has_value ()) keys[5].key = data.value ();
			else {
				data = str_array.get ({"pv_editor"})->as_str ();
				if (data.has_value ()) keys[5].key = data.value ();
			}
		}
		if (!keys[6].key.has_value ()) {
			auto data = str_array.get ({GetLang ().GetName (), "guitar_player"})->as_str ();
			if (data.has_value ()) keys[6].key = data.value ();
			else {
				data = str_array.get ({"guitar_player"})->as_str ();
				if (data.has_value ()) keys[6].key = data.value ();
			}
		}

		auto lang_designers = str_array.get ({GetLang ().GetName (), "module_designer"});
		if (lang_designers.has_value () && lang_designers->is_table ()) {
			for (int i = 0; i < lang_designers->u.tab.size; i++) {
				char *end    = nullptr;
				const u32 id = strtol (lang_designers->u.tab.key[i], &end, 10);
				if (!end || lang_designers->u.tab.value[i].type != TOML_STRING || module_designers.contains (id)) continue;

				module_designers.insert ({id, lang_designers->u.tab.value[i].u.s});
			}
		}

		auto designers = str_array.get ({"module_designer"});
		if (designers.has_value () && designers->is_table ()) {
			for (int i = 0; i < designers->u.tab.size; i++) {
				char *end    = nullptr;
				const u32 id = strtol (designers->u.tab.key[i], &end, 10);
				if (!end || designers->u.tab.value[i].type != TOML_STRING || module_designers.contains (id)) continue;

				module_designers.insert ({id, designers->u.tab.value[i].u.s});
			}
		}
	}
}

extern "C" {
__declspec (dllexport) void
init () {
	INSTALL_HOOK (DisplayWaitScreen);
	INSTALL_HOOK (DisplayModuleSelect);
	INSTALL_HOOK (LoadStrArray);

	SetCurrentDirectoryW (L"../../");
	auto config = toml::parse_file_ex ("config.toml");
	if (config.ok ()) {
		auto data = config.get ({"mods"})->as_str ().value_or ("mods");
		strcpy (modsPrefix, data.data ());
	} else {
		strcpy (modsPrefix, "mods");
	}
}
}
