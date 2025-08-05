#include "BoostPathingReward.h"
#include "AdvancedBoostPathingReward.h"
#include "CombinedReward.h"
#include "CommonRewards.h"
#include "../Gamestates/GameState.h"
#include <memory>

// Example of how to use the boost pathing reward functions
namespace RLGSC {
	namespace BoostPathingExamples {
		
		// Example 1: Basic boost pathing reward
		std::shared_ptr<RewardFunction> CreateBasicBoostPathingReward() {
			BoostPathingReward::Config config;
			config.proximityReward = 0.1f;
			config.collectionReward = 1.0f;
			config.pathingReward = 0.05f;
			config.efficiencyReward = 0.2f;
			config.proximityThreshold = 300.0f;
			config.collectionThreshold = 100.0f;
			config.boostEfficiencyThreshold = 0.3f;
			
			return std::make_shared<BoostPathingReward>(config);
		}
		
		// Example 2: Advanced boost pathing reward with timing
		std::shared_ptr<RewardFunction> CreateAdvancedBoostPathingReward() {
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
			
			return std::make_shared<AdvancedBoostPathingReward>(config);
		}
		
		// Example 3: Combined reward with boost pathing and other rewards
		std::shared_ptr<RewardFunction> CreateCombinedBoostPathingReward() {
			// Create boost pathing reward
			auto boostPathing = CreateAdvancedBoostPathingReward();
			
			// Create other common rewards
			auto velocityReward = std::make_shared<VelocityReward>(false);
			auto saveBoostReward = std::make_shared<SaveBoostReward>(0.5f);
			auto touchBallReward = std::make_shared<TouchBallReward>(0.0f);
			
			// Create event reward for goals, touches, etc.
			EventReward::WeightScales eventScales;
			eventScales.goal = 10.0f;
			eventScales.teamGoal = 5.0f;
			eventScales.concede = -5.0f;
			eventScales.touch = 0.1f;
			eventScales.shot = 1.0f;
			eventScales.save = 1.0f;
			eventScales.boostPickup = 0.5f;
			auto eventReward = std::make_shared<EventReward>(eventScales);
			
			// Combine all rewards
			std::vector<std::shared_ptr<RewardFunction>> rewards = {
				boostPathing,
				velocityReward,
				saveBoostReward,
				touchBallReward,
				eventReward
			};
			
			return std::make_shared<CombinedReward>(rewards);
		}
		
		// Example 4: Specialized boost pathing for different play styles
		std::shared_ptr<RewardFunction> CreateAggressiveBoostPathingReward() {
			AdvancedBoostPathingReward::Config config;
			// Aggressive settings - prioritize large pads and timing
			config.proximityReward = 0.03f;
			config.collectionReward = 1.5f;
			config.pathingReward = 0.05f;
			config.timingReward = 0.3f;
			config.efficiencyReward = 0.1f;
			config.strategicReward = 0.15f;
			config.proximityThreshold = 500.0f;
			config.collectionThreshold = 150.0f;
			config.boostEfficiencyThreshold = 0.5f;
			config.timingWindow = 0.3f;
			config.largePadPreference = 3.0f;  // Strong preference for large pads
			config.backWallPreference = 2.0f;
			config.centerPreference = 1.5f;
			
			return std::make_shared<AdvancedBoostPathingReward>(config);
		}
		
		std::shared_ptr<RewardFunction> CreateConservativeBoostPathingReward() {
			AdvancedBoostPathingReward::Config config;
			// Conservative settings - prioritize efficiency and small pads
			config.proximityReward = 0.08f;
			config.collectionReward = 0.8f;
			config.pathingReward = 0.04f;
			config.timingReward = 0.1f;
			config.efficiencyReward = 0.25f;
			config.strategicReward = 0.08f;
			config.proximityThreshold = 350.0f;
			config.collectionThreshold = 100.0f;
			config.boostEfficiencyThreshold = 0.3f;
			config.timingWindow = 0.8f;
			config.largePadPreference = 1.5f;
			config.backWallPreference = 1.2f;
			config.centerPreference = 1.1f;
			
			return std::make_shared<AdvancedBoostPathingReward>(config);
		}
		
		// Example 5: Boost pathing with team coordination
		std::shared_ptr<RewardFunction> CreateTeamBoostPathingReward() {
			// This would require additional logic for team coordination
			// For now, we'll use the advanced reward with team-friendly settings
			AdvancedBoostPathingReward::Config config;
			config.proximityReward = 0.06f;
			config.collectionReward = 1.2f;
			config.pathingReward = 0.04f;
			config.timingReward = 0.15f;
			config.efficiencyReward = 0.2f;
			config.strategicReward = 0.12f;
			config.proximityThreshold = 450.0f;
			config.collectionThreshold = 130.0f;
			config.boostEfficiencyThreshold = 0.35f;
			config.timingWindow = 0.6f;
			config.largePadPreference = 2.5f;
			config.backWallPreference = 1.8f;
			config.centerPreference = 1.4f;
			
			return std::make_shared<AdvancedBoostPathingReward>(config);
		}
	}
}

/*
Boost Pathing Reward Function Documentation

This implementation provides comprehensive boost pathing rewards for Rocket League bots.
The reward functions encourage efficient boost collection behavior through multiple mechanisms:

1. Proximity Rewards:
   - Rewards the agent for being near active boost pads
   - Higher rewards for large pads (100 boost) vs small pads (12 boost)
   - Distance-based scaling (closer = higher reward)

2. Collection Rewards:
   - Rewards successful boost pad collection
   - Only rewards intentional collection (pad was nearby in previous frame)
   - Multipliers for different pad types

3. Pathing Rewards:
   - Rewards movement toward nearby boost pads
   - Uses dot product to measure alignment with pad direction
   - Distance-based scaling

4. Timing Rewards (Advanced):
   - Rewards collecting pads just as they spawn
   - Encourages efficient timing and prediction
   - Higher rewards for quick collection after spawn

5. Efficiency Rewards:
   - Rewards being near boost when boost is low
   - Encourages proactive boost management
   - Prevents boost starvation

6. Strategic Rewards (Advanced):
   - Rewards choosing high-priority boost pads
   - Considers pad type, location, and distance
   - Encourages strategic decision making

Configuration Parameters:

Basic Parameters:
- proximityReward: Reward for being near boost pads
- collectionReward: Reward for collecting boost pads
- pathingReward: Reward for moving toward boost pads
- efficiencyReward: Reward for efficient boost usage
- proximityThreshold: Distance threshold for "nearby" pads
- collectionThreshold: Distance threshold for collection
- boostEfficiencyThreshold: Boost level below which efficiency is rewarded

Advanced Parameters:
- timingReward: Reward for collecting pads just as they spawn
- strategicReward: Reward for strategic pad selection
- timingWindow: Time window after spawn for timing rewards
- largePadPreference: Multiplier for large pad preference
- backWallPreference: Multiplier for back wall pad preference
- centerPreference: Multiplier for center pad preference

Usage Examples:

1. Basic boost pathing:
   auto reward = BoostPathingExamples::CreateBasicBoostPathingReward();

2. Advanced boost pathing with timing:
   auto reward = BoostPathingExamples::CreateAdvancedBoostPathingReward();

3. Combined with other rewards:
   auto reward = BoostPathingExamples::CreateCombinedBoostPathingReward();

4. Aggressive play style:
   auto reward = BoostPathingExamples::CreateAggressiveBoostPathingReward();

5. Conservative play style:
   auto reward = BoostPathingExamples::CreateConservativeBoostPathingReward();

Boost Pad Locations:
The system uses the standard Rocket League boost pad locations:
- 34 total boost pads
- Large pads (100 boost): Center, corners, back walls
- Small pads (12 boost): Throughout the field
- All positions are hardcoded in CommonValues::BOOST_LOCATIONS

The reward functions automatically handle:
- Boost pad state tracking (active/inactive)
- Cooldown timers
- Player position and movement
- Boost fraction tracking
- Historical data for intentional collection detection

This implementation provides a solid foundation for training Rocket League bots with sophisticated boost management skills.
*/