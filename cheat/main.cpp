#include "main.h"

// Calls WinMain; it's like main but for windows
int __stdcall wWinMain(
	HINSTANCE instance,
	HINSTANCE previousInstance,
	PWSTR arguments,
	int commandShow)
{
	try {
		//Create memory objects
		forceinline::memory_manager memory("csgo.exe");
		assert(memory.is_attached());
		Logger("Attached to process");

		// Create DX overlay
		forceinline::dx_overlay overlay(L"Valve001", L"Counter-Strike: Global Offensive - Direct3D 9", false);
		assert(overlay.is_initialized());
		Logger("Created overlay");

		// Create GUI
		gui::CreateHWindow("Cheat Menu", "Cheat Menu Class");
		gui::CreateDevice();
		gui::CreateImGui();
		
		// Create DX renderer
		forceinline::dx_renderer renderer = overlay.create_renderer();
		
		//Create a mutex so we can multithread safely
		std::mutex ent_mtx;

		//Grab the base of the modules
		sdk::modules_t modules(&memory);
		
		while (gui::exit)
		{
			
			std::thread render_gui([&]() -> void {
				while (true) {
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
					
					gui::BeginRender();
					gui::Render();
					gui::EndRender();
					Logger("Gui render thread complete");
				}
				});
			

			//Create a thread to read info so we don't slow down our rendering part
			std::thread read_ent_info([&]() -> void {
				std::vector< sdk::ent_info_t > ent_info(64);

				while (true) {
					std::this_thread::sleep_for(std::chrono::milliseconds(1));

					//Lock our mutex so our ESP thread doesn't do dumb shit
					std::unique_lock lock(ent_mtx);

					//Invalidate all ESP entities as we're updating them
					for (auto& esp_ent : esp::entities)
						esp_ent.valid = false;

					//Get clientstate for the entitylist
					std::uintptr_t client_state = memory.read< std::uintptr_t >(modules.engine_dll + offset::dwClientState); //m_dwClientState

					//Read the whole entity list at once
					memory.read_ex< sdk::ent_info_t >(ent_info.data(), modules.client_dll + offset::dwEntityList, ent_info.size()); //m_dwEntityList

					//Get our local player ptr
					int local_player_idx = memory.read< int >(client_state + offset::dwClientState_GetLocalPlayer); //m_dwClientState_GetLocalPlayer
					std::uintptr_t local_ptr = ent_info[local_player_idx].entity_ptr;

					//Is our local player ptr valid?
					if (!local_ptr)
						continue;

					//Get our local player
					sdk::entity_t local(&memory, &modules, local_ptr);

					//Gather entity information for our ESP
					for (std::size_t i = 0; i < ent_info.size(); i++) {
						std::uintptr_t ent_ptr = ent_info[i].entity_ptr;

						//Entity is invalid, don't draw on ESP
						if (!ent_ptr)
							continue;

						//Create an entity object so we can get information the easy way
						sdk::entity_t entity(&memory, &modules, ent_ptr);

						//Continue if entity is dormant or dead
						if (entity.dormant() || !entity.is_alive())
							continue;

						//We don't want to draw ESP on our team
						if (entity.team() == local.team())
							continue;

						//We have a valid entity, get a reference to it for ease of use
						esp::esp_entity_t& esp_entity = esp::entities[i];

						//Get entity information for our ESP
						esp_entity.health = entity.health();
						entity.get_name(esp_entity.name);
						esp_entity.origin = entity.origin();
						esp_entity.top_origin = esp_entity.origin + sdk::vec3_t(0.f, 0.f, 75.f);

						//Our ESP entity is now valid to draw
						esp_entity.valid = true;
						Logger("Ent info thread complete");
					}
				}
				});

			Logger("Started read thread, continuing");

			//MSG struct for WndProc
			MSG m;
			ZeroMemory(&m, sizeof m);

			//Message and rendering loop
			do {
				if (PeekMessage(&m, overlay.get_overlay_wnd(), NULL, NULL, PM_REMOVE)) {
					TranslateMessage(&m);
					DispatchMessage(&m);
				}

				//Lock the mutex so we don't fuck shit up
				std::unique_lock lock(ent_mtx);

				//Render our ESP
				renderer.begin_rendering();
				renderer.draw_text(std::to_string(renderer.get_fps()), 2, 2, 0xFFFFFFFF, false);
				esp::draw(&memory, &renderer);
				renderer.end_rendering();

				Logger("Render thread complete");
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			} while (m.message != WM_QUIT);

			Logger("Exiting...");
			std::this_thread::sleep_for(std::chrono::seconds(3));

			return 0;
		}
	}
	catch (const std::exception& e) {
		// Log exceptions
		Logger(e.what());
	}
}