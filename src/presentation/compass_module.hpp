#pragma once
#include "mods/api.h"
#include "mods/svc/ui.h"
namespace remaster::presentation::compass {
ModResult install();
ModResult build_controls(UiElementHandle pane);
ModResult shutdown();
}
