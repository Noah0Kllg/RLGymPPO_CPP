#include "SimplifiedStrategicDemoReward.h"
#include <iostream>

namespace RLGSC {

    SimplifiedStrategicDemoReward::SimplifiedStrategicDemoReward(
        float baseDemoReward,
        float bumpMultiplier,
        float approachRewardWeight,
        float strategicMultiplier,
        float minApproachSpeed,
        float maxApproachDistance,
        float cooldownTime
    ) : 
        baseDemoReward(baseDemoReward),
        bumpMultiplier(bumpMultiplier),
        approachRewardWeight(approachRewardWeight),
        strategicMultiplier(strategicMultiplier),
        minApproachSpeed(minApproachSpeed),
        maxApproachDistance(maxApproachDistance),
        cooldownTime(cooldownTime) {
    }

    void SimplifiedStrategicDemoReward::Reset(const GameState& initialState) {
        playerData.clear();
        
        // Initialize player data
        for (const auto& player : initialState.players) {
            PlayerTrackingData newData;
            newData.lastMatchDemos = player.matchDemos;
            newData.lastMatchBumps = player.matchBumps;
            newData.gameTime = 0.0f;
            
            playerData[player.carId] = newData;
        }
    }

    float SimplifiedStrategicDemoReward::GetReward(const PlayerData& player, const GameState& state, const Action& prevAction) {
        // Get or initialize player data
        if (playerData.find(player.carId) == playerData.end()) {
            PlayerTrackingData newData;
            newData.lastMatchDemos = player.matchDemos;
            newData.lastMatchBumps = player.matchBumps;
            newData.gameTime = 0.0f;
            playerData[player.carId] = newData;
        }
        
        auto& data = playerData[player.carId];
        data.gameTime += 1.0f / 120.0f; // Assuming 120 FPS
        
        float totalReward = 0.0f;

        // 1. CHECK FOR NEW DEMOS (using built-in counter)
        int newDemos = player.matchDemos - data.lastMatchDemos;
        if (newDemos > 0) {
            // Find which opponent was likely demoed for strategic value calculation
            float bestStrategicValue = 1.0f;
            for (const auto& opponent : state.players) {
                if (opponent.team != player.team && opponent.carState.isDemoed) {
                    float strategicValue = CalculateStrategicValue(player, opponent, state);
                    bestStrategicValue = std::max(bestStrategicValue, strategicValue);
                }
            }
            
            float demoReward = baseDemoReward * bestStrategicValue * newDemos;
            totalReward += demoReward;
            
            // Debug output
            std::cout << "[SimplifiedStrategicDemo] Player " << player.carId 
                     << " DEMO! Count: " << newDemos << " Reward: " << demoReward 
                     << " (strategic: " << bestStrategicValue << ")" << std::endl;
            
            data.lastMatchDemos = player.matchDemos;
        }

        // 2. CHECK FOR NEW BUMPS (using built-in counter)
        int newBumps = player.matchBumps - data.lastMatchBumps - newDemos; // Subtract demos since they count as bumps too
        if (newBumps > 0) {
            // Calculate strategic value for bumps (use average since we can't identify specific target)
            float averageStrategicValue = 1.0f;
            int opponentCount = 0;
            
            for (const auto& opponent : state.players) {
                if (opponent.team != player.team) {
                    averageStrategicValue += CalculateStrategicValue(player, opponent, state);
                    opponentCount++;
                }
            }
            
            if (opponentCount > 0) {
                averageStrategicValue /= opponentCount;
            }
            
            float bumpReward = baseDemoReward * bumpMultiplier * averageStrategicValue * newBumps;
            totalReward += bumpReward;
            
            // Debug output
            std::cout << "[SimplifiedStrategicDemo] Player " << player.carId 
                     << " BUMP! Count: " << newBumps << " Reward: " << bumpReward 
                     << " (strategic: " << averageStrategicValue << ")" << std::endl;
            
            data.lastMatchBumps = player.matchBumps;
        }

        // 3. APPROACH REWARDS (continuous small rewards for approaching opponents)
        for (const auto& opponent : state.players) {
            // Skip teammates, self, and demoed opponents
            if (opponent.team == player.team || opponent.carId == player.carId || opponent.carState.isDemoed) {
                continue;
            }
            
            if (IsValidApproachTarget(player, opponent, data)) {
                float approachReward = CalculateApproachReward(player, opponent, data);
                if (approachReward > 0.0f) {
                    totalReward += approachReward;
                    
                    // Update cooldown for this opponent
                    data.lastApproachRewardTime[opponent.carId] = data.gameTime;
                }
            }
        }

        return totalReward;
    }

    bool SimplifiedStrategicDemoReward::IsValidApproachTarget(const PlayerData& player, const PlayerData& opponent, PlayerTrackingData& data) const {
        // Check distance
        float distToOpponent = (player.phys.pos - opponent.phys.pos).Length();
        if (distToOpponent > maxApproachDistance) {
            return false;
        }
        
        // Check if player has sufficient speed
        float playerSpeed = player.phys.vel.Length();
        if (playerSpeed < minApproachSpeed) {
            return false;
        }
        
        // Check cooldown for this specific opponent
        if (data.lastApproachRewardTime.find(opponent.carId) != data.lastApproachRewardTime.end()) {
            float timeSinceLastReward = data.gameTime - data.lastApproachRewardTime[opponent.carId];
            if (timeSinceLastReward < cooldownTime) {
                return false;
            }
        }
        
        // Check if moving towards opponent
        float velocityTowards = CalculateVelocityTowardsOpponent(player, opponent);
        if (velocityTowards < 500.0f) {
            return false;
        }
        
        return true;
    }

    float SimplifiedStrategicDemoReward::CalculateApproachReward(const PlayerData& player, const PlayerData& opponent, PlayerTrackingData& data) const {
        float distToOpponent = (player.phys.pos - opponent.phys.pos).Length();
        float velocityTowards = CalculateVelocityTowardsOpponent(player, opponent);
        
        // Distance factor (closer = higher reward)
        float distanceFactor = std::max(0.0f, (maxApproachDistance - distToOpponent) / maxApproachDistance);
        
        // Velocity factor (faster approach = higher reward)
        float velocityFactor = std::min(1.0f, velocityTowards / 1500.0f);
        
        // Strategic value for this target
        float strategicValue = CalculateStrategicValue(player, opponent, state);
        
        // Supersonic bonus
        float supersonicBonus = player.carState.isSupersonic ? 1.5f : 1.0f;
        
        // Base approach reward
        float baseReward = approachRewardWeight * distanceFactor * velocityFactor * strategicValue * supersonicBonus;
        
        return baseReward;
    }

    float SimplifiedStrategicDemoReward::CalculateStrategicValue(const PlayerData& player, const PlayerData& opponent, const GameState& state) const {
        float value = 1.0f; // Base value
        
        // 1. Ball proximity bonus - demo opponent when they're close to ball
        float opponentDistToBall = (opponent.phys.pos - state.ball.pos).Length();
        if (opponentDistToBall < 500.0f) {
            value *= 1.5f; // Higher value for demoing ball-carriers
        }
        
        // 2. Opponent in scoring position
        Vec opponentGoal = opponent.team == Team::BLUE ? 
                          CommonValues::BLUE_GOAL_BACK : 
                          CommonValues::ORANGE_GOAL_BACK;
        float opponentDistToTheirGoal = (opponent.phys.pos - opponentGoal).Length();
        
        // Higher value for demoing opponent in their defensive third
        if (opponentDistToTheirGoal < 3000.0f) {
            value *= 1.3f;
        }
        
        // 3. Risk assessment - lower value for risky demos near your goal
        Vec yourGoal = player.team == Team::BLUE ? 
                      CommonValues::BLUE_GOAL_BACK : 
                      CommonValues::ORANGE_GOAL_BACK;
        float yourDistToYourGoal = (player.phys.pos - yourGoal).Length();
        
        // Reduce value for demos very close to your own goal (risky)
        if (yourDistToYourGoal < 2000.0f) {
            value *= 0.7f;
        }
        
        // 4. Opponent speed bonus - higher value for demoing fast opponents
        float opponentSpeed = opponent.phys.vel.Length();
        if (opponentSpeed > 1000.0f) {
            value *= 1.2f; // Bonus for stopping fast opponents
        }
        
        // 5. Ball control bonus - extra value if opponent has ball control
        if (opponentDistToBall < 200.0f) {
            Vec opponentToBall = (state.ball.pos - opponent.phys.pos).Normalized();
            Vec opponentForward = opponent.phys.rotMat.forward;
            float facingBall = opponentToBall.Dot(opponentForward);
            
            if (facingBall > 0.7f) {
                value *= 1.4f; // High value for demoing opponent with ball control
            }
        }
        
        // 6. 1v1 specific bonuses (since you mentioned 1v1 focus)
        // In 1v1, demos are high risk/high reward
        Vec ballToYourGoal = (yourGoal - state.ball.pos);
        float ballDistToYourGoal = ballToYourGoal.Length();
        
        if (ballDistToYourGoal > 4000.0f) {
            // Ball far from your goal = safer to demo
            value *= 1.2f;
        } else if (ballDistToYourGoal < 2000.0f) {
            // Ball close to your goal = very risky to demo
            value *= 0.5f;
        }
        
        return value;
    }

    float SimplifiedStrategicDemoReward::CalculateVelocityTowardsOpponent(const PlayerData& player, const PlayerData& opponent) const {
        Vec dirToOpponent = (opponent.phys.pos - player.phys.pos);
        float distToOpponent = dirToOpponent.Length();
        
        if (distToOpponent < 1e-6f) {
            return 0.0f;
        }
        
        dirToOpponent = dirToOpponent / distToOpponent;
        return std::max(0.0f, player.phys.vel.Dot(dirToOpponent));
    }

} // namespace RLGSC