#include "BoostPathingReward.h"
#include <algorithm>
#include <cmath>

namespace RLGSC {

	BoostPathingReward::BoostPathingReward(const Config& config) : config(config) {
		InitializeBoostPadInfos();
	}

	void BoostPathingReward::InitializeBoostPadInfos() {
		boostPadInfos.clear();
		boostPadInfos.reserve(CommonValues::BOOST_LOCATIONS_AMOUNT);

		for (int i = 0; i < CommonValues::BOOST_LOCATIONS_AMOUNT; i++) {
			BoostPadInfo padInfo;
			padInfo.position = CommonValues::BOOST_LOCATIONS[i];
			padInfo.isLargePad = IsLargePad(padInfo.position);
			padInfo.collectionRadius = padInfo.isLargePad ? 150.0f : 120.0f; // Large pads have larger collection radius
			boostPadInfos.push_back(padInfo);
		}
	}

	bool BoostPathingReward::IsLargePad(const Vec& position) const {
		// Large boost pads are typically at the corners and center of the field
		// Based on the boost pad locations, large pads are at:
		// - Center field (0, 0, 73)
		// - Corner positions with higher Z coordinates (73 instead of 70)
		// - Back wall positions
		
		// Check if it's a center pad
		if (std::abs(position.x) < 50 && std::abs(position.y) < 50) {
			return true;
		}

		// Check if it's a corner pad (higher Z coordinate)
		if (position.z > 70.5f) {
			return true;
		}

		// Check if it's a back wall pad
		if (std::abs(position.y) > 4000) {
			return true;
		}

		return false;
	}

	void BoostPathingReward::Reset(const GameState& initialState) {
		playerStates.clear();
		
		// Initialize player states
		for (const auto& player : initialState.players) {
			PlayerBoostState state;
			state.lastPosition = player.phys.pos;
			state.lastBoostFraction = player.boostFraction;
			state.boostPickups = player.boostPickups;
			state.nearbyPads.resize(CommonValues::BOOST_LOCATIONS_AMOUNT, false);
			state.padDistances.resize(CommonValues::BOOST_LOCATIONS_AMOUNT, 0.0f);
			playerStates[player.carId] = state;
		}
	}

	void BoostPathingReward::PreStep(const GameState& state) {
		// Update player states with current information
		for (const auto& player : state.players) {
			auto& playerState = playerStates[player.carId];
			
			// Calculate distances to all boost pads
			playerState.padDistances = CalculatePadDistances(player.phys.pos, state);
			
			// Update nearby pad flags
			for (int i = 0; i < CommonValues::BOOST_LOCATIONS_AMOUNT; i++) {
				if (state.boostPads[i]) { // Only consider active pads
					playerState.nearbyPads[i] = IsPadNearby(player.phys.pos, boostPadInfos[i].position);
				} else {
					playerState.nearbyPads[i] = false;
				}
			}
		}
	}

	float BoostPathingReward::GetReward(const PlayerData& player, const GameState& state, const Action& prevAction) {
		float totalReward = 0.0f;

		// Calculate different components of the reward
		totalReward += CalculateProximityReward(player, state);
		totalReward += CalculatePathingReward(player, state);
		totalReward += CalculateEfficiencyReward(player, state);
		totalReward += CalculateCollectionReward(player, state);

		// Update player state for next frame
		auto& playerState = playerStates[player.carId];
		playerState.lastPosition = player.phys.pos;
		playerState.lastBoostFraction = player.boostFraction;
		playerState.boostPickups = player.boostPickups;

		return totalReward;
	}

	float BoostPathingReward::CalculateProximityReward(const PlayerData& player, const GameState& state) {
		float reward = 0.0f;
		const auto& playerState = playerStates[player.carId];

		for (int i = 0; i < CommonValues::BOOST_LOCATIONS_AMOUNT; i++) {
			if (state.boostPads[i] && playerState.nearbyPads[i]) {
				// Reward for being near an active boost pad
				float distance = playerState.padDistances[i];
				float proximityFactor = 1.0f - (distance / config.proximityThreshold);
				proximityFactor = std::max(0.0f, std::min(1.0f, proximityFactor));
				
				// Give higher reward for large pads
				float padMultiplier = boostPadInfos[i].isLargePad ? 2.0f : 1.0f;
				reward += config.proximityReward * proximityFactor * padMultiplier;
			}
		}

		return reward;
	}

	float BoostPathingReward::CalculatePathingReward(const PlayerData& player, const GameState& state) {
		float reward = 0.0f;
		const auto& playerState = playerStates[player.carId];
		Vec currentPos = player.phys.pos;
		Vec lastPos = playerState.lastPosition;

		// Check if player is moving toward nearby boost pads
		for (int i = 0; i < CommonValues::BOOST_LOCATIONS_AMOUNT; i++) {
			if (state.boostPads[i] && playerState.nearbyPads[i]) {
				Vec padPos = boostPadInfos[i].position;
				
				// Calculate if player is moving toward the pad
				Vec directionToPad = (padPos - currentPos).Normalized();
				Vec playerMovement = (currentPos - lastPos).Normalized();
				
				// Dot product indicates alignment (1 = moving directly toward, -1 = moving away)
				float alignment = directionToPad.Dot(playerMovement);
				
				if (alignment > 0.3f) { // Moving toward the pad
					float distance = playerState.padDistances[i];
					float distanceFactor = 1.0f - (distance / config.proximityThreshold);
					distanceFactor = std::max(0.0f, std::min(1.0f, distanceFactor));
					
					float padMultiplier = boostPadInfos[i].isLargePad ? 1.5f : 1.0f;
					reward += config.pathingReward * alignment * distanceFactor * padMultiplier;
				}
			}
		}

		return reward;
	}

	float BoostPathingReward::CalculateEfficiencyReward(const PlayerData& player, const GameState& state) {
		float reward = 0.0f;
		const auto& playerState = playerStates[player.carId];

		// Reward for efficient boost usage when boost is low
		if (player.boostFraction < config.boostEfficiencyThreshold) {
			// Check if player is near boost pads when they need boost
			bool nearBoostPad = false;
			for (int i = 0; i < CommonValues::BOOST_LOCATIONS_AMOUNT; i++) {
				if (state.boostPads[i] && playerState.nearbyPads[i]) {
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

	float BoostPathingReward::CalculateCollectionReward(const PlayerData& player, const GameState& state) {
		float reward = 0.0f;
		const auto& playerState = playerStates[player.carId];

		// Check if player collected any boost pads
		for (int i = 0; i < CommonValues::BOOST_LOCATIONS_AMOUNT; i++) {
			if (state.boostPads[i]) {
				Vec padPos = boostPadInfos[i].position;
				
				// Check if player is close enough to collect the pad
				if (IsPadCollectible(player.phys.pos, padPos)) {
					// Check if this pad was nearby in the previous frame (indicating intentional collection)
					if (playerState.nearbyPads[i]) {
						float padMultiplier = boostPadInfos[i].isLargePad ? 3.0f : 1.0f;
						reward += config.collectionReward * padMultiplier;
					}
				}
			}
		}

		return reward;
	}

	std::vector<float> BoostPathingReward::CalculatePadDistances(const Vec& playerPos, const GameState& state) {
		std::vector<float> distances;
		distances.reserve(CommonValues::BOOST_LOCATIONS_AMOUNT);

		for (int i = 0; i < CommonValues::BOOST_LOCATIONS_AMOUNT; i++) {
			if (state.boostPads[i]) {
				float distance = playerPos.Dist2D(boostPadInfos[i].position);
				distances.push_back(distance);
			} else {
				distances.push_back(std::numeric_limits<float>::max()); // Pad not available
			}
		}

		return distances;
	}

	bool BoostPathingReward::IsPadNearby(const Vec& playerPos, const Vec& padPos) const {
		return playerPos.Dist2D(padPos) <= config.proximityThreshold;
	}

	bool BoostPathingReward::IsPadCollectible(const Vec& playerPos, const Vec& padPos) const {
		// Find the pad info for this position
		for (const auto& padInfo : boostPadInfos) {
			if (padInfo.position.Dist2D(padPos) < 10.0f) { // Close enough to be the same pad
				return playerPos.Dist2D(padPos) <= padInfo.collectionRadius;
			}
		}
		return false;
	}

}