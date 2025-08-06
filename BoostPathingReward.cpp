#include "BoostPathingReward.h"
#include <iostream>

namespace RLGSC {

    BoostPathingReward::BoostPathingReward(
        float maxTargetTime,
        float minBoostThreshold,
        float maxApproachDistance,
        float approachRewardWeight,
        float collectionRewardWeight,
        float necessityWeight,
        bool enableStrategicPositioning
    ) : 
        maxTargetTime(maxTargetTime),
        minBoostThreshold(minBoostThreshold),
        maxApproachDistance(maxApproachDistance),
        approachRewardWeight(approachRewardWeight),
        collectionRewardWeight(collectionRewardWeight),
        necessityWeight(necessityWeight),
        enableStrategicPositioning(enableStrategicPositioning) {
        
        InitializeBoostPads();
    }

    void BoostPathingReward::InitializeBoostPads() {
        boostPads.clear();
        
        // Big boost pads (100 boost, z=73)
        boostPads.emplace_back(-3072.0f, -4096.0f, 73.0f, true);  // (3)
        boostPads.emplace_back( 3072.0f, -4096.0f, 73.0f, true);  // (4)
        boostPads.emplace_back(-3584.0f,     0.0f, 73.0f, true);  // (15)
        boostPads.emplace_back( 3584.0f,     0.0f, 73.0f, true);  // (18)
        boostPads.emplace_back(-3072.0f,  4096.0f, 73.0f, true);  // (29)
        boostPads.emplace_back( 3072.0f,  4096.0f, 73.0f, true);  // (30)
        
        // Small boost pads (12 boost, z=70)
        boostPads.emplace_back(    0.0f, -4240.0f, 70.0f, false); // (0)
        boostPads.emplace_back(-1792.0f, -4184.0f, 70.0f, false); // (1)
        boostPads.emplace_back( 1792.0f, -4184.0f, 70.0f, false); // (2)
        boostPads.emplace_back(- 940.0f, -3308.0f, 70.0f, false); // (5)
        boostPads.emplace_back(  940.0f, -3308.0f, 70.0f, false); // (6)
        boostPads.emplace_back(    0.0f, -2816.0f, 70.0f, false); // (7)
        boostPads.emplace_back(-3584.0f, -2484.0f, 70.0f, false); // (8)
        boostPads.emplace_back( 3584.0f, -2484.0f, 70.0f, false); // (9)
        boostPads.emplace_back(-1788.0f, -2300.0f, 70.0f, false); // (10)
        boostPads.emplace_back( 1788.0f, -2300.0f, 70.0f, false); // (11)
        boostPads.emplace_back(-2048.0f, -1036.0f, 70.0f, false); // (12)
        boostPads.emplace_back(    0.0f, -1024.0f, 70.0f, false); // (13)
        boostPads.emplace_back( 2048.0f, -1036.0f, 70.0f, false); // (14)
        boostPads.emplace_back(-1024.0f,     0.0f, 70.0f, false); // (16)
        boostPads.emplace_back( 1024.0f,     0.0f, 70.0f, false); // (17)
        boostPads.emplace_back(-2048.0f,  1036.0f, 70.0f, false); // (19)
        boostPads.emplace_back(    0.0f,  1024.0f, 70.0f, false); // (20)
        boostPads.emplace_back( 2048.0f,  1036.0f, 70.0f, false); // (21)
        boostPads.emplace_back(-1788.0f,  2300.0f, 70.0f, false); // (22)
        boostPads.emplace_back( 1788.0f,  2300.0f, 70.0f, false); // (23)
        boostPads.emplace_back(-3584.0f,  2484.0f, 70.0f, false); // (24)
        boostPads.emplace_back( 3584.0f,  2484.0f, 70.0f, false); // (25)
        boostPads.emplace_back(    0.0f,  2816.0f, 70.0f, false); // (26)
        boostPads.emplace_back(- 940.0f,  3308.0f, 70.0f, false); // (27)
        boostPads.emplace_back(  940.0f,  3308.0f, 70.0f, false); // (28)
        boostPads.emplace_back(-1792.0f,  4184.0f, 70.0f, false); // (31)
        boostPads.emplace_back( 1792.0f,  4184.0f, 70.0f, false); // (32)
        boostPads.emplace_back(    0.0f,  4240.0f, 70.0f, false); // (33)
    }

    void BoostPathingReward::Reset(const GameState& initialState) {
        playerData.clear();
        
        // Initialize player data
        for (const auto& player : initialState.players) {
            playerData[player.carId] = PlayerBoostData{};
            playerData[player.carId].lastBoostAmount = player.boostFraction;
        }
    }

    float BoostPathingReward::GetReward(const PlayerData& player, const GameState& state, const Action& prevAction) {
        // Get or initialize player data
        if (playerData.find(player.carId) == playerData.end()) {
            playerData[player.carId] = PlayerBoostData{};
            playerData[player.carId].lastBoostAmount = player.boostFraction;
        }
        
        auto& data = playerData[player.carId];
        float reward = 0.0f;
        
        // Calculate boost necessity (how much the player needs boost)
        float boostNecessity = CalculateBoostNecessity(player, state);
        
        // 1. COLLECTION REWARD - Check if player collected boost
        float collectionReward = CalculateCollectionReward(player, state, data);
        if (collectionReward > 0.0f) {
            reward += collectionReward * collectionRewardWeight;
            
            // Reset targeting when boost is collected
            data.targetBoostId = -1;
            data.timeTargeting = 0.0f;
            data.timeSinceLastCollection = 0.0f;
        }
        
        // 2. APPROACH REWARD - Only if boost is needed and not recently collected
        if (boostNecessity > 0.1f && data.timeSinceLastCollection > 1.0f) {
            float approachReward = CalculateApproachReward(player, state, data);
            reward += approachReward * approachRewardWeight * boostNecessity * necessityWeight;
        }
        
        // Update player tracking
        UpdatePlayerTracking(player, state, data);
        
        return reward;
    }

    void BoostPathingReward::UpdatePlayerTracking(const PlayerData& player, const GameState& state, PlayerBoostData& data) {
        // Update time counters
        data.timeTargeting += 1.0f / 120.0f; // Assuming 120 FPS
        data.timeSinceLastCollection += 1.0f / 120.0f;
        
        // Reset target if it's been too long without collection
        if (data.timeTargeting > maxTargetTime) {
            data.targetBoostId = -1;
            data.timeTargeting = 0.0f;
        }
        
        // Reset target if boost is no longer available
        if (data.targetBoostId >= 0 && !IsBoostAvailable(state, data.targetBoostId)) {
            data.targetBoostId = -1;
            data.timeTargeting = 0.0f;
        }
        
        // Update last boost amount
        data.lastBoostAmount = player.boostFraction;
    }

    float BoostPathingReward::CalculateBoostNecessity(const PlayerData& player, const GameState& state) const {
        float boostLevel = player.boostFraction;
        
        // Base necessity based on current boost level
        float necessity = 0.0f;
        if (boostLevel < minBoostThreshold) {
            necessity = (minBoostThreshold - boostLevel) / minBoostThreshold;
        }
        
        // Increase necessity based on game context
        float distToBall = (player.phys.pos - state.ball.pos).Length();
        bool isAirborne = !player.carState.isOnGround;
        float playerSpeed = player.phys.vel.Length();
        
        // Higher necessity if:
        // - Far from ball and need to get there quickly
        if (distToBall > 2000.0f && playerSpeed < 1200.0f) {
            necessity += 0.3f;
        }
        
        // - Airborne with low boost (dangerous situation)
        if (isAirborne && boostLevel < 0.2f) {
            necessity += 0.5f;
        }
        
        // - In defensive situation (ball close to own goal)
        Vec ownGoal = player.team == Team::BLUE ? 
                     CommonValues::BLUE_GOAL_BACK : 
                     CommonValues::ORANGE_GOAL_BACK;
        float ballDistToOwnGoal = (state.ball.pos - ownGoal).Length();
        if (ballDistToOwnGoal < 3000.0f && boostLevel < 0.4f) {
            necessity += 0.4f;
        }
        
        return std::min(1.0f, necessity);
    }

    float BoostPathingReward::CalculateCollectionReward(const PlayerData& player, const GameState& state, PlayerBoostData& data) const {
        // Check if boost increased significantly (indicating collection)
        float boostIncrease = player.boostFraction - data.lastBoostAmount;
        
        if (boostIncrease > 0.1f) { // Big boost collected
            return 1.0f;
        } else if (boostIncrease > 0.05f) { // Small boost collected
            return 0.12f;
        }
        
        return 0.0f;
    }

    float BoostPathingReward::CalculateApproachReward(const PlayerData& player, const GameState& state, PlayerBoostData& data) const {
        // Find or update target boost
        if (data.targetBoostId == -1) {
            // Need big boost if very low
            bool needsBigBoost = player.boostFraction < 0.15f;
            data.targetBoostId = FindNearestBoostPad(player.phys.pos, maxApproachDistance, needsBigBoost);
            
            if (data.targetBoostId == -1) {
                return 0.0f; // No suitable boost found
            }
            
            data.timeTargeting = 0.0f;
            data.lastDistanceToTarget = (player.phys.pos - boostPads[data.targetBoostId].position).Length();
        }
        
        // Check if target is still valid
        if (!IsBoostAvailable(state, data.targetBoostId) || 
            !ShouldTargetBoost(player, state, data.targetBoostId)) {
            data.targetBoostId = -1;
            return 0.0f;
        }
        
        const BoostPad& targetBoost = boostPads[data.targetBoostId];
        float currentDistance = (player.phys.pos - targetBoost.position).Length();
        
        // Check if moving towards boost
        bool movingTowards = currentDistance < data.lastDistanceToTarget;
        
        if (!movingTowards) {
            return 0.0f; // Not approaching, no reward
        }
        
        // Calculate approach reward based on distance and strategic value
        float distanceFactor = std::max(0.0f, (maxApproachDistance - currentDistance) / maxApproachDistance);
        float approachSpeed = (data.lastDistanceToTarget - currentDistance) * 120.0f; // Convert to units/second
        
        // Base approach reward
        float reward = distanceFactor * 0.1f;
        
        // Bonus for faster approach
        if (approachSpeed > 100.0f) {
            float speedFactor = std::min(1.0f, approachSpeed / 1000.0f);
            reward *= (1.0f + speedFactor);
        }
        
        // Strategic value multiplier
        if (enableStrategicPositioning) {
            float strategicValue = CalculateStrategicValue(player, state, data.targetBoostId);
            reward *= strategicValue;
        }
        
        // Update tracking
        data.lastDistanceToTarget = currentDistance;
        data.wasMovingTowardsTarget = movingTowards;
        
        return reward;
    }

    int BoostPathingReward::FindNearestBoostPad(const Vec& position, float maxDistance, bool needsBigBoost) const {
        int nearestId = -1;
        float nearestDistance = maxDistance;
        
        for (int i = 0; i < boostPads.size(); ++i) {
            const BoostPad& pad = boostPads[i];
            
            // Skip if we need big boost but this is small boost
            if (needsBigBoost && !pad.isBigBoost) {
                continue;
            }
            
            float distance = (position - pad.position).Length();
            if (distance < nearestDistance) {
                nearestDistance = distance;
                nearestId = i;
            }
        }
        
        return nearestId;
    }

    bool BoostPathingReward::ShouldTargetBoost(const PlayerData& player, const GameState& state, int boostId) const {
        if (boostId < 0 || boostId >= boostPads.size()) {
            return false;
        }
        
        const BoostPad& boost = boostPads[boostId];
        
        // Don't target if player has enough boost (unless it's a big boost and player is very low)
        if (player.boostFraction > 0.7f && !boost.isBigBoost) {
            return false;
        }
        
        // Don't target if boost is too far and there are closer options
        float distance = (player.phys.pos - boost.position).Length();
        if (distance > maxApproachDistance) {
            return false;
        }
        
        // Don't target if player is in immediate ball action (very close to ball)
        float distToBall = (player.phys.pos - state.ball.pos).Length();
        if (distToBall < 300.0f && boost.isBigBoost) {
            return false; // Don't abandon ball for big boost when already in possession
        }
        
        return true;
    }

    float BoostPathingReward::CalculateStrategicValue(const PlayerData& player, const GameState& state, int boostId) const {
        if (boostId < 0 || boostId >= boostPads.size()) {
            return 1.0f;
        }
        
        const BoostPad& boost = boostPads[boostId];
        float value = 1.0f;
        
        // Higher value for boost pads on your side of the field
        bool isPlayerSide = (player.team == Team::BLUE && boost.position.y < 0) ||
                           (player.team == Team::ORANGE && boost.position.y > 0);
        
        if (isPlayerSide) {
            value *= 1.2f; // Prefer boost on your side
        }
        
        // Higher value if boost is between player and ball (good positioning)
        Vec playerToBall = (state.ball.pos - player.phys.pos).Normalized();
        Vec playerToBoost = (boost.position - player.phys.pos).Normalized();
        float alignment = playerToBall.Dot(playerToBoost);
        
        if (alignment > 0.5f) {
            value *= 1.3f; // Good positioning bonus
        }
        
        // Lower value if boost is in dangerous area (near opponent goal when defending)
        Vec ownGoal = player.team == Team::BLUE ? 
                     CommonValues::BLUE_GOAL_BACK : 
                     CommonValues::ORANGE_GOAL_BACK;
        float ballDistToOwnGoal = (state.ball.pos - ownGoal).Length();
        float boostDistToOwnGoal = (boost.position - ownGoal).Length();
        
        if (ballDistToOwnGoal < 3000.0f && boostDistToOwnGoal > 6000.0f) {
            value *= 0.6f; // Dangerous to go for far boost when defending
        }
        
        return value;
    }

    bool BoostPathingReward::IsBoostAvailable(const GameState& state, int boostId) const {
        // This is a simplified check - in a real implementation, you'd need to check
        // the actual boost pad states from the game state. For now, assume all are available.
        // You would need to access state.boostPads or similar data structure.
        
        // Placeholder: always return true since boost availability tracking 
        // requires game state boost pad information that may not be easily accessible
        return true;
    }

} // namespace RLGSC