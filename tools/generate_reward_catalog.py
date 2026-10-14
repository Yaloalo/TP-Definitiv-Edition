#!/usr/bin/env python3
"""Generate the explicit runtime reward table and reviewable placement list."""
import argparse,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def outputs():
 data=json.loads((ROOT/'data/reward_catalog.json').read_text());rows=data['finds'];seen=set()
 for r in rows:
  assert r['check'] not in seen and r['original'] in set(range(1,8))|set(range(10,19))|set(range(22,30))|{237},r
  assert r['kind'] in ['Craft','Ancient','Growth','Flask','Rupees','Sunward','Mirror']
  seen.add(r['check'])
 quote=lambda s:json.dumps(s,ensure_ascii=True)
 header='#pragma once\n// Generated from data/reward_catalog.json by tools/generate_reward_catalog.py.\n#include <array>\n#include <cstdint>\n#include <string_view>\nnamespace remaster::progression::treasures {\nenum class Kind { Craft, Ancient, Growth, Flask, Rupees, Sunward, Mirror };\nstruct Find { const char* check; const char* location; uint8_t original; Kind kind; };\n'
 header+=f'inline constexpr std::array<Find,{len(rows)}> kFinds={{{{\n'
 header+=''.join(f'    {{{quote(r["check"])}, {quote(r["location"])}, {r["original"]}, Kind::{r["kind"]}}},\n' for r in rows)
 header+='}};\ninline const Find* find(std::string_view name) {\n    for (const auto& entry:kFinds) if(name==entry.check)return &entry;\n    return nullptr;\n}\n}\n'
 doc='# Deliberate exploration reward placements\n\nAssignments belong to this mod. Native check coordinates/original item IDs were\nverified against the local Dusklight Randomizer catalogue. No runtime randomizer\nis needed. Each named grant is finite and recorded per save slot.\n\nNew-save growth budget: 85 native heart fragments + 11 collection fragments +\n14 curated treasure fragments = 110, or 22 shared blessings. Individual bugs\nprovide Gilded Thread for permanent upgrades. Existing growth is never deducted.\n\nCraft = a Hyrule Craft Relic; Ancient = an Ancient Rune; Sunward and Mirror are\nunique artifacts for Magic Armor upgrades. Full recipes and flask pacing are\nin PROGRESSION_AND_WARD.md.\n\n| Location | Native check | Reward |\n|---|---|---|\n'
 doc+=''.join(f'| {r["location"]} | `{r["check"]}` | {r["kind"]} |\n' for r in rows)
 return {ROOT/'src/progression/treasure_table.hpp':header,ROOT/'docs/REWARD_PLACEMENTS.md':doc}
def main():
 parser=argparse.ArgumentParser();parser.add_argument('--check',action='store_true');args=parser.parse_args()
 for path,text in outputs().items():
  if args.check:
   if not path.exists() or path.read_text()!=text:raise SystemExit(f'Stale reward catalogue: {path}')
  else:path.write_text(text)
if __name__=='__main__':main()
