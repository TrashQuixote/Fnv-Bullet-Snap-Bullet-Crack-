#pragma once
#include "Gathering_Code.h"
#define PC_Ref PlayerCharacter::GetSingleton()
#define Ut_Square(x) ( (x) * (x) ) 

static __forceinline bool PJIsExplosion(const BGSProjectile* basepj) {
	return (basepj->projFlags & 2) != 0 && (basepj->type & 1) != 0;
}

static __forceinline float CalcProjSpeed(const BGSProjectile* basepj, const Projectile* _pjref) {
	return (basepj->speed * _pjref->speedMult);
}

static __forceinline SceneGraph* GetSceneGraph() {
	return (*(SceneGraph**)0x11DEB7C);
}

static __forceinline float GetRefDistanceSquare(const TESObjectREFR* _refA, const TESObjectREFR* _refB) {
	return (Ut_Square(_refB->posX - _refA->posX) + Ut_Square(_refB->posY - _refA->posY) + Ut_Square(_refB->posZ - _refA->posZ));
}

static __forceinline float GetRefDistanceSquare2D(const TESObjectREFR* _refA, const TESObjectREFR* _refB) {
	return (Ut_Square(_refB->posX - _refA->posX) + Ut_Square(_refB->posY - _refA->posY));
}

static __forceinline BGSImpactDataSet* GetImpactDataSet(const TESObjectWEAP* _weap) {
	return _weap->impactDataSet;
}


/*
 ==== material ====
0	Stone | 1	Dirt | 2	Grass | 3	Glass
4	Metal | 5	Wood | 6  Organic | 7	Cloth
8	Water | 9	Hollow Metal | 10	Organic Bug | 11	Organic Glow
*/
static __forceinline BGSImpactData* GetImpactDataByMaterial(const TESObjectWEAP* _weap,UINT8 _material) {
	if (_material < 12) return GetImpactDataSet(_weap)->impactDatas[_material];
	return nullptr;
}