#include "EnhancedAirDribbleReward.h"
#include <algorithm>
#include <iostream>

namespace RLGSC {

void EnhancedAirDribbleReward::Reset(const GameState& initialState) {
    prev_score_line = initialState.scoreLine;
    last_player_z = 0.0f;
    last_ball_z = 0.0f;
    cumulative_roll_direction = 0.0f;
    last_roll_input = 0.0f;
    direction_changes = 0;
}

EnhancedAirDribbleReward::EnhancedAirDribbleReward(
    float minHeight,
    float maxDistance,
    float proximityWeight,
    float velocityWeight,
    float heightWeight,
    float speedWeight,
    float touchBonus,
    float airRollWeight,
    float airRollMinHeight,
    float facingBallThreshold,
    float boostThreshold,
    float speedThreshold,
    float boostEfficiencyWeight,
    float goalDirectionStrength,
    float wallPenaltyStrength,
    bool ownGoal,
    bool enableAirRollReward
) :
    min_height(minHeight),
    max_distance(maxDistance),
    proximity_weight(proximityWeight),
    velocity_weight(velocityWeight),
    height_weight(heightWeight),
    speed_weight(speedWeight),
    touch_bonus(touchBonus),
    air_roll_weight(airRollWeight),
    air_roll_min_height(airRollMinHeight),
    facing_ball_threshold(facingBallThreshold),
    boost_threshold(boostThreshold),
    speed_threshold(speedThreshold),
    boost_efficiency_weight(boostEfficiencyWeight),
    goal_direction_strength(goalDirectionStrength),
    wall_penalty_strength(wallPenaltyStrength),
    own_goal(ownGoal),
    enable_air_roll_reward(enableAirRollReward),
    last_player_z(0.0f),
    last_ball_z(0.0f),
    cumulative_roll_direction(0.0f),
    last_roll_input(0.0f),
    direction_changes(0)
{
    float totalWeight = proximity_weight + velocity_weight + height_weight + speed_weight;
    assert(totalWeight > 1e-6f && "Sum of weights must be positive");
    assert(max_distance > CommonValues::BALL_RADIUS && "max_distance must be greater than ball radius");

    reward_scaling_denominator = std::max(1e-6f, max_distance - CommonValues::BALL_RADIUS);
    height_scaling_denominator = std::max(1e-6f, CommonValues::CEILING_Z - min_height);
}

float EnhancedAirDribbleReward::GetReward(const PlayerData& player, const GameState& state, const Action& prevAction) {
    const Vec& car_pos = player.phys.pos;
    const Vec& ball_pos = state.ball.pos;
    float reward = 0.0f;

    // Check if player is airborne and at minimum height
    bool is_airborne = !player.carState.isOnGround && car_pos.z >= this->min_height;
    bool ball_airborne = ball_pos.z >= this->min_height;
    
    // CRITICAL: Check if player has enough boost for air dribble (at least 20%)
    if (player.boostFraction < 0.2f) {
        return 0.0f;
    }

    // Calculate distance to ball
    Vec dist_vec = ball_pos - car_pos;
    float dist = dist_vec.Length();

    // Calculate facing direction towards ball
    float facing_ball = 0.0f;
    if (dist > 1e-6f) {
        Vec dir_to_ball = dist_vec / dist;
        Vec car_forward = player.phys.rotMat.forward;
        facing_ball = car_forward.Dot(dir_to_ball);
    }

    // Enhanced Air Roll Reward
    float air_roll_reward = 0.0f;
    if (is_airborne && car_pos.z > this->air_roll_min_height && !player.carState.isFlipping) {
        float current_roll_input = prevAction.roll;
        float current_angular_vel = player.phys.angVel.y;
        
        // Track direction changes (detect wiggling)
        if (std::abs(current_roll_input) > 0.1f && std::abs(this->last_roll_input) > 0.1f) {
            bool direction_changed = (current_roll_input * this->last_roll_input) < 0;
            if (direction_changed) {
                this->direction_changes++;
            }
        }
        
        // Decay direction changes over time
        if (this->direction_changes > 0) {
            this->direction_changes = std::max(0, this->direction_changes - 1);
        }
        
        // Update cumulative roll direction
        this->cumulative_roll_direction += current_roll_input;
        this->cumulative_roll_direction *= 0.98f;
        
        // Only grant roll reward if not wiggling and within range
        bool is_wiggling = this->direction_changes > MAX_DIRECTION_CHANGES;
        
        if (enable_air_roll_reward && !is_wiggling && std::abs(current_roll_input) > 0.1f && dist <= 1000.0f) {
            float roll_angular_vel = std::abs(current_angular_vel);
            float roll_intensity = std::min(1.0f, roll_angular_vel / CommonValues::CAR_MAX_ANG_VEL);
            
            // Base air roll reward
            air_roll_reward = roll_intensity * this->air_roll_weight;
            
            // Right roll preference
            if (current_roll_input > 0.1f) {
                air_roll_reward *= 1.5f;
                if (this->cumulative_roll_direction > 2.0f) {
                    air_roll_reward *= 1.3f;
                }
            } else {
                air_roll_reward *= 0.3f;
            }
            
            // Boost + air roll bonus
            if (prevAction.boost && player.boostFraction > 0.2f) {
                air_roll_reward *= 1.4f;
            }
            
            // Enhanced reward during air dribbling conditions
            if (ball_airborne && dist < this->max_distance && facing_ball > this->facing_ball_threshold) {
                air_roll_reward *= 1.8f;
            }
        }
    }

    // MAIN AIR DRIBBLE REWARD with INTEGRATED GOAL DIRECTION
    if (is_airborne && ball_airborne && dist <= this->max_distance) {
        
        // 1. Proximity Reward
        float proximity_factor = std::max(0.0f, 1.0f - (dist - CommonValues::BALL_RADIUS) / this->reward_scaling_denominator);
        float prox_reward = proximity_factor * this->proximity_weight;

        // 2. Controlled Velocity Towards Ball
        float vel_reward = 0.0f;
        if (dist > 1e-6f) {
            Vec dir_to_ball = dist_vec / dist;
            const Vec& player_vel = player.phys.vel;
            float speed_towards_ball = player_vel.Dot(dir_to_ball);

            if (speed_towards_ball > 0) {
                float optimal_approach_speed = CommonValues::CAR_MAX_SPEED * 0.4f;
                float approach_factor;
                
                if (speed_towards_ball <= optimal_approach_speed) {
                    approach_factor = speed_towards_ball / optimal_approach_speed;
                } else {
                    float excess_ratio = (speed_towards_ball - optimal_approach_speed) / (CommonValues::CAR_MAX_SPEED - optimal_approach_speed);
                    approach_factor = 1.0f - (excess_ratio * 0.5f);
                }
                
                vel_reward = approach_factor * this->velocity_weight * 0.6f;
            }
        }

        // 3. Height Reward
        float current_height = car_pos.z;
        float height_factor = std::max(0.0f, (current_height - this->min_height) / this->height_scaling_denominator);
        float height_reward = height_factor * this->height_weight;

        // 4. Controlled Speed Reward
        float controlled_speed_reward = 0.0f;
        float player_speed = player.phys.vel.Length();
        
        float optimal_min_speed = 800.0f;
        float optimal_max_speed = 1400.0f;
        
        if (player_speed >= optimal_min_speed && player_speed <= optimal_max_speed) {
            float control_factor = 1.0f - std::abs(player_speed - (optimal_min_speed + optimal_max_speed) / 2.0f) / ((optimal_max_speed - optimal_min_speed) / 2.0f);
            controlled_speed_reward = control_factor * this->speed_weight * 0.8f;
        } else if (player_speed > optimal_max_speed) {
            float excess_speed_penalty = std::min(0.5f, (player_speed - optimal_max_speed) / 1000.0f);
            controlled_speed_reward = -excess_speed_penalty * this->speed_weight * 0.3f;
        }
        
        // 5. Boost Conservation Reward
        float boost_conservation_reward = 0.0f;
        if (is_airborne && ball_airborne && dist < this->max_distance) {
            if (player.boostFraction > 0.3f) {
                float boost_factor = std::pow(player.boostFraction, 0.7f);
                boost_conservation_reward = boost_factor * this->boost_efficiency_weight;
                
                if (player.boostFraction > 0.7f) {
                    boost_conservation_reward *= 1.4f;
                }
            }
        }

        // 6. Ascending Bonus
        float ascending_bonus = 0.0f;
        bool player_ascending = car_pos.z > this->last_player_z;
        bool ball_ascending = ball_pos.z > this->last_ball_z;
        if (player_ascending && ball_ascending && car_pos.z < ball_pos.z) {
            ascending_bonus = 0.5f;
        }

        // 7. Position Bonus
        float position_bonus = 0.0f;
        if (car_pos.z < ball_pos.z && facing_ball > this->facing_ball_threshold) {
            position_bonus = 0.3f;
        }

        // *** CRITICAL: CALCULATE GOAL DIRECTION MULTIPLIER ***
        float goalDirectionMultiplier = CalculateGoalDirectionMultiplier(player, state);

        // Combine base air dribble rewards
        float base_reward = prox_reward + vel_reward + height_reward + controlled_speed_reward + boost_conservation_reward + ascending_bonus + position_bonus;
        float total_weight = this->proximity_weight + this->velocity_weight + this->height_weight + this->speed_weight + this->boost_efficiency_weight + 1.3f;
        
        if (total_weight > 1e-6f) {
            // *** APPLY GOAL DIRECTION MULTIPLIER TO ALL AIR DRIBBLE REWARDS ***
            reward += (base_reward / total_weight) * goalDirectionMultiplier;
        }

        // Touch Bonus with Goal Direction
        if (player.ballTouchedStep) {
            float touch_multiplier = 1.0f;
            
            // Controlled touch bonus
            if (player_speed >= 800.0f && player_speed <= 1400.0f) {
                touch_multiplier *= 2.5f;
            } else if (player_speed > 1400.0f) {
                float speed_penalty = std::min(0.7f, (player_speed - 1400.0f) / 1000.0f);
                touch_multiplier *= (1.5f - speed_penalty);
            } else {
                touch_multiplier *= 1.2f;
            }
            
            // Boost management bonus
            if (player.boostFraction > this->boost_threshold) {
                touch_multiplier *= 1.8f;
            }
            
            // Proximity bonus
            if (dist < CommonValues::BALL_RADIUS + 150.0f) {
                touch_multiplier *= 1.4f;
            }
            
            // *** APPLY GOAL DIRECTION TO TOUCH BONUS TOO ***
            reward += (this->touch_bonus * touch_multiplier) * goalDirectionMultiplier;
        }

        // Height scaling factor
        float height_scaling = ball_pos.z / 750.0f;
        reward *= std::max(0.1f, std::min(2.0f, height_scaling));
    }

    // Add air roll reward (separate from air dribble)
    reward += air_roll_reward;

    // Scoring Bonus
    bool scored = false;
    if (player.team == Team::BLUE && state.scoreLine[(int)Team::BLUE] > prev_score_line[(int)Team::BLUE]) {
        scored = true;
    } else if (player.team == Team::ORANGE && state.scoreLine[(int)Team::ORANGE] > prev_score_line[(int)Team::ORANGE]) {
        scored = true;
    }

    if (scored && state.lastTouchCarID == player.carId) {
        reward *= 5.0f; // MASSIVE scoring bonus for goal-directed air dribble goals
    }

    // Update state tracking
    prev_score_line = state.scoreLine;
    last_player_z = car_pos.z;
    last_ball_z = ball_pos.z;
    last_roll_input = prevAction.roll;

    return reward;
}

Vec EnhancedAirDribbleReward::GetTargetGoal(const PlayerData& player) const {
    bool targetOrangeGoal = player.team == Team::BLUE;
    if (this->own_goal) {
        targetOrangeGoal = !targetOrangeGoal;
    }
    
    return targetOrangeGoal ? CommonValues::ORANGE_GOAL_BACK : CommonValues::BLUE_GOAL_BACK;
}

float EnhancedAirDribbleReward::CalculateGoalDirectionMultiplier(const PlayerData& player, const GameState& state) const {
    // Get target goal
    Vec targetGoal = GetTargetGoal(player);
    
    // Direction from ball to goal
    Vec ballToGoal = (targetGoal - state.ball.pos).Normalized();
    
    // Ball velocity direction (normalized)
    Vec ballVelNormalized = state.ball.vel;
    float ballSpeed = ballVelNormalized.Length();
    
    if (ballSpeed < 50.0f) {
        return 1.0f; // Neutral if ball is barely moving
    }
    
    ballVelNormalized = ballVelNormalized / ballSpeed;
    
    // Calculate how aligned ball velocity is with goal direction
    float goalAlignment = ballToGoal.Dot(ballVelNormalized);
    
    // Convert alignment to multiplier
    float multiplier;
    
    if (goalAlignment > 0.0f) {
        // Moving towards goal: 1.0x to (1.0 + goal_direction_strength)x reward
        // goalAlignment ranges from 0 to 1, so this gives 1.0x to 3.0x by default
        multiplier = 1.0f + (goalAlignment * goal_direction_strength);
    } else {
        // Moving away from goal: wall_penalty_strength to 1.0x reward
        // goalAlignment ranges from -1 to 0, so this gives 0.2x to 1.0x by default
        multiplier = wall_penalty_strength + (goalAlignment + 1.0f) * (1.0f - wall_penalty_strength);
    }
    
    // Additional distance-based scaling - closer to goal = stronger effect
    float distToGoal = (targetGoal - state.ball.pos).Length();
    float distanceFactor = 1.0f - std::min(1.0f, distToGoal / 8000.0f); // Stronger effect when closer to goal
    
    // Blend the multiplier effect based on distance to goal
    float finalMultiplier = 1.0f + (multiplier - 1.0f) * (0.5f + distanceFactor * 0.5f);
    
    return std::max(wall_penalty_strength, finalMultiplier);
}

} // namespace RLGSC