#include "BoostPathingReward.h"
#include "AdvancedBoostPathingReward.h"
#include "../Gamestates/GameState.h"
#include "../BasicTypes/Action.h"
#include <iostream>
#include <memory>

// Simple test to demonstrate boost pathing reward functions
namespace RLGSC {
	namespace BoostPathingTest {
		
		void TestBasicBoostPathing() {
			std::cout << "Testing Basic Boost Pathing Reward..." << std::endl;
			
			// Create reward function
			BoostPathingReward::Config config;
			config.proximityReward = 0.1f;
			config.collectionReward = 1.0f;
			config.pathingReward = 0.05f;
			config.efficiencyReward = 0.2f;
			config.proximityThreshold = 300.0f;
			config.collectionThreshold = 100.0f;
			config.boostEfficiencyThreshold = 0.3f;
			
			auto reward = std::make_shared<BoostPathingReward>(config);
			
			// Create test game state
			GameState state;
			state.deltaTime = 1.0f / 120.0f; // 120 FPS
			
			// Add a test player
			PlayerData player;
			player.carId = 1;
			player.team = Team::BLUE;
			player.phys.pos = Vec(0, 0, 0); // Center of field
			player.boostFraction = 0.5f;
			player.boostPickups = 0;
			state.players.push_back(player);
			
			// Set up boost pads (activate some pads)
			for (int i = 0; i < CommonValues::BOOST_LOCATIONS_AMOUNT; i++) {
				state.boostPads[i] = (i % 3 == 0); // Activate every 3rd pad
				state.boostPadTimers[i] = 0.0f;
			}
			
			// Create test action
			Action action;
			action.throttle = 1.0f;
			action.steer = 0.0f;
			action.pitch = 0.0f;
			action.yaw = 0.0f;
			action.roll = 0.0f;
			action.jump = false;
			action.boost = false;
			action.handbrake = false;
			
			// Initialize reward function
			reward->Reset(state);
			
			// Test reward calculation
			float rewardValue = reward->GetReward(player, state, action);
			std::cout << "Basic reward value: " << rewardValue << std::endl;
			
			// Test with player near a boost pad
			player.phys.pos = CommonValues::BOOST_LOCATIONS[0]; // Move to first boost pad
			state.players[0] = player;
			
			rewardValue = reward->GetReward(player, state, action);
			std::cout << "Reward near boost pad: " << rewardValue << std::endl;
			
			// Test with low boost
			player.boostFraction = 0.1f;
			state.players[0] = player;
			
			rewardValue = reward->GetReward(player, state, action);
			std::cout << "Reward with low boost: " << rewardValue << std::endl;
		}
		
		void TestAdvancedBoostPathing() {
			std::cout << "\nTesting Advanced Boost Pathing Reward..." << std::endl;
			
			// Create reward function
			AdvancedBoostPathingReward::Config config;
			config.proximityReward = 0.05f;
			config.collectionReward = 1.0f;
			config.pathingReward = 0.03f;
			config.timingReward = 0.2f;
			config.efficiencyReward = 0.15f;
			config.strategicReward = 0.1f;
			config.proximityThreshold = 400.0f;
			config.collectionThreshold = 120.0f;
			config.boostEfficiencyThreshold = 0.4f;
			config.timingWindow = 0.5f;
			config.largePadPreference = 2.0f;
			config.backWallPreference = 1.5f;
			config.centerPreference = 1.3f;
			
			auto reward = std::make_shared<AdvancedBoostPathingReward>(config);
			
			// Create test game state
			GameState state;
			state.deltaTime = 1.0f / 120.0f;
			
			// Add a test player
			PlayerData player;
			player.carId = 1;
			player.team = Team::BLUE;
			player.phys.pos = Vec(0, 0, 0);
			player.boostFraction = 0.5f;
			player.boostPickups = 0;
			state.players.push_back(player);
			
			// Set up boost pads
			for (int i = 0; i < CommonValues::BOOST_LOCATIONS_AMOUNT; i++) {
				state.boostPads[i] = (i % 2 == 0); // Activate every 2nd pad
				state.boostPadTimers[i] = 0.0f;
			}
			
			// Create test action
			Action action;
			action.throttle = 1.0f;
			action.steer = 0.0f;
			action.pitch = 0.0f;
			action.yaw = 0.0f;
			action.roll = 0.0f;
			action.jump = false;
			action.boost = false;
			action.handbrake = false;
			
			// Initialize reward function
			reward->Reset(state);
			
			// Test reward calculation
			float rewardValue = reward->GetReward(player, state, action);
			std::cout << "Advanced reward value: " << rewardValue << std::endl;
			
			// Test with player near a large boost pad (center)
			player.phys.pos = Vec(0, 0, 73); // Center large pad
			state.players[0] = player;
			
			rewardValue = reward->GetReward(player, state, action);
			std::cout << "Reward near large pad: " << rewardValue << std::endl;
			
			// Test with player near a back wall pad
			player.phys.pos = Vec(0, 4240, 70); // Back wall pad
			state.players[0] = player;
			
			rewardValue = reward->GetReward(player, state, action);
			std::cout << "Reward near back wall pad: " << rewardValue << std::endl;
		}
		
		void TestBoostPadDetection() {
			std::cout << "\nTesting Boost Pad Detection..." << std::endl;
			
			// Test large pad detection
			AdvancedBoostPathingReward::Config config;
			auto reward = std::make_shared<AdvancedBoostPathingReward>(config);
			
			// Test center pad
			Vec centerPad(0, 0, 73);
			bool isLarge = reward->IsLargePad(centerPad);
			std::cout << "Center pad (0, 0, 73) is large: " << (isLarge ? "Yes" : "No") << std::endl;
			
			// Test corner pad
			Vec cornerPad(-3072, -4096, 73);
			isLarge = reward->IsLargePad(cornerPad);
			std::cout << "Corner pad (-3072, -4096, 73) is large: " << (isLarge ? "Yes" : "No") << std::endl;
			
			// Test back wall pad
			Vec backWallPad(0, 4240, 70);
			isLarge = reward->IsLargePad(backWallPad);
			std::cout << "Back wall pad (0, 4240, 70) is large: " << (isLarge ? "Yes" : "No") << std::endl;
			
			// Test small pad
			Vec smallPad(1024, 0, 70);
			isLarge = reward->IsLargePad(smallPad);
			std::cout << "Small pad (1024, 0, 70) is large: " << (isLarge ? "Yes" : "No") << std::endl;
		}
		
		void RunAllTests() {
			std::cout << "=== Boost Pathing Reward Function Tests ===" << std::endl;
			
			TestBasicBoostPathing();
			TestAdvancedBoostPathing();
			TestBoostPadDetection();
			
			std::cout << "\n=== Tests Complete ===" << std::endl;
		}
	}
}

// Example usage in main function (uncomment to test)
/*
int main() {
    RLGSC::BoostPathingTest::RunAllTests();
    return 0;
}
*/