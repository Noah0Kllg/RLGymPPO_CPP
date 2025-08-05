#pragma once
#include <RLGymSim_CPP/Utils/RewardFunctions/RewardFunction.h>
#include <RLGymSim_CPP/Utils/Gamestates/GameState.h>
#include <RLGymSim_CPP/Utils/CommonValues.h>
#include <algorithm> // For std::clamp
#include <cmath> // For std::abs

namespace RLGSC {
    /**
     * Enhanced JumpTouchReward - Rewards touching the ball while airborne with air roll
     * 
     * This reward function has two main components:
     * 1. Jump Touch Reward: Rewards for touching the ball in the air, scaled by height
     * 2. Air Roll Reward: Rewards for air rolling, especially when approaching and touching the ball
     * 
     * The air roll component is designed to encourage the agent to air roll during aerial
     * challenges and 50/50s, similar to how human players use air roll for better control.
     * 
     * Anti-farming measures prevent the agent from performing normal aerials and then
     * air rolling after the touch to farm bonus rewards.
     */
    class JumpTouchReward : public RewardFunction {
    public:
        // Core parameters
        float minHeight;        // Minimum height for jump touch rewards
        float maxHeight;        // Maximum height for scaling rewards
        float range;            // Calculated range between min and max height
        float air_roll_weight;  // Weight for air roll rewards
        float air_roll_min_height; // Minimum height for air roll rewards

        /**
         * Constructor to set reward parameters
         * 
         * @param minHeight Minimum ball height for jump touch rewards
         * @param maxHeight Maximum ball height for scaling rewards
         * @param airRollWeight Weight for air roll rewards
         * @param airRollMinHeight Minimum car height for air roll rewards
         */
        JumpTouchReward(float minHeight = 120.0f, float maxHeight = 1000.0f, 
                        float airRollWeight = 2.5f, float airRollMinHeight = 160.0f)
            : minHeight(minHeight), maxHeight(maxHeight), range(maxHeight - minHeight), 
              air_roll_weight(airRollWeight), air_roll_min_height(airRollMinHeight),
              last_roll_input(0.0f), direction_changes(0), cumulative_roll_direction(0.0f),
              ball_touched_recently(0), air_roll_cooldown_steps(30) {
                // Ensure range is positive to avoid division issues
                if (range <= 0) {
                    this->range = 1.0f;
                }
            }

        // Reset air roll tracking state
        virtual void Reset(const GameState& initialState) override {
            last_roll_input = 0.0f;
            direction_changes = 0;
            cumulative_roll_direction = 0.0f;
            ball_touched_recently = 0;
        }

        // Calculate reward per step
        virtual float GetReward(const PlayerData& player, const GameState& state, const Action& prevAction) override;

    private:
        // Air roll tracking variables
        float last_roll_input;
        int direction_changes;
        float cumulative_roll_direction;
        
        // Anti-farming variables
        int ball_touched_recently;      // Countdown after ball touch
        const int air_roll_cooldown_steps; // Steps to prevent air roll rewards after touch
        
        // Constants
        static constexpr int MAX_DIRECTION_CHANGES = 2;
    };
} // namespace RLGSC