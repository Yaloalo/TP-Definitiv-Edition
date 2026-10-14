#include "presentation/texture_packs.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include "mods/svc/host.h"
#include "services.hpp"

// Host function behind the built-in "Use Texture Pack" setting (src/dusk/texture_replacements.cpp).
// Not part of the mod SDK, but exported by the game binary (checked by tools/check_symbols.sh).
namespace dusk::texture_replacements {
void reload();
}

namespace remaster::presentation::texture_packs {
namespace {

namespace fs = std::filesystem;

constexpr const char* kActiveLinkName = "active-pack";

struct Pack {
    std::string dirName;  // empty = original textures
    std::string label;
};

std::vector<Pack> g_packs;
std::vector<const char*> g_labels;

std::string label_for(const std::string& dirName) {
    if (dirName == "dusklight-hd") {
        return "Dusklight HD (faithful, hand-made)";
    }
    if (dirName == "henriko-4k") {
        return "Henriko 4K";
    }
    return dirName;
}

fs::path packs_dir() {
    if (const char* override = std::getenv("TEXTURE_PACKS"); override != nullptr && *override != '\0') {
        return override;
    }
    const char* home = std::getenv("HOME");
    return fs::path(home != nullptr ? home : ".") / "Games/Dusklight/texture-packs";
}

// The mod's persistent data dir is <user dir>/mod_data/<mod id>; texture_replacements is a sibling of
// mod_data. Going through HostService keeps --user-dir overrides working.
fs::path replacements_dir() {
    const char* dataDir = nullptr;
    if (svc_host->data_dir(mod_ctx, &dataDir) != MOD_OK || dataDir == nullptr) {
        return {};
    }
    return fs::path(dataDir).parent_path().parent_path() / "texture_replacements";
}

void scan() {
    g_packs.clear();
    g_packs.push_back({"", "Original (no texture pack)"});
    std::error_code ec;
    std::vector<std::string> names;
    for (const fs::directory_entry& entry : fs::directory_iterator(packs_dir(), ec)) {
        if (entry.is_directory(ec)) {
            names.push_back(entry.path().filename().string());
        }
    }
    std::ranges::sort(names);
    for (const std::string& name : names) {
        g_packs.push_back({name, label_for(name)});
    }
    g_labels.clear();
    for (const Pack& pack : g_packs) {
        g_labels.push_back(pack.label.c_str());
    }
}

int64_t active_index() {
    std::error_code ec;
    const fs::path link = replacements_dir() / kActiveLinkName;
    if (!fs::is_symlink(fs::symlink_status(link, ec))) {
        return 0;
    }
    const std::string target = fs::read_symlink(link, ec).filename().string();
    for (size_t i = 1; i < g_packs.size(); ++i) {
        if (g_packs[i].dirName == target) {
            return static_cast<int64_t>(i);
        }
    }
    return 0;
}

void activate(int64_t index) {
    if (index < 0 || static_cast<size_t>(index) >= g_packs.size()) {
        return;
    }
    const fs::path dir = replacements_dir();
    if (dir.empty()) {
        mods::log::error("texture packs: user data folder unavailable");
        return;
    }
    std::error_code ec;
    fs::create_directories(dir, ec);
    const fs::path link = dir / kActiveLinkName;
    fs::remove(link, ec);  // only ever a symlink we created; never follows it
    const Pack& pack = g_packs[static_cast<size_t>(index)];
    if (!pack.dirName.empty()) {
        fs::create_directory_symlink(packs_dir() / pack.dirName, link, ec);
        if (ec) {
            mods::log::error("texture packs: cannot link {}: {}", pack.dirName, ec.message());
            return;
        }
    }
    dusk::texture_replacements::reload();
    mods::log::info("texture pack: {}", pack.label);

    UiToastDesc toast = UI_TOAST_DESC_INIT;
    toast.title_rml = "Texture pack";
    toast.body_rml = pack.label.c_str();
    toast.duration_ms = 2500;
    svc_ui->push_toast(mod_ctx, &toast);
}

void get_value(ModContext*, void*, UiControlValue* out) {
    out->int_value = active_index();
}

void set_value(ModContext*, void*, const UiControlValue* value) {
    if (value->int_value != active_index()) {
        activate(value->int_value);
    }
}

}  // namespace

ModResult build_controls(UiElementHandle pane) {
    scan();
    UiControlDesc control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_DROPDOWN;
    control.label = "Texture pack";
    control.tooltip = "Applies live. Link's HD look comes from the Enhanced Hero's Tunic mod and stays "
                      "unless that mod is disabled. Requires Settings > Use Texture Pack.";
    control.binding = UI_BINDING_CALLBACKS;
    control.get = get_value;
    control.set = set_value;
    control.options = g_labels.data();
    control.option_count = g_labels.size();
    return svc_ui->pane_add_control(mod_ctx, pane, &control, nullptr);
}

}  // namespace remaster::presentation::texture_packs
