
#include "SnapManager.h"
//#include "utilities/IConsole.h"
//NoGore is unsupported in xNVSE


IDebugLog		gLog("Snap.log");
PluginHandle	g_pluginHandle = kPluginHandle_Invalid;
NVSEMessagingInterface* g_messagingInterface{};
NVSEInterface* g_nvseInterface{};
NVSEEventManagerInterface* g_eventInterface{};
_InventoryRefGetForID InventoryRefGetForID;
_InventoryRefCreate InventoryRefCreate;



static UINT32 proj_destroy_vtfun;
static UINT32 proj_impact_vtfun;
static UINT32 proj_update_vtfun_snap;

bool NVSEPlugin_Query(const NVSEInterface* nvse, PluginInfo* info)
{
	_MESSAGE("query");

	// fill out the info structure
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "Snap";
	info->version = 114;

	// version checks
	if (nvse->nvseVersion < PACKED_NVSE_VERSION)
	{
		_ERROR("NVSE version too old (got %08X expected at least %08X)", nvse->nvseVersion, PACKED_NVSE_VERSION);
		return false;
	}

	if (!nvse->isEditor)
	{
		if (nvse->runtimeVersion < RUNTIME_VERSION_1_4_0_525)
		{
			_ERROR("incorrect runtime version (got %08X need at least %08X)", nvse->runtimeVersion, RUNTIME_VERSION_1_4_0_525);
			return false;
		}

		if (nvse->isNogore)
		{
			_ERROR("NoGore is not supported");
			return false;
		}
	}
	else
	{
		if (nvse->editorVersion < CS_VERSION_1_4_0_518)
		{
			_ERROR("incorrect editor version (got %08X need at least %08X)", nvse->editorVersion, CS_VERSION_1_4_0_518);
			return false;
		}
	}

	// version checks pass
	// any version compatibility checks should be done here
	return true;
}




static Projectile* __fastcall MissileProjectileDestroyVF(MissileProjectile* _this, void* edx, char flag) {
	SnapMng.Erase(_this);
	return ThisStdCall<Projectile*>(proj_destroy_vtfun, _this, flag);
}

static void __fastcall MissileProjectileUpdateVF_ForSnap(MissileProjectile* _this, void* unuse, float timepassed) {
	if (!_this || !_this->sourceRef || _this->sourceRef == PC_Ref || !IS_ACTOR(_this->sourceRef) ||
		!((Actor*)_this->sourceRef)->baseProcess || ((Actor*)_this->sourceRef)->baseProcess->processLevel || !_this->sourceWeap || _this->hasImpacted ) {
		
		ThisStdCall<void>(proj_update_vtfun_snap, _this, timepassed);
		return; 
	}
	const BGSProjectile* basepj = (BGSProjectile*)_this->baseForm;
	if ( !basepj || PJIsExplosion(basepj) ){
		ThisStdCall<void>(proj_update_vtfun_snap, _this, timepassed);
		return;
	}

	ThisStdCall<void>(proj_update_vtfun_snap, _this, timepassed);
	SnapMng.TryToSnapAlt(_this);
	SnapMng.RecordThisPJ(_this);
}



static bool __fastcall PJ_Impact_Hook_ForSnap(MissileProjectile* _this, void* unuse) {

using namespace BulletSnap;
	
	bool ret = ThisStdCall<bool>(proj_impact_vtfun, (MissileProjectile*)_this);
	if ( ret && _this && _this->hasImpacted )SnapMng.SnapImpact(_this);
	return ret; 
}

static inline void InstallHook()
{
	//INIT_Bullet_Flag();
	proj_destroy_vtfun = DetourVtable(0x108FA54, UInt32(MissileProjectileDestroyVF));
	proj_impact_vtfun = DetourVtable(0x108FD58, UInt32(PJ_Impact_Hook_ForSnap));
	proj_update_vtfun_snap = DetourVtable(0x108FD54, UInt32(MissileProjectileUpdateVF_ForSnap));
}


// This is a message handler for nvse events
// With this, plugins can listen to messages such as whenever the game loads
void MessageHandler(NVSEMessagingInterface::Message* msg)
{
	switch (msg->type)
	{
	case NVSEMessagingInterface::kMessage_DeferredInit:
		InstallHook();
		BulletSnap::InitSnap();
		break;
	}
}


bool NVSEPlugin_Load(NVSEInterface* nvse)
{
	_MESSAGE("BulletSnap load");
	g_pluginHandle = nvse->GetPluginHandle();
	// save the NVSE interface in case we need it later
	g_nvseInterface = nvse;
	//NVSEDataInterface* nvseData = (NVSEDataInterface*)nvse->QueryInterface(kInterface_Data);
	

	// register to receive messages from NVSE

	if (!nvse->isEditor)
	{
		g_messagingInterface = static_cast<NVSEMessagingInterface*>(nvse->QueryInterface(kInterface_Messaging));
		g_messagingInterface->RegisterListener(g_pluginHandle, "NVSE", MessageHandler);
	}
	return true;
}