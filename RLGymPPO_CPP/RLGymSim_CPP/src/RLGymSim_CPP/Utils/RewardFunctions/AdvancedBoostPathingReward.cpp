#include "AdvancedBoostPathingReward.h"
#include <algorithm>
#include <cmath>

namespace RLGSC {

	AdvancedBoostPathingReward::AdvancedBoostPathingReward(const Config& config) 
		: config(config), currentTime(0.0f) {
		InitializeBoostPadStates();
	}

	void AdvancedBoostPathingReward::InitializeBoostPadStates() {
		boostPadStates.clear();
		boostPadStates.reserve(CommonValues::BOOST_LOCATIONS_AMOUNT);

		for (int i = 0; i < CommonValues::BOOST_LOCATIONS_AMOUNT; i++) {
			BoostPadState padState;
			padState.position = CommonValues::BOOST_LOCATIONS[i];
			padState.isLargePad = IsLargePad(padState.position);
			padState.cooldownTimer = 0.0f;
			padState.isActive = true;
			padState.lastCollectionTime = 0.0f;
			padState.collectionCount = 0;
			boostPadStates.push_back(padState);
		}
	}

	bool AdvancedBoostPathingReward::IsLargePad(const Vec& position) const {
		// Large boost pads are at corners and center
		if (std::abs(position.x) < 50 && std::abs(position.y) < 50) {
			return true; // Center pad
		}
		if (position.z > 70.5f) {
			return true; // Corner pads (higher Z)
		}
		if (std::abs(position.y) > 4000) {
			return true; // Back wall pads
		}
		return false;
	}

	bool AdvancedBoostPathingReward::IsBackWallPad(const Vec& position) const {
		return std::abs(position.y) > 4000;
	}

	bool AdvancedBoostPathingReward::IsCenterPad(const Vec& position) const {
		return std::abs(position.x) < 200 && std::abs(position.y) < 200;
	}

	void AdvancedBoostPathingReward::UpdateBoostPadStates(const GameState& state) {
		for (int i = 0; i < CommonValues::BOOST_LOCATIONS_AMOUNT; i++) {
			auto& padState = boostPadStates[i];
			
			// Update active state
			bool wasActive = padState.isActive;
			padState.isActive = state.boostPads[i];
			
			// If pad just became active, update timing info
			if (padState.isActive && !wasActive) {
				padState.lastCollectionTime = currentTime;
			}
			
			// Update cooldown timer
			padState.cooldownTimer = state.boostPadTimers[i];
		}
	}

	void AdvancedBoostPathingReward::Reset(const GameState& initialState) {
		playerHistories.clear();
		currentTime = 0.0f;
		
		// Initialize player histories
		for (const auto& player : initialState.players) {
			playerHistories[player.carId] = std::vector<PlayerBoostHistory>();
		}
		
		// Reset boost pad states
		InitializeBoostPadStates();
	}

	void AdvancedBoostPathingReward::PreStep(const GameState& state) {
		currentTime += state.deltaTime;
		UpdateBoostPadStates(state);
		
		// Update player histories
		for (const auto& player : state.players) {
			UpdatePlayerHistory(player, state);
		}
	}

	void AdvancedBoostPathingReward::UpdatePlayerHistory(const PlayerData& player, const GameState& state) {
		auto& history = playerHistories[player.carId];
		
		PlayerBoostHistory entry;
		entry.position = player.phys.pos;
		entry.boostFraction = player.boostFraction;
		entry.timestamp = currentTime;
		entry.nearbyPads.resize(CommonValues::BOOST_LOCATIONS_AMOUNT, false);
		
		// Calculate which pads are nearby
		for (int i = 0; i < CommonValues::BOOST_LOCATIONS_AMOUNT; i++) {
			if (state.boostPads[i]) {
				entry.nearbyPads[i] = IsPadNearby(player.phys.pos, boostPadStates[i].position);
			}
		}
		
		history.push_back(entry);
		
		// Keep only recent history (last 2 seconds)
		while (history.size() > 1 && (currentTime - history[0].timestamp) > 2.0f) {
			history.erase(history.begin());
		}
	}

	float AdvancedBoostPathingReward::GetReward(const PlayerData& player, const GameState& state, const Action& prevAction) {
		float totalReward = 0.0f;

		// Calculate all reward components
		totalReward += CalculateProximityReward(player, state);
		totalReward += CalculateCollectionReward(player, state);
		totalReward += CalculatePathingReward(player, state);
		totalReward += CalculateTimingReward(player, state);
		totalReward += CalculateEfficiencyReward(player, state);
		totalReward += CalculateStrategicReward(player, state);

		return totalReward;
	}

	float AdvancedBoostPathingReward::CalculateProximityReward(const PlayerData& player, const GameState& state) {
		float reward = 0.0f;
		auto distances = CalculatePadDistances(player.phys.pos, state);

		for (int i = 0; i < CommonValues::BOOST_LOCATIONS_AMOUNT; i++) {
			if (state.boostPads[i] && distances[i] <= config.proximityThreshold) {
				float proximityFactor = 1.0f - (distances[i] / config.proximityThreshold);
				proximityFactor = std::max(0.0f, std::min(1.0f, proximityFactor));
				
				float padMultiplier = GetPadPreferenceMultiplier(boostPadStates[i].position);
				reward += config.proximityReward * proximityFactor * padMultiplier;
			}
		}

		return reward;
	}

	float AdvancedBoostPathingReward::CalculateCollectionReward(const PlayerData& player, const GameState& state) {
		float reward = 0.0f;
		auto& history = playerHistories[player.carId];
		
		if (history.size() < 2) return 0.0f;
		
		// Check if player collected any boost pads
		for (int i = 0; i < CommonValues::BOOST_LOCATIONS_AMOUNT; i++) {
			if (state.boostPads[i]) {
				Vec padPos = boostPadStates[i].position;
				
				// Check if player is close enough to collect
				if (IsPadCollectible(player.phys.pos, padPos)) {
					// Check if this pad was nearby in previous frame (indicating intentional collection)
					if (history.size() > 1 && history[history.size() - 2].nearbyPads[i]) {
						float padMultiplier = GetPadPreferenceMultiplier(padPos);
						reward += config.collectionReward * padMultiplier;
						
						// Update collection count
						boostPadStates[i].collectionCount++;
					}
				}
			}
		}

		return reward;
	}

	float AdvancedBoostPathingReward::CalculatePathingReward(const PlayerData& player, const GameState& state) {
		float reward = 0.0f;
		auto& history = playerHistories[player.carId];
		
		if (history.size() < 2) return 0.0f;
		
		Vec currentPos = player.phys.pos;
		Vec lastPos = history[history.size() - 2].position;
		
		// Check if player is moving toward nearby boost pads
		for (int i = 0; i < CommonValues::BOOST_LOCATIONS_AMOUNT; i++) {
			if (state.boostPads[i]) {
				Vec padPos = boostPadStates[i].position;
				float distance = currentPos.Dist2D(padPos);
				
				if (distance <= config.proximityThreshold) {
					// Calculate if player is moving toward the pad
					Vec directionToPad = (padPos - currentPos).Normalized();
					Vec playerMovement = (currentPos - lastPos).Normalized();
					
					float alignment = directionToPad.Dot(playerMovement);
					
					if (alignment > 0.2f) { // Moving toward the pad
						float distanceFactor = 1.0f - (distance / config.proximityThreshold);
						distanceFactor = std::max(0.0f, std::min(1.0f, distanceFactor));
						
						float padMultiplier = GetPadPreferenceMultiplier(padPos);
						reward += config.pathingReward * alignment * distanceFactor * padMultiplier;
					}
				}
			}
		}

		return reward;
	}

	float AdvancedBoostPathingReward::CalculateTimingReward(const PlayerData& player, const GameState& state) {
		float reward = 0.0f;
		
		// Check if player collected pads just as they spawned
		for (int i = 0; i < CommonValues::BOOST_LOCATIONS_AMOUNT; i++) {
			if (state.boostPads[i]) {
				auto& padState = boostPadStates[i];
				
				// Check if pad was recently spawned
				float timeSinceSpawn = currentTime - padState.lastCollectionTime;
				if (timeSinceSpawn <= config.timingWindow) {
					Vec padPos = padState.position;
					
					// Check if player is near this recently spawned pad
					if (IsPadNearby(player.phys.pos, padPos)) {
						float timingFactor = 1.0f - (timeSinceSpawn / config.timingWindow);
						float padMultiplier = GetPadPreferenceMultiplier(padPos);
						reward += config.timingReward * timingFactor * padMultiplier;
					}
				}
			}
		}

		return reward;
	}

	float AdvancedBoostPathingReward::CalculateEfficiencyReward(const PlayerData& player, const GameState& state) {
		float reward = 0.0f;
		
		// Reward for efficient boost usage when boost is low
		if (player.boostFraction < config.boostEfficiencyThreshold) {
			// Check if player is near boost pads when they need boost
			bool nearBoostPad = false;
			for (int i = 0; i < CommonValues::BOOST_LOCATIONS_AMOUNT; i++) {
				if (state.boostPads[i] && IsPadNearby(player.phys.pos, boostPadStates[i].position)) {
					nearBoostPad = true;
					break;
				}
			}

			if (nearBoostPad) {
				// Reward for being near boost when low on boost
				reward += config.efficiencyReward * (1.0f - player.boostFraction);
			}
		}

		return reward;
	}

	float AdvancedBoostPathingReward::CalculateStrategicReward(const PlayerData& player, const GameState& state) {
		float reward = 0.0f;
		auto priorities = CalculatePadPriorities(player.phys.pos, state);
		
		// Reward for choosing strategic boost pads
		for (int i = 0; i < CommonValues::BOOST_LOCATIONS_AMOUNT; i++) {
			if (state.boostPads[i] && IsPadNearby(player.phys.pos, boostPadStates[i].position)) {
				Vec padPos = boostPadStates[i].position;
				float priority = priorities[i];
				
				// Reward for choosing high-priority pads
				if (priority > 0.7f) {
					float padMultiplier = GetPadPreferenceMultiplier(padPos);
					reward += config.strategicReward * priority * padMultiplier;
				}
			}
		}

		return reward;
	}

	std::vector<float> AdvancedBoostPathingReward::CalculatePadDistances(const Vec& playerPos, const GameState& state) {
		std::vector<float> distances;
		distances.reserve(CommonValues::BOOST_LOCATIONS_AMOUNT);

		for (int i = 0; i < CommonValues::BOOST_LOCATIONS_AMOUNT; i++) {
			if (state.boostPads[i]) {
				float distance = playerPos.Dist2D(boostPadStates[i].position);
				distances.push_back(distance);
			} else {
				distances.push_back(std::numeric_limits<float>::max());
			}
		}

		return distances;
	}

	std::vector<float> AdvancedBoostPathingReward::CalculatePadPriorities(const Vec& playerPos, const GameState& state) {
		std::vector<float> priorities;
		priorities.reserve(CommonValues::BOOST_LOCATIONS_AMOUNT);

		for (int i = 0; i < CommonValues::BOOST_LOCATIONS_AMOUNT; i++) {
			if (state.boostPads[i]) {
				Vec padPos = boostPadStates[i].position;
				float distance = playerPos.Dist2D(padPos);
				
				// Calculate priority based on distance and pad type
				float distancePriority = 1.0f - (distance / 5000.0f); // Normalize by field size
				distancePriority = std::max(0.0f, std::min(1.0f, distancePriority));
				
				float typePriority = boostPadStates[i].isLargePad ? 1.0f : 0.5f;
				float locationPriority = GetPadPreferenceMultiplier(padPos) / config.largePadPreference;
				
				float priority = (distancePriority + typePriority + locationPriority) / 3.0f;
				priorities.push_back(priority);
			} else {
				priorities.push_back(0.0f);
			}
		}

		return priorities;
	}

	bool AdvancedBoostPathingReward::IsPadNearby(const Vec& playerPos, const Vec& padPos) const {
		return playerPos.Dist2D(padPos) <= config.proximityThreshold;
	}

	bool AdvancedBoostPathingReward::IsPadCollectible(const Vec& playerPos, const Vec& padPos) const {
		return playerPos.Dist2D(padPos) <= config.collectionThreshold;
	}

	float AdvancedBoostPathingReward::GetPadPreferenceMultiplier(const Vec& padPos) const {
		float multiplier = 1.0f;
		
		if (IsLargePad(padPos)) {
			multiplier *= config.largePadPreference;
		}
		if (IsBackWallPad(padPos)) {
			multiplier *= config.backWallPreference;
		}
		if (IsCenterPad(padPos)) {
			multiplier *= config.centerPreference;
		}
		
		return multiplier;
	}

}