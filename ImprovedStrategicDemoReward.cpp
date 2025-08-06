#include "ImprovedStrategicDemoReward.h"
#include <iostream>

namespace RLGSC {

    ImprovedStrategicDemoReward::ImprovedStrategicDemoReward(
        float baseDemoReward,
        float bumpMultiplier,
        float approachRewardWeight,
        float strategicMultiplier,
        float minApproachSpeed,
        float maxApproachDistance,
        float cooldownTime,
        bool enableBumpRewards
    ) : 
        baseDemoReward(baseDemoReward),
        bumpMultiplier(bumpMultiplier),
        approachRewardWeight(approachRewardWeight),
        strategicMultiplier(strategicMultiplier),
        minApproachSpeed(minApproachSpeed),
        maxApproachDistance(maxApproachDistance),
        cooldownTime(cooldownTime),
        enableBumpRewards(enableBumpRewards) {
    }

    void ImprovedStrategicDemoReward::Reset(const GameState& initialState) {
        playerData.clear();
        
        // Initialize player data
        for (const auto& player : initialState.players) {
            PlayerDemoData newData;
            newData.gameTime = 0.0f;
            
            // Initialize opponent tracking
            for (const auto& opponent : initialState.players) {
                if (opponent.team != player.team) {
                    newData.lastOpponentPos[opponent.carId] = opponent.phys.pos;
                    newData.lastOpponentSpeed[opponent.carId] = opponent.phys.vel.Length();
                    newData.lastDemoTime[opponent.carId] = -cooldownTime; // Allow immediate targeting
                    newData.lastBumpTime[opponent.carId] = -cooldownTime;
                }
            }
            
            playerData[player.carId] = newData;
        }
    }

    float ImprovedStrategicDemoReward::GetReward(const PlayerData& player, const GameState& state, const Action& prevAction) {
        // Get or initialize player data
        if (playerData.find(player.carId) == playerData.end()) {
            PlayerDemoData newData;
            newData.gameTime = 0.0f;
            playerData[player.carId] = newData;
        }
        
        auto& data = playerData[player.carId];
        data.gameTime += 1.0f / 120.0f; // Assuming 120 FPS
        
        float totalReward = 0.0f;

        // Process each opponent
        for (const auto& opponent : state.players) {
            // Skip teammates and self
            if (opponent.team == player.team || opponent.carId == player.carId) {
                continue;
            }
            
            // Skip if opponent is demoed
            if (opponent.carState.isDemoed) {
                continue;
            }
            
            // Initialize opponent data if needed
            if (data.lastOpponentPos.find(opponent.carId) == data.lastOpponentPos.end()) {
                data.lastOpponentPos[opponent.carId] = opponent.phys.pos;
                data.lastOpponentSpeed[opponent.carId] = opponent.phys.vel.Length();
                data.lastDemoTime[opponent.carId] = -cooldownTime;
                data.lastBumpTime[opponent.carId] = -cooldownTime;
            }

            // 1. CHECK FOR DEMO
            if (DetectDemo(player, opponent, data)) {
                float demoReward = baseDemoReward;
                float strategicValue = CalculateStrategicValue(player, opponent, state);
                demoReward *= strategicValue;
                
                totalReward += demoReward;
                data.lastDemoTime[opponent.carId] = data.gameTime;
                
                // Debug output
                std::cout << "[ImprovedStrategicDemo] Player " << player.carId 
                         << " DEMO! Reward: " << demoReward << " (strategic: " << strategicValue << ")" << std::endl;
            }
            
            // 2. CHECK FOR BUMP (if enabled and no recent demo)
            else if (enableBumpRewards && DetectBump(player, opponent, data)) {
                float bumpReward = baseDemoReward * bumpMultiplier;
                float strategicValue = CalculateStrategicValue(player, opponent, state);
                bumpReward *= strategicValue;
                
                totalReward += bumpReward;
                data.lastBumpTime[opponent.carId] = data.gameTime;
                
                // Debug output
                std::cout << "[ImprovedStrategicDemo] Player " << player.carId 
                         << " BUMP! Reward: " << bumpReward << " (strategic: " << strategicValue << ")" << std::endl;
            }
            
            // 3. APPROACH REWARD (continuous small rewards for approaching)
            else if (IsValidTarget(player, opponent, state, data)) {
                float approachReward = CalculateApproachReward(player, opponent, data);
                if (approachReward > 0.0f) {
                    totalReward += approachReward;
                }
            }
            
            // Update opponent tracking
            data.lastOpponentPos[opponent.carId] = opponent.phys.pos;
            data.lastOpponentSpeed[opponent.carId] = opponent.phys.vel.Length();
        }

        return totalReward;
    }

    bool ImprovedStrategicDemoReward::DetectDemo(const PlayerData& player, const PlayerData& opponent, PlayerDemoData& data) const {
        // Must be supersonic to demo
        if (!player.carState.isSupersonic) {
            return false;
        }
        
        // Check if opponent's speed dropped dramatically (indicating demo)
        float currentOpponentSpeed = opponent.phys.vel.Length();
        float lastOpponentSpeed = data.lastOpponentSpeed[opponent.carId];
        
        // Demo detection: opponent speed drops to near zero AND player was close
        bool speedDrop = (lastOpponentSpeed > 200.0f && currentOpponentSpeed < 50.0f);
        
        // Check if player was close to opponent
        float distToOpponent = (player.phys.pos - opponent.phys.pos).Length();
        bool wasClose = distToOpponent < 300.0f;
        
        // Check velocity towards opponent
        float velocityTowards = CalculateVelocityTowardsOpponent(player, opponent);
        bool approachingFast = velocityTowards > 800.0f;
        
        return speedDrop && wasClose && approachingFast;
    }

    bool ImprovedStrategicDemoReward::DetectBump(const PlayerData& player, const PlayerData& opponent, PlayerDemoData& data) const {
        // Don't need to be supersonic for bumps, but need good speed
        float playerSpeed = player.phys.vel.Length();
        if (playerSpeed < 600.0f) {
            return false;
        }
        
        // Check if opponent's velocity changed significantly (indicating bump)
        Vec currentOpponentVel = opponent.phys.vel;
        float currentOpponentSpeed = currentOpponentVel.Length();
        float lastOpponentSpeed = data.lastOpponentSpeed[opponent.carId];
        
        // Bump detection: opponent speed/direction changed significantly
        bool significantChange = std::abs(currentOpponentSpeed - lastOpponentSpeed) > 300.0f;
        
        // Check if player was close to opponent
        float distToOpponent = (player.phys.pos - opponent.phys.pos).Length();
        bool wasClose = distToOpponent < 200.0f;
        
        // Check velocity towards opponent
        float velocityTowards = CalculateVelocityTowardsOpponent(player, opponent);
        bool approachingFast = velocityTowards > 400.0f;
        
        // Cooldown check for bumps
        float timeSinceLastBump = data.gameTime - data.lastBumpTime[opponent.carId];
        bool notOnCooldown = timeSinceLastBump > (cooldownTime * 0.5f); // Shorter cooldown for bumps
        
        return significantChange && wasClose && approachingFast && notOnCooldown;
    }

    bool ImprovedStrategicDemoReward::IsValidTarget(const PlayerData& player, const PlayerData& opponent, const GameState& state, PlayerDemoData& data) const {
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
        
        // Check cooldown
        float timeSinceLastDemo = data.gameTime - data.lastDemoTime[opponent.carId];
        if (timeSinceLastDemo < cooldownTime) {
            return false;
        }
        
        // Check if moving towards opponent
        float velocityTowards = CalculateVelocityTowardsOpponent(player, opponent);
        if (velocityTowards < 500.0f) {
            return false;
        }
        
        return true;
    }

    float ImprovedStrategicDemoReward::CalculateApproachReward(const PlayerData& player, const PlayerData& opponent, PlayerDemoData& data) const {
        float distToOpponent = (player.phys.pos - opponent.phys.pos).Length();
        float velocityTowards = CalculateVelocityTowardsOpponent(player, opponent);
        
        // Distance factor (closer = higher reward)
        float distanceFactor = std::max(0.0f, (maxApproachDistance - distToOpponent) / maxApproachDistance);
        
        // Velocity factor (faster approach = higher reward)
        float velocityFactor = std::min(1.0f, velocityTowards / 1500.0f);
        
        // Strategic value
        float strategicValue = CalculateStrategicValue(player, opponent, state);
        
        // Supersonic bonus
        float supersonicBonus = player.carState.isSupersonic ? 1.5f : 1.0f;
        
        // Base approach reward
        float baseReward = approachRewardWeight * distanceFactor * velocityFactor * strategicValue * supersonicBonus;
        
        return baseReward;
    }

    float ImprovedStrategicDemoReward::CalculateStrategicValue(const PlayerData& player, const PlayerData& opponent, const GameState& state) const {
        float value = 1.0f; // Base value
        
        // 1. Ball proximity bonus - demo opponent when they're close to ball
        float opponentDistToBall = (opponent.phys.pos - state.ball.pos).Length();
        if (opponentDistToBall < 500.0f) {
            value *= 1.5f; // Higher value for demoing ball-carriers
        }
        
        // 2. Opponent goal proximity - demo when opponent is in scoring position
        Vec opponentGoal = opponent.team == Team::BLUE ? 
                          CommonValues::BLUE_GOAL_BACK : 
                          CommonValues::ORANGE_GOAL_BACK;
        float opponentDistToTheirGoal = (opponent.phys.pos - opponentGoal).Length();
        
        // Higher value for demoing opponent in their defensive third
        if (opponentDistToTheirGoal < 3000.0f) {
            value *= 1.3f;
        }
        
        // 3. Your goal proximity - lower value for risky demos near your goal
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
        
        // 5. Ball possession bonus - extra value if opponent has ball control
        if (opponentDistToBall < 200.0f) {
            Vec opponentToBall = (state.ball.pos - opponent.phys.pos).Normalized();
            Vec opponentForward = opponent.phys.rotMat.forward;
            float facingBall = opponentToBall.Dot(opponentForward);
            
            if (facingBall > 0.7f) {
                value *= 1.4f; // High value for demoing opponent with ball control
            }
        }
        
        return value;
    }

    float ImprovedStrategicDemoReward::CalculateVelocityTowardsOpponent(const PlayerData& player, const PlayerData& opponent) const {
        Vec dirToOpponent = (opponent.phys.pos - player.phys.pos);
        float distToOpponent = dirToOpponent.Length();
        
        if (distToOpponent < 1e-6f) {
            return 0.0f;
        }
        
        dirToOpponent = dirToOpponent / distToOpponent;
        return std::max(0.0f, player.phys.vel.Dot(dirToOpponent));
    }

} // namespace RLGSC