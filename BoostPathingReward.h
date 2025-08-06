#pragma once
#include <RLGymSim_CPP/Utils/RewardFunctions/RewardFunction.h>
#include <RLGymSim_CPP/Utils/Gamestates/GameState.h>
#include <RLGymSim_CPP/Utils/CommonValues.h>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cmath>

namespace RLGSC {
    /**
     * BoostPathingReward - Encourages smart boost collection and pathing
     * 
     * This reward function:
     * 1. Rewards collecting big boost pads (1.0) and small boost pads (0.12)
     * 2. Rewards moving towards boost pads when boost is needed
     * 3. Prevents abuse by requiring actual collection within a time window
     * 4. Uses boost necessity scoring to avoid wasteful boost collection
     * 5. Considers strategic positioning relative to game state
     */
    class BoostPathingReward : public RewardFunction {
    public:
        // Boost pad positions and types
        struct BoostPad {
            Vec position;
            bool isBigBoost;
            float reward;
            
            BoostPad(float x, float y, float z, bool big) 
                : position(x, y, z), isBigBoost(big), reward(big ? 1.0f : 0.12f) {}
        };
        
        // Player tracking data
        struct PlayerBoostData {
            int targetBoostId = -1;           // Currently targeted boost pad
            float timeTargeting = 0.0f;       // Time spent targeting current boost
            float lastDistanceToTarget = 0.0f; // Last distance to target boost
            bool wasMovingTowardsTarget = false; // Was moving towards target last frame
            float lastBoostAmount = 0.0f;     // Last frame's boost amount
            float timeSinceLastCollection = 0.0f; // Time since last boost collection
        };

        // Configuration parameters
        float maxTargetTime;              // Max time to target a boost without collecting
        float minBoostThreshold;          // Min boost level to start seeking boost
        float maxApproachDistance;        // Max distance to start rewarding approach
        float approachRewardWeight;       // Weight for approach rewards
        float collectionRewardWeight;     // Weight for collection rewards  
        float necessityWeight;            // Weight for boost necessity factor
        bool enableStrategicPositioning; // Whether to consider strategic positioning

        /**
         * Constructor
         * 
         * @param maxTargetTime Maximum time to target boost without collecting (seconds)
         * @param minBoostThreshold Minimum boost level to encourage boost seeking (0-1)
         * @param maxApproachDistance Maximum distance to reward boost approach
         * @param approachRewardWeight Weight for approach rewards
         * @param collectionRewardWeight Weight for collection rewards  
         * @param necessityWeight Weight for boost necessity calculations
         * @param enableStrategicPositioning Whether to consider game state for boost choice
         */
        BoostPathingReward(
            float maxTargetTime = 5.0f,
            float minBoostThreshold = 0.3f,
            float maxApproachDistance = 2000.0f,
            float approachRewardWeight = 0.1f,
            float collectionRewardWeight = 1.0f,
            float necessityWeight = 0.5f,
            bool enableStrategicPositioning = true
        );

        virtual void Reset(const GameState& initialState) override;
        virtual float GetReward(const PlayerData& player, const GameState& state, const Action& prevAction) override;

    private:
        std::vector<BoostPad> boostPads;
        std::unordered_map<int, PlayerBoostData> playerData;
        
        // Helper methods
        void InitializeBoostPads();
        int FindNearestBoostPad(const Vec& position, float maxDistance, bool needsBigBoost = false) const;
        float CalculateBoostNecessity(const PlayerData& player, const GameState& state) const;
        float CalculateApproachReward(const PlayerData& player, const GameState& state, PlayerBoostData& data) const;
        float CalculateCollectionReward(const PlayerData& player, const GameState& state, PlayerBoostData& data) const;
        bool ShouldTargetBoost(const PlayerData& player, const GameState& state, int boostId) const;
        float CalculateStrategicValue(const PlayerData& player, const GameState& state, int boostId) const;
        void UpdatePlayerTracking(const PlayerData& player, const GameState& state, PlayerBoostData& data);
        bool IsBoostAvailable(const GameState& state, int boostId) const;
    };
} // namespace RLGSC