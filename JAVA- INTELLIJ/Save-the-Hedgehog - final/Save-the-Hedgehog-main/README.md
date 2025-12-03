# UGarden 2025

## Project Overview
University project something something... public static void main args

## Implementation Roadmap

### 1. Core Gameplay Foundation

- Implement display of all game elements including the hedgehog and bonuses
- Use MapLevelDefaultStart as the initial map
- Implement movement restrictions: prevent moving outside map boundaries or through obstacles
- Set up victory condition (finding the hedgehog) and defeat condition (gardener death)
- Display player stats in the info panel: energy level, fatigue level, and bomb count
- Implement energy consumption with each movement
- Allow energy regeneration when the gardener remains stationary
- Enable bonus item collection mechanics
- Implement apple effects: health restoration and energy boost
- Implement rotten apple core effects: temporary illness and increased fatigue level

### 2. World Management

- Enable loading maps from external files
- Implement door and carrot collection mechanics
- Make doors open automatically when all carrots on the level have been collected

### 3. Wasp Behavior System

- Program wasp nests to generate a new wasp every 5 seconds
- Implement random movement patterns for wasps
- Wasps die after stinging the player once or when hit by a bomb
- Adjust movement frequency based on the waspMoveFrequency parameter

### 4. Insecticide Bomb System

- Each newly spawned wasp triggers a bomb to appear at a random location
- Allow the gardener to collect and carry bombs
- Implement automatic bomb usage when encountering a wasp (if bombs are available)

### 5. Hornet Extension

- Each newly spawned hornet triggers two bombs to appear at random locations
- Program hornet nests to generate a new hornet every 10 seconds
- Implement hornet movement similar to wasps, but with double sting capability
- Hornets are damaged but not immediately killed when they move over a bomb
- Adjust movement frequency based on the hornetMoveFrequency parameter

## Architecture

```
Main
└── GameLauncherView (UI, Menu)
    ├── GameLauncher (Loads game config/map)
    │   └── Game (Holds game state, World, Gardener)
    │       └── World (Holds Maps for levels)
    │           └── Level (Represents a single map grid)
    │
    └── GameEngine (Runs the game loop, handles input, updates, rendering)
        ├── Input (Detects key presses)
        ├── Gardener.requestMove() (Processes input)
        ├── Gardener.update() (Applies movement if requested/possible)
        └── render() (Calls Sprite.render() for all sprites)
            └── Sprite.render() (Updates ImageView position/image based on GameObject state)
```


## Game Graphics Reference
![Game animation preview](https://github.com/DrKief/Save-the-Hedgehog/blob/main/fish.gif)

---
*Note: The image displayed above is for illustrative purposes only and does not represent the final appearance of the game. Actual gameplay visuals may differ substantially.*

## For more details visit these links
- [Project Documentation](https://www.youtube.com/watch?v=eYVOSQZZfFs)
- [More Project Documentation](https://www.reveillere.fr/POO/projet/index-2025.html)