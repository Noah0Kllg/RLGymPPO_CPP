#pragma once
#include "RewardFunction.h"
#include "../CommonValues.h"
#include <unordered_map>

namespace RLGSC {
	// Boost pathing reward function that rewards efficient boost collection
	class BoostPathingReward : public RewardFunction {
	public:
		struct BoostPadInfo {
			Vec position;
			bool isLargePad;  // true for 100 boost pads, false for 12 boost pads
			float collectionRadius;  // radius within which the pad can be collected
		};

		struct PlayerBoostState {
			Vec lastPosition;
			float lastBoostFraction;
			int boostPickups;
			std::vector<bool> nearbyPads;  // tracks which pads were nearby last frame
			std::vector<float> padDistances;  // distance to each pad
		};

		// Configuration parameters
		struct Config {
			float proximityReward = 0.1f;        // reward for being near a boost pad
			float collectionReward = 1.0f;       // reward for collecting a boost pad
			float pathingReward = 0.05f;         // reward for moving toward nearby pads
			float efficiencyReward = 0.2f;       // reward for efficient boost usage
			float proximityThreshold = 300.0f;   // distance threshold for "nearby" pads
			float collectionThreshold = 100.0f;  // distance threshold for collection
			float boostEfficiencyThreshold = 0.3f; // boost level below which efficiency is rewarded
		};

		BoostPathingReward(const Config& config = Config{});

		virtual void Reset(const GameState& initialState) override;
		virtual void PreStep(const GameState& state) override;
		virtual float GetReward(const PlayerData& player, const GameState& state, const Action& prevAction) override;

	private:
		Config config;
		std::unordered_map<uint32_t, PlayerBoostState> playerStates;
		std::vector<BoostPadInfo> boostPadInfos;

		// Helper functions
		void InitializeBoostPadInfos();
		bool IsLargePad(const Vec& position) const;
		float CalculateProximityReward(const PlayerData& player, const GameState& state);
		float CalculatePathingReward(const PlayerData& player, const GameState& state);
		float CalculateEfficiencyReward(const PlayerData& player, const GameState& state);
		float CalculateCollectionReward(const PlayerData& player, const GameState& state);
		std::vector<float> CalculatePadDistances(const Vec& playerPos, const GameState& state);
		bool IsPadNearby(const Vec& playerPos, const Vec& padPos) const;
		bool IsPadCollectible(const Vec& playerPos, const Vec& padPos) const;
	};
}