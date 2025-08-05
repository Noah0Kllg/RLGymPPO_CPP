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
     * This reward function has multiple components:
     * 1. Jump Touch Reward: Rewards for touching the ball in the air, scaled by height
     * 2. Air Roll Reward: Rewards for air rolling continuously when approaching the ball
     * 3. Double Jump Reward: Small reward for double jumping (+0.2)
     * 4. Approach Speed Bonus: Higher rewards for faster ball approaches
     * 
     * The air roll component uses distance-based scaling to encourage constant air rolling
     * during aerial approaches, with higher rewards the closer the agent gets to the ball.
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
              first_jump_used(false) {
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
            first_jump_used = false;
        }

        // Calculate reward per step
        virtual float GetReward(const PlayerData& player, const GameState& state, const Action& prevAction) override;

    private:
        // Air roll tracking variables
        float last_roll_input;
        int direction_changes;
        float cumulative_roll_direction;
        
        // Jump tracking
        bool first_jump_used;
        
        // Constants
        static constexpr int MAX_DIRECTION_CHANGES = 2;
    };
} // namespace RLGSC