#pragma once
#include <RLGymSim_CPP/Utils/RewardFunctions/RewardFunction.h>
#include <RLGymSim_CPP/Utils/Gamestates/GameState.h>
#include <RLGymSim_CPP/Utils/CommonValues.h>
#include <unordered_map>
#include <cmath>
#include <vector>

namespace RLGSC {
    /**
     * ImprovedStrategicDemoReward - Rewards strategic demolitions and bumps
     * 
     * This reward function:
     * 1. Rewards actual demos (1.0x) and bumps (0.5x)
     * 2. Rewards approaching opponents at supersonic speed
     * 3. Uses strategic context for better targeting
     * 4. Clear velocity-based approach rewards
     * 5. Simpler detection for better learning signals
     */
    class ImprovedStrategicDemoReward : public RewardFunction {
    public:
        // Configuration parameters
        float baseDemoReward;           // Base reward for successful demo
        float bumpMultiplier;           // Multiplier for bumps (0.5x demo reward)
        float approachRewardWeight;     // Weight for approaching opponents
        float strategicMultiplier;      // Bonus for strategic demos/bumps
        float minApproachSpeed;         // Minimum speed to get approach rewards
        float maxApproachDistance;     // Maximum distance for approach rewards
        float cooldownTime;            // Cooldown between demo attempts on same player
        bool enableBumpRewards;        // Whether to reward bumps as well as demos

        // Player tracking data
        struct PlayerDemoData {
            std::unordered_map<int, float> lastDemoTime;      // Last demo time for each opponent
            std::unordered_map<int, float> lastBumpTime;      // Last bump time for each opponent
            std::unordered_map<int, Vec> lastOpponentPos;     // Last known opponent positions
            std::unordered_map<int, float> lastOpponentSpeed; // Last known opponent speeds
            float gameTime = 0.0f;                            // Current game time
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
         * @param cooldownTime Cooldown between attempts on same opponent
         * @param enableBumpRewards Whether to reward bumps
         */
        ImprovedStrategicDemoReward(
            float baseDemoReward = 1.0f,
            float bumpMultiplier = 0.5f,
            float approachRewardWeight = 0.1f,
            float strategicMultiplier = 2.0f,
            float minApproachSpeed = 1000.0f,
            float maxApproachDistance = 1500.0f,
            float cooldownTime = 3.0f,
            bool enableBumpRewards = true
        );

        virtual void Reset(const GameState& initialState) override;
        virtual float GetReward(const PlayerData& player, const GameState& state, const Action& prevAction) override;

    private:
        std::unordered_map<int, PlayerDemoData> playerData;
        
        // Helper methods
        float CalculateApproachReward(const PlayerData& player, const PlayerData& opponent, PlayerDemoData& data) const;
        float CalculateStrategicValue(const PlayerData& player, const PlayerData& opponent, const GameState& state) const;
        bool DetectDemo(const PlayerData& player, const PlayerData& opponent, PlayerDemoData& data) const;
        bool DetectBump(const PlayerData& player, const PlayerData& opponent, PlayerDemoData& data) const;
        bool IsValidTarget(const PlayerData& player, const PlayerData& opponent, const GameState& state, PlayerDemoData& data) const;
        float CalculateVelocityTowardsOpponent(const PlayerData& player, const PlayerData& opponent) const;
    };
} // namespace RLGSC