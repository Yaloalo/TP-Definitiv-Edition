"""Production blob loading/persistence: absence vs errors, oversize and changed reads."""
from fixture import ROOT, function, run
source=(ROOT/'src/progression/rewards.cpp').read_text()
code=r'''
#include <cassert>
#include <cstring>
#include "progression/model.hpp"
struct ModContext{};ModContext*mod_ctx=nullptr;constexpr int MOD_OK=0,MOD_UNAVAILABLE=2,MOD_ERROR=1;constexpr unsigned SAVE_BLOB_BUDGET_BYTES=65536;
bool active=true;bool is_active(){return active;}unsigned slot=0;unsigned dComIfGs_getDataNum(){return slot;}
int probe=MOD_OK,read=MOD_OK,writes=0,errors=0;size_t reported=0;bool changeSize=false;std::vector<uint8_t>saved;
struct Save{int get_blob(ModContext*,const char*,void*buf,size_t*n){if(!buf){*n=reported;return probe;}if(read!=MOD_OK)return read;assert(*n>=saved.size());std::memcpy(buf,saved.data(),saved.size());*n=changeSize?reported+1:saved.size();return MOD_OK;}int set_blob(ModContext*,const char*,const void*,size_t){++writes;return MOD_OK;}}save;auto*svc_save=&save;
namespace mods::log{void error(const char*){++errors;}void warn(const char*){}}
namespace remaster::tuning{struct Values{int flaskStartingCharges=3,flaskMaximumCharges=8;}cfg;auto&get(){return cfg;}}
namespace remaster::progression{Model g_model;bool g_ready=false;
'''+function(source,'persist')+function(source,'load')+r'''
}
int main(){using namespace remaster::progression;
 Model original;original.migrate_growth(15);original.add_growth(15);original.allocate(true);original.rareRupees=135;original.charges=1;saved=encode(original);reported=saved.size();load();assert(g_ready&&encode(g_model)==saved);persist();assert(writes==1);
 for(int scenario=0;scenario<6;++scenario){probe=MOD_OK;read=MOD_OK;reported=saved.size();changeSize=false;
  if(scenario==0)probe=MOD_ERROR;if(scenario==1)reported=SAVE_BLOB_BUDGET_BYTES+1;if(scenario==2)read=MOD_ERROR;if(scenario==3)read=MOD_UNAVAILABLE;if(scenario==4)changeSize=true;if(scenario==5){saved[0]^=1;}
  auto untouched=saved;int before=writes;load();assert(!g_ready);persist();assert(writes==before&&saved==untouched);
 }
 saved=encode(original);reported=saved.size();probe=MOD_UNAVAILABLE;read=MOD_OK;changeSize=false;load();assert(g_ready&&g_model.claimed.empty()); // no mod blob in a real slot
 slot=255;load();assert(!g_ready);slot=0;active=false;load();assert(!g_ready);assert(errors==7);
}
'''
run(code)
print('Save callbacks: existing progress, missing blob, failed probes/reads, oversize, changed size, corrupt data and no-slot fail-closed persistence passed')
