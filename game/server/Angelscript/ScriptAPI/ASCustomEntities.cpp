#include <angelscript.h>

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "Angelscript/Entities/CASCustomEntities.h"
#include "Angelscript/Entities/IASCustomEntity.h"

#include "ASCustomEntities.h"

#define CCUSTOMENTITIES_CLASSNAME "CCustomEntities"

static void RegisterScriptCASCustomEntities( asIScriptEngine& engine )
{
	const char* const pszObjectName = CCUSTOMENTITIES_CLASSNAME;

	engine.RegisterObjectType(
		pszObjectName, 0, asOBJ_REF | asOBJ_NOCOUNT );

	engine.RegisterObjectMethod(
		pszObjectName, "bool RegisterCustomEntity(const string& in szMapName, const string& in szClassName)",
		asMETHOD( CASCustomEntities, RegisterCustomEntity ), asCALL_THISCALL );

	engine.RegisterObjectMethod(
		pszObjectName, "bool UnregisterCustomEntity(const string& in szMapName, const string& in szClassName)",
		asMETHOD( CASCustomEntities, UnregisterCustomEntity ), asCALL_THISCALL );

	engine.RegisterObjectMethod(
		pszObjectName, "CBaseEntity@ CreateCustomEntity(const string& in szMapName)",
		asMETHODPR( CASCustomEntities, CreateCustomEntity, ( const std::string& ), CBaseEntity* ), asCALL_THISCALL );

	engine.RegisterGlobalProperty( CCUSTOMENTITIES_CLASSNAME " g_CustomEntities", &g_CustomEntities );
}

static void RegisterScriptICustomEntity( asIScriptEngine& engine )
{
	const char* const pszObjectName = "ICustomEntity";

	engine.RegisterInterface( pszObjectName );
}

namespace CustomEnts
{
static void* Cast( CBaseEntity* pEntity )
{
	if( !pEntity )
		return nullptr;

	auto pCustom = dynamic_cast<IASCustomEntity*>( pEntity );

	if( !pCustom )
		return nullptr;

	auto& instance = pCustom->GetObject();

	void* pInstance = nullptr;

	//AddRef it to prevent problems.
	as::SetObjPointer( pInstance, instance.Get(), *instance.GetTypeInfo().Get() );

	return pInstance;
}
}

static void RegisterScriptCustomEntitiesUtils( asIScriptEngine& engine )
{
	const std::string szOldNS = engine.GetDefaultNamespace();

	engine.SetDefaultNamespace( "CustomEnts" );

	engine.RegisterGlobalFunction(
		"ICustomEntity@ Cast(CBaseEntity@ pEntity)",
		asFUNCTION( CustomEnts::Cast ), asCALL_CDECL );

	engine.SetDefaultNamespace( szOldNS.c_str() );
}

void RegisterScriptCustomEntities( asIScriptEngine& engine )
{
	RegisterScriptCASCustomEntities( engine );
	RegisterScriptICustomEntity( engine );
	RegisterScriptCustomEntitiesUtils( engine );
}
