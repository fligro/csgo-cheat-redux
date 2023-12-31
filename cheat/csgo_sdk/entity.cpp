#include "csgo_sdk.h"
#include "../offsets.h"

//Macro to implement simple netvars quickly
#define ent_netvar( nv_type, nv_name, nv_offset ) nv_type entity_t::##nv_name( ) { return m_memory->read< nv_type >( m_ptr + nv_offset );  }

namespace sdk {
	entity_t::entity_t( forceinline::memory_manager* memory, sdk::modules_t* modules, std::uintptr_t ent_ptr ) {
		m_memory = memory;
		m_modules = modules;
		m_ptr = ent_ptr;
	}

	ent_netvar( bool, dormant, offset::m_bDormant );

	ent_netvar( int, health, offset::m_iHealth );
	ent_netvar( int, index, 0x64 ); // Dunno what this is even for
	ent_netvar( int, life_state, offset::m_lifeState );
	ent_netvar( int, team, offset::m_iTeamNum );

	ent_netvar( vec3_t, origin, offset::m_vecOrigin );

	void entity_t::get_name( std::string& out ) {
		struct player_info_t {
			char __pad[ 0x10 ];
			char name[ 32 ];
		};

		std::uintptr_t client_state = m_memory->read< std::uintptr_t >( m_modules->engine_dll + 0x590D8C ); //m_dwClientState
		std::uintptr_t user_info_table = m_memory->read< std::uintptr_t >( client_state + 0x52B8 ); //m_dwClientState_PlayerInfo
		std::uintptr_t x = m_memory->read< std::uintptr_t >( m_memory->read< std::uintptr_t >( user_info_table + 0x40 ) + 0xC );
		player_info_t p = m_memory->read< player_info_t >( m_memory->read< uintptr_t >( x + 0x28 + 0x34 * ( index( ) - 1 ) ) );

		out.resize( 32 );
		memcpy( out.data( ), p.name, 32 );
	}

	bool entity_t::is_alive( ) {
		return life_state( ) == 0;
	}
}