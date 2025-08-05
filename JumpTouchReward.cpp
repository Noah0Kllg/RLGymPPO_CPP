#include "JumpTouchReward.h"
#include <iostream>

namespace RLGSC {

    float JumpTouchReward::GetReward(const PlayerData& player, const GameState& state, const Action& prevAction) {
        float reward = 0.0f;
        float air_roll_reward = 0.0f;
        float touch_reward = 0.0f;
        
        const Vec& car_pos = player.phys.pos;
        const Vec& ball_pos = state.ball.pos;
        bool is_airborne = !player.carState.isOnGround && car_pos.z >= this->air_roll_min_height;
        
        Vec dist_vec = ball_pos - car_pos;
        float dist = dist_vec.Length();
        
        // 1. JUMP TOUCH REWARD
        if (player.ballTouchedStep && is_airborne && ball_pos.z >= minHeight) {
            float heightAboveMin = ball_pos.z - minHeight;
            float height_factor = std::clamp(heightAboveMin / range, 0.0f, 1.0f);
            touch_reward = height_factor * 1.0f;
            
            float ball_speed = state.ball.vel.Length();
            if (ball_speed > 300.0f) {
                float speed_factor = std::min(1.0f, ball_speed / 2000.0f);
                touch_reward *= (1.0f + speed_factor);
            }
            
            float direction_factor = 0.0f;
            if (player.team == Team::BLUE && state.ball.vel.y > 0) {
                direction_factor = std::min(1.0f, state.ball.vel.y / 1000.0f);
            } else if (player.team == Team::ORANGE && state.ball.vel.y < 0) {
                direction_factor = std::min(1.0f, -state.ball.vel.y / 1000.0f);
            }
            touch_reward *= (1.0f + direction_factor * 0.5f);
            
            reward += touch_reward;
        }
        
        // 2. AIR ROLL REWARD - Distance-based continuous reward
        if (is_airborne) {
            float current_roll_input = prevAction.roll;
            
            if (std::abs(current_roll_input) > 0.1f && std::abs(this->last_roll_input) > 0.1f) {
                if ((current_roll_input * this->last_roll_input) < 0) this->direction_changes++;
            }
            if (this->direction_changes > 0) this->direction_changes = std::max(0, this->direction_changes - 1);
            
            this->cumulative_roll_direction = (this->cumulative_roll_direction + current_roll_input) * 0.98f;
            
            bool is_wiggling = this->direction_changes > MAX_DIRECTION_CHANGES;
            
            if (!is_wiggling && std::abs(current_roll_input) > 0.1f) {
                float current_angular_vel = player.phys.angVel.y;
                float roll_angular_vel = std::abs(current_angular_vel);
                float roll_intensity = std::min(1.0f, roll_angular_vel / CommonValues::CAR_MAX_ANG_VEL);
                
                // Base air roll reward
                air_roll_reward = roll_intensity * this->air_roll_weight;
                
                // Directional preference
                if (current_roll_input > 0.1f) {
                    air_roll_reward *= 1.5f;
                    if (this->cumulative_roll_direction > 2.0f) air_roll_reward *= 1.3f;
                } else {
                    air_roll_reward *= 0.3f;
                }
                
                // Boost bonus
                if (prevAction.boost && player.boostFraction > 0.2f) air_roll_reward *= 1.4f;
                
                // DISTANCE-BASED BONUS: Reward air rolling when close to the ball
                if (ball_pos.z > 200.0f) { // Only when ball is in the air
                    float max_reward_distance = 1200.0f; // Start giving rewards from this distance
                    
                    if (dist < max_reward_distance) {
                        // Scale reward inversely with distance - closer = higher reward
                        float distance_factor = (max_reward_distance - dist) / max_reward_distance;
                        distance_factor = std::max(0.0f, distance_factor); // Ensure positive
                        
                        // Apply distance bonus - scales from 0x to 2x multiplier
                        float distance_multiplier = 1.0f + (distance_factor * 2.0f);
                        air_roll_reward *= distance_multiplier;
                        
                        // Extra bonus for being very close
                        if (dist < 400.0f) {
                            air_roll_reward *= 1.5f;
                        }
                        
                        // Bonus for moving towards the ball while air rolling
                        float speed_towards_ball = 0.f;
                        if (dist > 1e-6f) speed_towards_ball = player.phys.vel.Dot(dist_vec / dist);
                        
                        if (speed_towards_ball > 50.0f) {
                            float approach_factor = std::min(1.0f, speed_towards_ball / 1000.0f);
                            air_roll_reward *= (1.0f + approach_factor * 0.5f);
                        }
                    }
                }
                
                // Bonus for touching ball while air rolling
                if (player.ballTouchedStep) {
                    air_roll_reward *= 1.5f;
                }
            }
            
            this->last_roll_input = current_roll_input;
        } else {
            this->last_roll_input = 0.0f;
            this->direction_changes = 0;
            this->cumulative_roll_direction = 0.0f;
        }
        
        reward += air_roll_reward;
        return reward;
    }
} // namespace RLGSC