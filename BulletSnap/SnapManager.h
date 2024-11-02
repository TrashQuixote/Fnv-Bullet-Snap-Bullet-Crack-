#pragma once
#include "Gathering_Utility.h"
#include <random> 
#include <array>
#include "RoughINIReader.h"

namespace BulletSnap {
	static TESSound* snap_close = nullptr;
	static TESSound* snap_close_hc = nullptr;
	static TESSound* snap_close_vhc = nullptr;
	static TESSound* snap_dist = nullptr;
	static TESSound* snap_dist_hc = nullptr;
	static TESSound* snap_dist_vhc = nullptr;
	static TESSound* snap_far = nullptr;
	static TESSound* snap_far_vhc = nullptr;
	static TESSound* subsonic_close = nullptr;
	static TESSound* subsonic_mid = nullptr;


	static TESSound* InitTESSound(const char* sound_path, float min_att_dist, float max_att_dist,bool enable_2Dradius = false, UINT32 form_id = 0x21f) {
		TESForm* to_clone = LookupFormByID(form_id);
		if (!to_clone) {
			gLog.Message("LookupFormByID for snap failed");
			return nullptr;
		}
		else if (!IS_TYPE(to_clone, TESSound))return nullptr;

		TESSound* ret = (TESSound*)TempCloneForm(to_clone);

		if (!ret) {
			gLog.Message("TempCloneForm for snap failed");
			return nullptr;
		}
		else if (!IS_TYPE(ret, TESSound))return nullptr;

		ret->soundFile.path.Set(sound_path);
		if (enable_2Dradius) ret->soundFlags |= TESSound::kFlag_2DRadius;
		//ret->soundFlags |= TESSound::kFlag_360LFE;
		//
		ret->minAttenuationDist = min_att_dist;
		ret->maxAttenuationDist = max_att_dist;
		return ret;
	}

	static __forceinline void Init_Crack_Sound();
	
	

	/*
	constexpr float snap_veryclose_hAngZX = 200; // 10 * 10
	constexpr float snap_veryclose_dis_square = 262144; // 512 * 512
	constexpr float snap_close_hAngZX = 800;	// 20 * 20
	constexpr float snap_close_close_square = 2359296;	// 1536 * 1536
	constexpr float snap_dist_hAngZX = 1800;	// 30 * 30
	constexpr float snap_close_dis_square = 16744464;	// 4096 * 4096
	constexpr float snap_far_hAngZX = 4050;	// 45 * 45
	constexpr float snap_close_far_square = 144000000;	// 12000 * 12000
	*/
	constexpr float cos_15 = 0.966;
	constexpr float cos_225 = 0.924;
	constexpr float cos_45 = 0.707;
	constexpr float cos_675 = 0.381;

	enum DisBasedFlag : UINT8 {
		VeryClose = 0b00000000,
		Close = 0b00000001,
		Dis = 0b00000010,
		Far = 0b00000011,

		None = 0b00001111
	};

	enum DmgBasedFlag : UINT8 {
		general = 0b00010000,
		highcaliber = 0b00100000,
		veryhighcaliber = 0b00110000
	};

	enum SoundPick : UINT8 {
		general_veryclose = 0b00010000,
		general_close = 0b00010001,
		general_dis = 0b00010010,
		general_far = 0b00010011,

		highcaliber_veryclose = 0b00100000,
		highcaliber_close = 0b00100001,
		highcaliber_mid = 0b00100010,
		highcaliber_far = 0b00100011,

		veryhighcaliber_veryclose = 0b00110000,
		veryhighcaliber_close = 0b00110001,
		veryhighcaliber_mid = 0b00110010,
		veryhighcaliber_far = 0b00110011,
	};

	static __forceinline NiNode* GetProjectileNode(TESObjectREFR* _ref) {
		if (IS_ACTOR(_ref)) {
			Actor* _actor = (Actor*)_ref;
			if (BaseProcess* b_proc = _actor->baseProcess; !b_proc->processLevel)
				if (NiNode* node = b_proc->GetProjectileNode())
					return node;
		}
		return nullptr;
	}

	[[nodiscard]] static __forceinline bool Play_Snap_Sound(Projectile* _pj, TESSound* _snap, bool play_at_pc = false) {
		if (!_snap) {
			Init_Crack_Sound();
			return false;
		}
		if (play_at_pc || IsPJHitscan(_pj) ) _Sound::PlaySound3D(_snap, _pj,true,PC_Ref);
		if (play_at_pc) _Sound::PlaySound3D(_snap, _pj,true,PC_Ref);
		else _Sound::PlaySound3D(_snap, _pj);
		return true;
	}

	static float __forceinline DotProduct_PJToMainCamera(const Projectile* _proj) {
		const auto* scene_graph = GetSceneGraph();
		if (scene_graph){
			if (const NiCamera* main_camera = scene_graph->camera){
				const NiVector3& camera_pos = main_camera->m_worldTranslate;
				NiVector3 pj_to_camera{ camera_pos.x - _proj->posX, camera_pos.y - _proj->posY, camera_pos.z - _proj->posZ };
				pj_to_camera.Normalize();
				return pj_to_camera.DotProduct(_proj->UnitVector);
				//return (pj_to_camera.x * _proj->UnitVector.x + pj_to_camera.y * _proj->UnitVector.y + pj_to_camera.z * _proj->UnitVector.z);
			}
		}
		NiVector3 pj_to_pc{ PC_Ref->posX - _proj->posX, PC_Ref->posY - _proj->posY, PC_Ref->posZ - _proj->posZ };
		pj_to_pc.Normalize();
		return pj_to_pc.DotProduct(_proj->UnitVector);
	}
	using formid = UINT32;

	enum BulletTrackerFlag {
		GetCloseToPC = 0b00000001,
		WontSnap = 0b00000010,
		HaveSnap = 0b00000100
	};
	enum SettingFlag{
		impact_pc_snap = 0b00000001,
		distance_based_snap = 0b00000010,
		subsonic = 0b00000100

	};

	struct Snap_Info {
		UINT8 flag = 0;
		UINT8 sound_pick = DisBasedFlag::None;
		Snap_Info(UINT8 _flag, UINT8 _sound_pick) : flag(_flag), sound_pick(_sound_pick) {};
		__forceinline bool IsFlagOn(UINT8 _bit) const { return (flag & _bit) != 0; }
		__forceinline void SetFlagOn(UINT8 _bit) { flag |= _bit; }
		__forceinline bool UseSubsonic(const Projectile* _pj) const;
		TESSound* GetSnapSound(const Projectile* _pj) const;
		UINT8 GetDmgRange(const Projectile* _pj) const;
		TESSound* GetSnapSound_ImpactPC(const Projectile* _pj) const;
	};

	class SnapMng {
		SnapMng() {
			Init_Crack_Sound();
		};
		~SnapMng() {};
	public:
		UINT8 setting_flag = 0;
		std::unordered_map<formid, Snap_Info> SnapMap{};
		std::array<float, 4> distance_based{ 0 };
		std::array<UINT16, 4> volume_decrease{ 0 };
		float subsonic_speed = 26000;
		float dmg_mid = 30;
		float dmg_high = 45;
		float snap_cos_theta = 0;

		__forceinline void SetDistanceBased(UINT8 _index, float distance) { if (_index <= Far) distance_based[_index] = distance;}
		__forceinline void SetVolumeDecrease(UINT8 _index, UINT16 distance) { if (_index <= Far) volume_decrease[_index] = distance;}
		__forceinline bool IsSettingFlagOn(UINT8 _bit) const { return (setting_flag & _bit) != 0; }
		__forceinline void SetSettingFlagOn(UINT8 _bit) { setting_flag |= _bit; }
		__forceinline float GetDamageMid() const { return dmg_mid; }
		__forceinline float GetDamageHigh() const { return dmg_high; }
		__forceinline float GetDamageSnapAngleCos() const { return snap_cos_theta; }
		static __forceinline SnapMng& snap_manager_instance() {
			static SnapMng snap_manager_instance;
			return snap_manager_instance;
		}
		

		__forceinline void SnapImpact(Projectile* _pj) {
			if (const auto& iter = SnapMap.find(_pj->refID); iter != SnapMap.end()) {
				auto& snap_info = iter->second;
				if (snap_info.IsFlagOn( (WontSnap | HaveSnap) )) return ;
				if (IsSettingFlagOn(impact_pc_snap) && _pj->GetImpactRef() == PC_Ref) {
					if (Play_Snap_Sound(_pj, snap_info.GetSnapSound_ImpactPC(_pj), true)) { // do playing sound 
						snap_info.SetFlagOn(HaveSnap);
					}
					return;
				}

				if (snap_info.IsFlagOn(GetCloseToPC)) {
					float dot_res = DotProduct_PJToMainCamera(_pj);
					if (dot_res <= snap_manager_instance().GetDamageSnapAngleCos()) {
						if ( Play_Snap_Sound(_pj, snap_info.GetSnapSound(_pj) ) ) { // do playing sound 
							snap_info.SetFlagOn(HaveSnap); 
						}
					}
				}
			}
		}


		__forceinline bool IsPJHaveSnap(Projectile* _pj) {
			if (const auto& iter = SnapMap.find(_pj->refID); iter != SnapMap.end()) {
				return iter->second.IsFlagOn(HaveSnap);
			}
			return false;
		}

		__forceinline void SetPJHaveSnap(Projectile* _pj) {
			if (const auto& iter = SnapMap.find(_pj->refID); iter != SnapMap.end()) {
				iter->second.SetFlagOn(HaveSnap);
			}
		}

		__forceinline void Erase(Projectile* _pj) {
			if (const auto& iter = SnapMap.find(_pj->refID); iter != SnapMap.end()) {
				SnapMap.erase(iter);
			}
		}

		/*
			Record this projectile
			flag the projectile as 'NotSnap' or 'WillDoSnap'
		*/
		__forceinline void RecordThisPJ(Projectile* _pj) {
			if (const auto& iter = SnapMap.find(_pj->refID); iter != SnapMap.end()) return;	// has in map
			float res_dot = DotProduct_PJToMainCamera(_pj);
			if (res_dot <= snap_manager_instance().GetDamageSnapAngleCos()) {
				SnapMap.try_emplace(_pj->refID, Snap_Info{ WontSnap ,0 }); 
				return;
			}
			//gLog.FormattedMessage("Take PJ To Map dp: %.2f", res_dot);

			if (res_dot > cos_15) SnapMap.try_emplace(_pj->refID, Snap_Info{ GetCloseToPC ,VeryClose });
			else if (res_dot > cos_45) SnapMap.try_emplace(_pj->refID, Snap_Info{ GetCloseToPC ,Close });
			else if (res_dot > cos_675) SnapMap.try_emplace(_pj->refID, Snap_Info{ GetCloseToPC ,Dis});
			else if (res_dot > 0) SnapMap.try_emplace(_pj->refID, Snap_Info{ GetCloseToPC ,Far });
		}

		/*
			Will Try To Snap When This Proj Can Do
		*/
		__forceinline void TryToSnapAlt(Projectile* _pj) {
			if (const auto& iter = SnapMap.find(_pj->refID); iter != SnapMap.end()) {
				auto& snap_info = iter->second;
				if (snap_info.IsFlagOn(HaveSnap)) return;
				float dot_res = DotProduct_PJToMainCamera(_pj);
				if (!snap_info.IsFlagOn(GetCloseToPC) && dot_res > snap_manager_instance().GetDamageSnapAngleCos()){
					snap_info.SetFlagOn(GetCloseToPC);
					if (dot_res > cos_15) snap_info.sound_pick = VeryClose;
					else if (dot_res > cos_45) snap_info.sound_pick = Close;
					else if (dot_res > cos_675)snap_info.sound_pick = Dis;
					else if (dot_res > 0) snap_info.sound_pick = Far;
				}

				if (snap_info.IsFlagOn(GetCloseToPC)) {
					if (dot_res <= snap_manager_instance().GetDamageSnapAngleCos()) {
						if (Play_Snap_Sound(_pj, snap_info.GetSnapSound(_pj))) { // do playing sound
							//gLog.FormattedMessage("PJ %x Pass Sound Play dot_res %.2f", _pj->refID, dot_res);
							snap_info.SetFlagOn(HaveSnap);
							return;
						}
					}
					//else gLog.FormattedMessage("PJ %x Get Close To PC dot_res %.2f", _pj->refID, dot_res);
				}
			}
		}
	};


	inline bool Snap_Info::UseSubsonic(const Projectile* _pj) const {
		if (!SnapMng::snap_manager_instance().IsSettingFlagOn(subsonic)) return false;
		if (CalcProjSpeed((BGSProjectile*)_pj->baseForm, _pj) < SnapMng::snap_manager_instance().subsonic_speed) return true;
		return false;
	}

	inline UINT8 Snap_Info::GetDmgRange(const Projectile* _pj) const{
		if (_pj->hitDamage < SnapMng::snap_manager_instance().GetDamageMid()) return DmgBasedFlag::general;
		else if (_pj->hitDamage < SnapMng::snap_manager_instance().GetDamageHigh()) return DmgBasedFlag::highcaliber;
		else return DmgBasedFlag::veryhighcaliber;
	}

	
	TESSound* Snap_Info::GetSnapSound(const Projectile* _pj) const {
		UINT8 sound_choose = sound_pick;
		if (SnapMng::snap_manager_instance().IsSettingFlagOn(distance_based_snap)){
			float Dis_Sq_ToPC = GetRefDistanceSquare(PC_Ref,_pj);
			UINT8 _sound_index = 0;
			while (_sound_index <= Far){
				float dist = SnapMng::snap_manager_instance().distance_based[_sound_index];
				if (Dis_Sq_ToPC <= Ut_Square(dist)){
					sound_choose = _sound_index; 
					break;
				}
				++_sound_index;
			}
			if (_sound_index > Far) return nullptr; 

		}

		if (UseSubsonic(_pj))
		{
			if (sound_choose <= Close) return subsonic_close;
			else if (sound_choose == Dis) return subsonic_mid;
		}

		switch (sound_choose | (GetDmgRange(_pj))  )
		{
		case general_veryclose:
		case general_close:
			return snap_close;
		case general_dis:
			return snap_dist;
		case general_far:
		case highcaliber_far:
			return snap_far;
		case highcaliber_veryclose:
		case highcaliber_close:
			return snap_close_hc;
		case highcaliber_mid:
			return snap_dist_hc;
		case veryhighcaliber_veryclose:
		case veryhighcaliber_close:
			return snap_close_vhc;
		case veryhighcaliber_mid:
			return snap_dist_vhc;
		case veryhighcaliber_far:
			return snap_far_vhc;
		}
		return nullptr;
	}

	
	inline TESSound* Snap_Info::GetSnapSound_ImpactPC(const Projectile* _pj) const {
		if (UseSubsonic(_pj)) return subsonic_close;
		switch ( GetDmgRange(_pj) )
		{
		case DmgBasedFlag::general: 
			return snap_close;
		case DmgBasedFlag::highcaliber: 
			return snap_close_hc;
		case DmgBasedFlag::veryhighcaliber: 
			return snap_close_vhc;
		default:
			return snap_close;
		}
	}

	static inline bool ReadGenericConfig() {
		namespace fs = std::filesystem;
		gLog.Message("ReadGenericConfig");
		fs::path config_root_path = fs::current_path();
		config_root_path += R"(\Data\NVSE\Plugins\BulletSnapConfig\)";
		if (!fs::exists(config_root_path)) {
			gLog.Message("ReadGenericConfig path not exist");
			return false;
		}

		roughinireader::INIReader _ini{ config_root_path };

		auto ret = _ini.SetCurrentINIFileName("BulletSnap.ini");
		if (!ret.has_value()) {
			gLog.FormattedMessage("Failed to set generic config filename : %s", ret.error().message());
			return false;
		}
		ret = _ini.ConstructSectionMap();
		if (!ret.has_value()) {
			gLog.FormattedMessage("Failed to construct section map : %s", ret.error().message());
			return false;
		}

		// init the generic config
		
		std::string raw_type_val = "";
		UINT32 temp_flag = 0;
		raw_type_val = _ini.GetRawTypeVal("General", "ImpactedPlayerSnap");
		temp_flag = _ini.GetUInt(raw_type_val,0);
		if (temp_flag > 0) SnapMng::snap_manager_instance().SetSettingFlagOn(impact_pc_snap); 
		
		raw_type_val = _ini.GetRawTypeVal("General", "DistancedBasedSnap");
		temp_flag = _ini.GetUInt(raw_type_val, 0);
		if (temp_flag > 0) SnapMng::snap_manager_instance().SetSettingFlagOn(distance_based_snap);

		raw_type_val = _ini.GetRawTypeVal("General", "Subsonic");
		temp_flag = _ini.GetUInt(raw_type_val, 0);
		if (temp_flag > 0) SnapMng::snap_manager_instance().SetSettingFlagOn(subsonic);

		raw_type_val = _ini.GetRawTypeVal("General", "SubsonicThresholdSpeed");
		SnapMng::snap_manager_instance().subsonic_speed = _ini.GetFloat(raw_type_val, 26000);

		raw_type_val = _ini.GetRawTypeVal("General", "SnapAngle");
		SnapMng::snap_manager_instance().snap_cos_theta = std::cos(_ini.GetFloat(raw_type_val, 90));

		raw_type_val = _ini.GetRawTypeVal("DistanceBased", "VeryClose");
		SnapMng::snap_manager_instance().SetDistanceBased(VeryClose,_ini.GetFloat(raw_type_val, 0));

		raw_type_val = _ini.GetRawTypeVal("DistanceBased", "Close");
		SnapMng::snap_manager_instance().SetDistanceBased(Close, _ini.GetFloat(raw_type_val, 0));

		raw_type_val = _ini.GetRawTypeVal("DistanceBased", "Distance");
		SnapMng::snap_manager_instance().SetDistanceBased(Dis, _ini.GetFloat(raw_type_val, 0));

		raw_type_val = _ini.GetRawTypeVal("DistanceBased", "Far");
		SnapMng::snap_manager_instance().SetDistanceBased(Far, _ini.GetFloat(raw_type_val, 0));

		raw_type_val = _ini.GetRawTypeVal("DamageBased", "Middledmg");
		SnapMng::snap_manager_instance().dmg_mid = _ini.GetFloat(raw_type_val, 0);

		raw_type_val = _ini.GetRawTypeVal("DamageBased", "Highdmg");
		SnapMng::snap_manager_instance().dmg_high = _ini.GetFloat(raw_type_val, 0);


		gLog.FormattedMessage("VeryClose %f", SnapMng::snap_manager_instance().distance_based[VeryClose]);
		gLog.FormattedMessage("Close %f", SnapMng::snap_manager_instance().distance_based[Close]);
		gLog.FormattedMessage("Dis %f", SnapMng::snap_manager_instance().distance_based[Dis]);
		gLog.FormattedMessage("Far %f", SnapMng::snap_manager_instance().distance_based[Far]);


		//for (auto iter = BulletManager::bullet_manager_instance().MaterialPenalty.begin(); iter != BulletManager::bullet_manager_instance().MaterialPenalty.end(); iter++)
		//{
		//	gLog.FormattedMessage("MP %f",*iter);
		//} 
		return true;
	}


	static __forceinline void Init_Crack_Sound()
	{
		snap_close = InitTESSound(R"(fx\GeneralCloseMid\Close\)", 255.0f, 120.96f);
		snap_close_hc = InitTESSound(R"(fx\HCCloseMid\Close\)", 255.0f, 120.96f);
		snap_close_vhc = InitTESSound(R"(fx\VeryHighCaliber\Close\)", 255.0f, 120.96f);
		snap_dist = InitTESSound(R"(fx\GeneralCloseMid\Mid\)", 255.0f, 120.96f);
		snap_dist_hc = InitTESSound(R"(fx\HCCloseMid\Mid\)", 255.0f, 120.96f);
		snap_dist_vhc = InitTESSound(R"(fx\VeryHighCaliber\Mid\)", 255.0f, 120.96f);
		snap_far = InitTESSound(R"(fx\GeneralFar\)", 255.0f, 120.96f);
		subsonic_close = InitTESSound(R"(fx\SubSonic\Close\)", 255.0f, 120.96f);
		subsonic_mid = InitTESSound(R"(fx\SubSonic\Mid\)", 255.0f, 120.96f);
	}
	static __forceinline void InitSnap() {
		ReadGenericConfig();
		Init_Crack_Sound();
	}

};
#define SnapMng BulletSnap::SnapMng::snap_manager_instance() 




