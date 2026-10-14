#include "economy/currency.hpp"
#include "combat/ward.hpp"
#include "core/tuning.hpp"
#include "core/hooks.hpp"
#include "services.hpp"
#include "d/d_com_inf_game.h"
#include "d/d_msg_class.h"
#include "d/d_msg_object.h"
#include "d/d_msg_flow.h"
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_npc_henna.h"
#include "d/d_pane_class.h"

namespace remaster::economy {
namespace {
DEFINE_HOOK(&dComIfGs_getRupee, RupeeRead);
DEFINE_HOOK(&dComIfGs_setRupee, RupeeWrite);
DEFINE_HOOK(&dComIfGp_setItemRupeeCount, RupeeDelta);
DEFINE_HOOK(&dMsgFlow_c::query004, PriceCheck);
DEFINE_HOOK(&jmessage_tReference::inputNumber, DonationInput);
DEFINE_HOOK_SYMBOL("daNpc_Henna_Execute", int(npc_henna_class*), HenaExecute);
DEFINE_HOOK(&daAlink_c::checkMagicArmorHeavy, ArmorHeavy);
DEFINE_HOOK(&daAlink_c::checkMagicArmorNoDamage, ArmorInvulnerability);
unsigned g_feeScope = 0;
struct HiddenRupees { dMeter2Draw_c* meter = nullptr; bool visible[3]{}; } g_hidden;
bool enabled() { return is_active() && tuning::get().removeCurrency; }
HookAction read(ModContext*, void*, void* result, void*) {
    if(enabled()&&g_feeScope){*static_cast<u16*>(result)=65535;return HOOK_SKIP_ORIGINAL;}
    if(combat::ward::visual_rupees(*static_cast<u16*>(result)))return HOOK_SKIP_ORIGINAL;
    if (!enabled()) return HOOK_CONTINUE;
    // Only Hena's native minigame fee checks see a virtual affordability value.
    // The actual balance, HUD, save data and every other caller see zero currency.
    *static_cast<u16*>(result) = g_feeScope ? 65535 : 0; return HOOK_SKIP_ORIGINAL;
}
HookAction write(ModContext*, void* args, void*, void*) {
    if (enabled()) mods::arg_ref<u16>(args, 0) = 0;
    return HOOK_CONTINUE;
}
HookAction delta(ModContext*, void*, void*, void*) { return enabled() ? HOOK_SKIP_ORIGINAL : HOOK_CONTINUE; }
HookAction price(ModContext*, void* args, void* result, void*) {
    if (!enabled()) return HOOK_CONTINUE;
    auto* node = mods::arg<mesg_flow_node_branch*>(args, 1);
    // Zero is a wallet-space query (space available); positive params are prices (never unaffordable).
    *static_cast<u16*>(result) = node && node->param == 0 ? 1 : 0;
    return HOOK_SKIP_ORIGINAL;
}
HookAction donation(ModContext*, void* args, void*, void*) {
    if (!enabled()) return HOOK_CONTINUE;
    auto* reference = mods::arg<jmessage_tReference*>(args, 0);
    if (!reference) return HOOK_CONTINUE;
    // Native donation event callbacks still run and set the bridge/Malo/prayer quest flags.
    // A single confirmation funds the quest; no wallet, collection or repeated numeric input.
    reference->getObjectPtr()->setInputValue(9999);
    return HOOK_SKIP_ORIGINAL;
}
HookAction fee_pre(ModContext*, void*, void*, void*) { if (enabled()) ++g_feeScope; return HOOK_CONTINUE; }
void fee_post(ModContext*, void*, void*, void*) { if (g_feeScope) --g_feeScope; }
HookAction armor(ModContext*, void*, void* result, void*) {
    if (!is_active()) return HOOK_CONTINUE;
    // Armor protection is handled by our rechargeable ward module. Empty wallets cannot immobilize Link.
    *static_cast<BOOL*>(result) = FALSE; return HOOK_SKIP_ORIGINAL;
}
HookAction meter_pre(ModContext*, void* args, void*, void*) {
    if (!enabled()) return HOOK_CONTINUE;
    auto* meter = mods::arg<dMeter2Draw_c*>(args, 0); if (!meter) return HOOK_CONTINUE;
    g_hidden.meter = meter;
    for (int i = 0; i < 3; ++i) if (meter->mpRupeeParent[i] && meter->mpRupeeParent[i]->getPanePtr()) {
        g_hidden.visible[i] = meter->mpRupeeParent[i]->getPanePtr()->isVisible();
        meter->mpRupeeParent[i]->getPanePtr()->hide();
    }
    return HOOK_CONTINUE;
}
void meter_post(ModContext*, void*, void*, void*) {
    if (!g_hidden.meter) return;
    for (int i = 0; i < 3; ++i) if (g_hidden.visible[i] && g_hidden.meter->mpRupeeParent[i])
        g_hidden.meter->mpRupeeParent[i]->getPanePtr()->show();
    g_hidden = {};
}
}
ModResult install() {
    auto r = mods::hook::add_pre<RupeeRead>(read);
    if (r == MOD_OK) r = mods::hook::add_pre<RupeeWrite>(write);
    if (r == MOD_OK) r = mods::hook::add_pre<RupeeDelta>(delta);
    if (r == MOD_OK) r = mods::hook::add_pre<PriceCheck>(price);
    if (r == MOD_OK) r = mods::hook::add_pre<DonationInput>(donation);
    if (r == MOD_OK) r = mods::hook::add_pre<HenaExecute>(fee_pre);
    if (r == MOD_OK) r = mods::hook::add_post<HenaExecute>(fee_post);
    if (r == MOD_OK) r = mods::hook::add_pre<ArmorHeavy>(armor);
    if (r == MOD_OK) r = mods::hook::add_pre<ArmorInvulnerability>(armor);
    if (r == MOD_OK) r = mods::hook::add_pre<hooks::Shared1>(meter_pre);
    if (r == MOD_OK) r = mods::hook::add_post<hooks::Shared1>(meter_post);
    return r;
}
}
