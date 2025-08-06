#pragma once
#include <RLGymSim_CPP/Utils/RewardFunctions/RewardFunction.h>
#include <RLGymSim_CPP/Utils/Gamestates/GameState.h>
#include <RLGymSim_CPP/Utils/CommonValues.h>
#include <unordered_map>
#include <cmath>

namespace RLGSC {
    /**
     * SimplifiedStrategicDemoReward - Uses built-in bump/demo detection
     * 
     * This reward function:
     * 1. Uses PlayerData.matchDemos and matchBumps for reliable detection
     * 2. Rewards demos (1.0x) and bumps (0.5x) 
     * 3. Rewards approaching opponents at high speed
     * 4. Strategic context multipliers for better targeting
     * 5. Much simpler and more reliable than custom detection
     */
    class SimplifiedStrategicDemoReward : public RewardFunction {
    public:
        // Configuration parameters
        float baseDemoReward;           // Base reward for successful demo
        float bumpMultiplier;           // Multiplier for bumps (0.5x demo reward)
        float approachRewardWeight;     // Weight for approaching opponents
        float strategicMultiplier;      // Bonus for strategic demos/bumps
        float minApproachSpeed;         // Minimum speed to get approach rewards
        float maxApproachDistance;     // Maximum distance for approach rewards
        float cooldownTime;            // Cooldown between approach rewards on same player

        // Player tracking data
        struct PlayerTrackingData {
            int lastMatchDemos = 0;          // Last known demo count
            int lastMatchBumps = 0;          // Last known bump count
            std::unordered_map<int, float> lastApproachRewardTime; // Cooldown for approach rewards
            float gameTime = 0.0f;           // Current game time
        };

        /**
         * Constructor
         * 
         * @param baseDemoReward Base reward for successful demolition
         * @param bumpMultiplier Multiplier for bump rewards (relative to demo)
         * @param approachRewardWeight Weight for velocity-based approach rewards
         * @param strategicMultiplier Bonus multiplier for strategic demos/bumps
         * @param minApproachSpeed Minimum speed to get approach rewards
         * @param maxApproachDistance Maximum distance for approach rewards
         * @param cooldownTime Cooldown between approach rewards on same opponent
         */
        SimplifiedStrategicDemoReward(
            float baseDemoReward = 1.0f,
            float bumpMultiplier = 0.5f,
            float approachRewardWeight = 0.1f,
            float strategicMultiplier = 2.0f,
            float minApproachSpeed = 1000.0f,
            float maxApproachDistance = 1500.0f,
            float cooldownTime = 2.0f
        );

        virtual void Reset(const GameState& initialState) override;
        virtual float GetReward(const PlayerData& player, const GameState& state, const Action& prevAction) override;

    private:
        std::unordered_map<int, PlayerTrackingData> playerData;
        
        // Helper methods
        float CalculateApproachReward(const PlayerData& player, const PlayerData& opponent, PlayerTrackingData& data) const;
        float CalculateStrategicValue(const PlayerData& player, const PlayerData& opponent, const GameState& state) const;
        bool IsValidApproachTarget(const PlayerData& player, const PlayerData& opponent, PlayerTrackingData& data) const;
        float CalculateVelocityTowardsOpponent(const PlayerData& player, const PlayerData& opponent) const;
    };
} // namespace RLGSC