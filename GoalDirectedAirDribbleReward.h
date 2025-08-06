#pragma once

#include <RLGymSim_CPP/Utils/RewardFunctions/RewardFunction.h>
#include <RLGymSim_CPP/Utils/Gamestates/GameState.h>
#include <RLGymSim_CPP/Utils/CommonValues.h>
#include <cassert>
#include <cmath>
#include <algorithm>

namespace RLGSC {

class GoalDirectedAirDribbleReward : public RewardFunction {
public:
    float min_height;
    float max_distance;
    float proximity_weight;
    float velocity_weight;
    float height_weight;
    float speed_weight;
    float touch_bonus;
    float air_roll_weight;
    float air_roll_min_height;
    float facing_ball_threshold;
    float boost_threshold;
    float speed_threshold;
    float boost_efficiency_weight;
    float goal_direction_strength;      // NEW: How strongly to prefer goal direction
    float wall_penalty_strength;       // NEW: How much to penalize non-goal directions
    bool own_goal;
    bool enable_air_roll_reward;

    // Anti-wiggle tracking
    float cumulative_roll_direction;
    float last_roll_input;
    int direction_changes;
    static constexpr int MAX_DIRECTION_CHANGES = 2;

private:
    float reward_scaling_denominator;
    float height_scaling_denominator;
    ScoreLine prev_score_line;
    float last_player_z;
    float last_ball_z;

public:
    GoalDirectedAirDribbleReward(
        float minHeight = 200.0f,
        float maxDistance = 400.0f,
        float proximityWeight = 1.5f,
        float velocityWeight = 1.2f,
        float heightWeight = 0.3f,
        float speedWeight = 1.0f,
        float touchBonus = 1.0f,
        float airRollWeight = 2.5f,
        float airRollMinHeight = 120.0f,
        float facingBallThreshold = 0.74f,
        float boostThreshold = 0.30f,
        float speedThreshold = 1200.0f,
        float boostEfficiencyWeight = 0.1f,
        float goalDirectionStrength = 2.0f,     // NEW: 2x bonus for perfect goal direction
        float wallPenaltyStrength = 0.2f,       // NEW: 20% reward when going wrong direction
        bool ownGoal = false,
        bool enableAirRollReward = true);

    virtual void Reset(const GameState& initialState) override;
    virtual float GetReward(const PlayerData& player, const GameState& state, const Action& prevAction) override;

private:
    float CalculateGoalDirectionMultiplier(const PlayerData& player, const GameState& state) const;
    Vec GetTargetGoal(const PlayerData& player) const;
};

} // namespace RLGSC