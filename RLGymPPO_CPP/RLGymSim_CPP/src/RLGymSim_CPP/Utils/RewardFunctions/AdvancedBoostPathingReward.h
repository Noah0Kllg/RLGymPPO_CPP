#pragma once
#include "RewardFunction.h"
#include "../CommonValues.h"
#include <unordered_map>
#include <vector>

namespace RLGSC {
	// Advanced boost pathing reward function with timing and strategic considerations
	class AdvancedBoostPathingReward : public RewardFunction {
	public:
		struct BoostPadState {
			Vec position;
			bool isLargePad;
			float cooldownTimer;
			bool isActive;
			float lastCollectionTime;
			int collectionCount;
		};

		struct PlayerBoostHistory {
			Vec position;
			float boostFraction;
			float timestamp;
			std::vector<bool> nearbyPads;
		};

		struct Config {
			// Basic rewards
			float proximityReward = 0.05f;
			float collectionReward = 1.0f;
			float pathingReward = 0.03f;
			
			// Timing-based rewards
			float timingReward = 0.2f;           // Reward for collecting pads just as they spawn
			float efficiencyReward = 0.15f;      // Reward for efficient boost usage
			float strategicReward = 0.1f;        // Reward for strategic pad selection
			
			// Thresholds
			float proximityThreshold = 400.0f;
			float collectionThreshold = 120.0f;
			float boostEfficiencyThreshold = 0.4f;
			float timingWindow = 0.5f;           // seconds after pad spawn to get timing reward
			
			// Strategic parameters
			float largePadPreference = 2.0f;     // Multiplier for preferring large pads
			float backWallPreference = 1.5f;     // Multiplier for back wall pads
			float centerPreference = 1.3f;       // Multiplier for center pads
		};

		AdvancedBoostPathingReward(const Config& config = Config{});

		virtual void Reset(const GameState& initialState) override;
		virtual void PreStep(const GameState& state) override;
		virtual float GetReward(const PlayerData& player, const GameState& state, const Action& prevAction) override;

	private:
		Config config;
		std::unordered_map<uint32_t, std::vector<PlayerBoostHistory>> playerHistories;
		std::vector<BoostPadState> boostPadStates;
		float currentTime;

		// Helper functions
		void InitializeBoostPadStates();
		void UpdateBoostPadStates(const GameState& state);
		bool IsLargePad(const Vec& position) const;
		bool IsBackWallPad(const Vec& position) const;
		bool IsCenterPad(const Vec& position) const;
		
		// Reward calculation functions
		float CalculateProximityReward(const PlayerData& player, const GameState& state);
		float CalculateCollectionReward(const PlayerData& player, const GameState& state);
		float CalculatePathingReward(const PlayerData& player, const GameState& state);
		float CalculateTimingReward(const PlayerData& player, const GameState& state);
		float CalculateEfficiencyReward(const PlayerData& player, const GameState& state);
		float CalculateStrategicReward(const PlayerData& player, const GameState& state);
		
		// Utility functions
		std::vector<float> CalculatePadDistances(const Vec& playerPos, const GameState& state);
		std::vector<float> CalculatePadPriorities(const Vec& playerPos, const GameState& state);
		bool IsPadNearby(const Vec& playerPos, const Vec& padPos) const;
		bool IsPadCollectible(const Vec& playerPos, const Vec& padPos) const;
		float GetPadPreferenceMultiplier(const Vec& padPos) const;
		void UpdatePlayerHistory(const PlayerData& player, const GameState& state);
	};
}