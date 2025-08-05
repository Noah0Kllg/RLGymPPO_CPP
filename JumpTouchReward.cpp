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
            
            // Mark that we just touched the ball to prevent air roll farming
            this->ball_touched_recently = this->air_roll_cooldown_steps;
        }
        
        // 2. AIR ROLL REWARD
        if (is_airborne) {
            float current_roll_input = prevAction.roll;
            
            if (std::abs(current_roll_input) > 0.1f && std::abs(this->last_roll_input) > 0.1f) {
                if ((current_roll_input * this->last_roll_input) < 0) this->direction_changes++;
            }
            if (this->direction_changes > 0) this->direction_changes = std::max(0, this->direction_changes - 1);
            
            this->cumulative_roll_direction = (this->cumulative_roll_direction + current_roll_input) * 0.98f;
            
            bool is_wiggling = this->direction_changes > MAX_DIRECTION_CHANGES;
            
            // Prevent air roll farming after ball touches
            bool is_farming_prevention_active = this->ball_touched_recently > 0;
            
            if (!is_wiggling && std::abs(current_roll_input) > 0.1f && !is_farming_prevention_active) {
                float current_angular_vel = player.phys.angVel.y;
                float roll_angular_vel = std::abs(current_angular_vel);
                float roll_intensity = std::min(1.0f, roll_angular_vel / CommonValues::CAR_MAX_ANG_VEL);
                
                air_roll_reward = roll_intensity * this->air_roll_weight;
                
                if (current_roll_input > 0.1f) {
                    air_roll_reward *= 1.5f;
                    if (this->cumulative_roll_direction > 2.0f) air_roll_reward *= 1.3f;
                } else {
                    air_roll_reward *= 0.3f;
                }
                
                if (prevAction.boost && player.boostFraction > 0.2f) air_roll_reward *= 1.4f;
                
                // Enhanced interception bonus - only when approaching ball
                if (dist < 1000.0f && ball_pos.z > 250.0f) {
                    float speed_towards_ball = 0.f;
                    if (dist > 1e-6f) speed_towards_ball = player.phys.vel.Dot(dist_vec / dist);
                    
                    if (speed_towards_ball > 100.0f) {
                        // Scale bonus based on distance - closer = higher bonus
                        float distance_factor = std::max(0.5f, (1000.0f - dist) / 1000.0f);
                        air_roll_reward *= (1.5f + distance_factor * 1.0f); // 1.5x to 2.5x multiplier
                        
                        // Bonus for roll intensity to encourage committed rolls
                        air_roll_reward *= (1.0f + roll_intensity * 0.5f);
                        
                        // Extra bonus for being very close to ball while air rolling
                        if (dist < 400.0f) {
                            air_roll_reward *= 1.3f;
                        }
                    }
                }
            }
            
            this->last_roll_input = current_roll_input;
        } else {
            this->last_roll_input = 0.0f;
            this->direction_changes = 0;
            this->cumulative_roll_direction = 0.0f;
        }
        
        // Decrease the cooldown counter
        if (this->ball_touched_recently > 0) {
            this->ball_touched_recently--;
        }
        
        reward += air_roll_reward;
        return reward;
    }
} // namespace RLGSC