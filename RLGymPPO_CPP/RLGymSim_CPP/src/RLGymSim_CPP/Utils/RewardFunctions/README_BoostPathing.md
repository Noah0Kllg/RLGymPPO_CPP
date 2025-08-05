# Boost Pathing Reward Functions for RLGymPPO-CPP

This directory contains advanced reward functions designed to train Rocket League bots with sophisticated boost management skills. The reward functions encourage efficient boost collection behavior through multiple mechanisms.

## Overview

Boost pathing is a crucial skill in Rocket League that involves:
- Efficiently collecting boost pads while maintaining game flow
- Prioritizing boost pads based on type (large vs small) and location
- Timing boost collection to maximize efficiency
- Managing boost levels to avoid starvation

## Files

### Core Implementation
- `BoostPathingReward.h/cpp` - Basic boost pathing reward function
- `AdvancedBoostPathingReward.h/cpp` - Advanced version with timing and strategic rewards
- `BoostPathingExample.cpp` - Usage examples and documentation

## Reward Components

### 1. Proximity Rewards
Rewards the agent for being near active boost pads.
- **Purpose**: Encourages the agent to stay near boost sources
- **Scaling**: Distance-based (closer = higher reward)
- **Differentiation**: Large pads (100 boost) get higher rewards than small pads (12 boost)

### 2. Collection Rewards
Rewards successful boost pad collection.
- **Purpose**: Reinforces successful boost gathering
- **Intentionality**: Only rewards collection if the pad was nearby in the previous frame
- **Multipliers**: Different rewards for different pad types

### 3. Pathing Rewards
Rewards movement toward nearby boost pads.
- **Purpose**: Encourages intentional movement toward boost
- **Method**: Uses dot product to measure alignment with pad direction
- **Scaling**: Distance-based scaling

### 4. Timing Rewards (Advanced)
Rewards collecting pads just as they spawn.
- **Purpose**: Encourages efficient timing and prediction
- **Window**: Configurable time window after spawn
- **Strategy**: Higher rewards for quick collection

### 5. Efficiency Rewards
Rewards being near boost when boost is low.
- **Purpose**: Prevents boost starvation
- **Threshold**: Configurable boost level threshold
- **Proactivity**: Encourages proactive boost management

### 6. Strategic Rewards (Advanced)
Rewards choosing high-priority boost pads.
- **Purpose**: Encourages strategic decision making
- **Factors**: Considers pad type, location, and distance
- **Priority**: Calculates pad priorities based on multiple factors

## Boost Pad Information

### Pad Types
- **Large Pads (100 boost)**: Center field, corners, back walls
- **Small Pads (12 boost)**: Distributed throughout the field

### Pad Locations
The system uses the standard Rocket League boost pad locations:
- 34 total boost pads
- All positions hardcoded in `CommonValues::BOOST_LOCATIONS`
- Automatic detection of large vs small pads

### Pad States
- **Active**: Pad is available for collection
- **Inactive**: Pad is on cooldown
- **Cooldown Timer**: Time remaining until pad respawns

## Configuration

### Basic Configuration (BoostPathingReward)
```cpp
BoostPathingReward::Config config;
config.proximityReward = 0.1f;        // Reward for being near boost pads
config.collectionReward = 1.0f;       // Reward for collecting boost pads
config.pathingReward = 0.05f;         // Reward for moving toward boost pads
config.efficiencyReward = 0.2f;       // Reward for efficient boost usage
config.proximityThreshold = 300.0f;   // Distance threshold for "nearby" pads
config.collectionThreshold = 100.0f;  // Distance threshold for collection
config.boostEfficiencyThreshold = 0.3f; // Boost level below which efficiency is rewarded
```

### Advanced Configuration (AdvancedBoostPathingReward)
```cpp
AdvancedBoostPathingReward::Config config;
// Basic rewards
config.proximityReward = 0.05f;
config.collectionReward = 1.0f;
config.pathingReward = 0.03f;

// Timing-based rewards
config.timingReward = 0.2f;           // Reward for collecting pads just as they spawn
config.efficiencyReward = 0.15f;      // Reward for efficient boost usage
config.strategicReward = 0.1f;        // Reward for strategic pad selection

// Thresholds
config.proximityThreshold = 400.0f;
config.collectionThreshold = 120.0f;
config.boostEfficiencyThreshold = 0.4f;
config.timingWindow = 0.5f;           // seconds after pad spawn to get timing reward

// Strategic parameters
config.largePadPreference = 2.0f;     // Multiplier for preferring large pads
config.backWallPreference = 1.5f;     // Multiplier for back wall pads
config.centerPreference = 1.3f;       // Multiplier for center pads
```

## Usage Examples

### Basic Usage
```cpp
#include "BoostPathingReward.h"

// Create basic boost pathing reward
BoostPathingReward::Config config;
config.proximityReward = 0.1f;
config.collectionReward = 1.0f;
auto reward = std::make_shared<BoostPathingReward>(config);
```

### Advanced Usage
```cpp
#include "AdvancedBoostPathingReward.h"

// Create advanced boost pathing reward
AdvancedBoostPathingReward::Config config;
config.timingReward = 0.2f;
config.strategicReward = 0.1f;
auto reward = std::make_shared<AdvancedBoostPathingReward>(config);
```

### Combined with Other Rewards
```cpp
#include "BoostPathingExample.cpp"

// Use the provided examples
auto reward = BoostPathingExamples::CreateCombinedBoostPathingReward();
```

### Play Style Specialization
```cpp
// Aggressive play style - prioritize large pads and timing
auto aggressiveReward = BoostPathingExamples::CreateAggressiveBoostPathingReward();

// Conservative play style - prioritize efficiency and small pads
auto conservativeReward = BoostPathingExamples::CreateConservativeBoostPathingReward();
```

## Integration with Training

### 1. Add to Your Training Setup
```cpp
// In your main training file
#include "Utils/RewardFunctions/BoostPathingReward.h"
#include "Utils/RewardFunctions/AdvancedBoostPathingReward.h"

// Create reward function
auto boostPathingReward = BoostPathingExamples::CreateAdvancedBoostPathingReward();

// Add to your environment configuration
// (Implementation depends on your specific setup)
```

### 2. Combine with Existing Rewards
```cpp
#include "Utils/RewardFunctions/CombinedReward.h"

std::vector<std::shared_ptr<RewardFunction>> rewards = {
    boostPathingReward,
    velocityReward,
    touchBallReward,
    eventReward
};

auto combinedReward = std::make_shared<CombinedReward>(rewards);
```

### 3. Tune Parameters
Start with the provided examples and adjust parameters based on:
- Training progress
- Desired play style
- Performance metrics
- Boost collection efficiency

## Performance Considerations

### Memory Usage
- Player histories are kept for 2 seconds
- Boost pad states are tracked for all 34 pads
- Minimal memory overhead per player

### Computational Cost
- Distance calculations for all active boost pads
- Historical data updates every frame
- Priority calculations for strategic rewards
- Overall impact: Low to moderate

### Optimization Tips
- Use basic reward for initial training
- Switch to advanced reward for fine-tuning
- Adjust thresholds based on performance
- Monitor boost collection rates during training

## Troubleshooting

### Common Issues

1. **No boost collection rewards**
   - Check if boost pads are active in the game state
   - Verify proximity thresholds are appropriate
   - Ensure player is close enough to collect

2. **Too many/few rewards**
   - Adjust reward magnitudes in configuration
   - Tune proximity and collection thresholds
   - Balance with other reward components

3. **Poor boost pathing behavior**
   - Increase pathing reward magnitude
   - Adjust proximity threshold
   - Consider using advanced reward with timing

### Debugging
- Monitor reward values during training
- Check boost pad states and player positions
- Verify configuration parameters
- Use logging to track reward components

## Best Practices

### Training Strategy
1. Start with basic reward function
2. Gradually increase complexity
3. Monitor boost collection efficiency
4. Adjust parameters based on performance
5. Consider play style specialization

### Parameter Tuning
1. Begin with example configurations
2. Adjust one parameter at a time
3. Monitor training metrics
4. Balance with other rewards
5. Test in actual gameplay

### Integration
1. Combine with existing reward functions
2. Maintain reward balance
3. Consider team coordination
4. Adapt to different game modes
5. Test with different skill levels

## Future Enhancements

### Potential Improvements
- Team coordination rewards
- Opponent boost denial
- Boost pad prediction
- Dynamic priority adjustment
- Game state awareness

### Advanced Features
- Boost pad timing prediction
- Strategic boost denial
- Team boost management
- Adaptive reward scaling
- Performance-based tuning

## Contributing

When contributing to the boost pathing reward functions:
1. Follow the existing code style
2. Add comprehensive documentation
3. Include usage examples
4. Test with different configurations
5. Consider performance implications

## License

This implementation follows the same license as the RLGymPPO-CPP framework.