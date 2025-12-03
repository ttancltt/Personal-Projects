package fr.ubx.poo.ubgarden.game;

import fr.ubx.poo.ubgarden.game.go.bonus.Carrot;
import fr.ubx.poo.ubgarden.game.go.bonus.InsecticideBomb;
import fr.ubx.poo.ubgarden.game.go.decor.Decor;
import fr.ubx.poo.ubgarden.game.go.decor.Door;
import fr.ubx.poo.ubgarden.game.go.decor.ground.Dirt;
import fr.ubx.poo.ubgarden.game.go.decor.ground.Grass;
import fr.ubx.poo.ubgarden.game.go.personage.Gardener;
import fr.ubx.poo.ubgarden.game.go.personage.Hornet;
import fr.ubx.poo.ubgarden.game.go.personage.Wasp;

import java.util.*;

public class Game {

    private final Configuration configuration;
    private final World world;
    private final Gardener gardener;
    private boolean switchLevelRequested = false;
    private int switchLevel;

    private final List<Wasp> activeWasps = new ArrayList<>();
    private final List<Hornet> activeHornets = new ArrayList<>();
    private final Random random = new Random();

    public Game(World world, Configuration configuration, Position gardenerPosition) {
        this.configuration = configuration;
        this.world = world;
        gardener = new Gardener(this, gardenerPosition);
    }

    public Configuration configuration() {
        return configuration;
    }

    public Gardener getGardener() {
        return this.gardener;
    }

    public World world() {
        return world;
    }

    public boolean isSwitchLevelRequested() {
        return switchLevelRequested;
    }

    public int getSwitchLevel() {
        return switchLevel;
    }

    public void requestSwitchLevel(int level) {
        this.switchLevel = level;
        switchLevelRequested = true;
    }

    public void clearSwitchLevel() {
        switchLevelRequested = false;
    }
    public void checkCarrotCompletion(int level) {
        Map currentMap = world.getGrid(level);
        if (currentMap == null) {
            System.err.println("Warning: Tried to check carrot completion on non-existent level: " + level);
            return;
        }

        // Step 1: Check if any carrots remain on the level
        boolean carrotsRemain = false;
        for (Decor decor : currentMap.values()) {
            if (decor.getBonus() instanceof Carrot) {
                carrotsRemain = true;
                break; // Found a carrot, no need to check further
            }
        }

        // Step 2: If no carrots remain, find and open the closed door(s)
        if (!carrotsRemain) {
            System.out.println("All carrots collected on level " + level + "! Checking for doors to open.");
            for (Decor decor : currentMap.values()) {
                // Check if it's a Door object and if it's currently closed
                // We assume MapEntity.DoorNextClosed corresponds to a Door object with isOpened() == false
                if (decor instanceof Door) {
                    Door door = (Door) decor;
                    if (!door.isOpened()) {
                        System.out.println("Opening door at " + door.getPosition());
                        door.setOpened(true);
                        door.setModified(true); // IMPORTANT: Mark for sprite update
                        // Optional: break here if you only expect one such door per level
                    }
                }
            }
        }
    }
    public void addWasp(Wasp wasp) {
        if (wasp != null) {
            activeWasps.add(wasp);
        }
    }

    // Called by NestWasp
    public void spawnWasp(Position position) {
        Wasp wasp = new Wasp(this, position);
        addWasp(wasp);
    }
    public void spawnHornet(Position position) {
        Hornet hornet = new Hornet(this, position);
        if (hornet != null) {
            activeHornets.add(hornet);
            System.out.println("Game: Hornet spawned by Nest. Active hornets: " + activeHornets.size());
        }
    }


    public List<Wasp> getActiveWasps() {
        return Collections.unmodifiableList(activeWasps);
    }

    public List<Hornet> getActiveHornets() {return Collections.unmodifiableList(activeHornets);}

    // Called by GameEngine during its update cycle
    public void updateDynamicEntities(long now) {
        // Update and remove dead wasps
        Iterator<Wasp> iterator = activeWasps.iterator();
        while (iterator.hasNext()) {
            Wasp wasp = iterator.next();
            if (wasp.isDeleted()) {
                iterator.remove();
            } else {
                wasp.update(now); // Wasp updates its own state (movement timer, etc.)
            }
        }

        Iterator<Hornet> hornetIterator = activeHornets.iterator();
        while (hornetIterator.hasNext()) {
            Hornet hornet = hornetIterator.next();
            if (hornet.isDeleted()) {
                hornetIterator.remove();
            } else {
                hornet.update(now);
            }
        }
        // Potentially update other dynamic entities here in the future
    }

    public void spawnRandomBomb(int levelNumber) {
        Map currentMap = world.getGrid(levelNumber);
        if (currentMap == null) {
            System.err.println("Cannot spawn bomb: map for level " + levelNumber + " not found.");
            return;
        }

        List<Decor> suitableDecor = new ArrayList<>();
        for (Decor decor : currentMap.values()) {
            // Bomb can be placed on Grass or Dirt if it doesn't already have a bonus
            if ((decor instanceof Grass || decor instanceof Dirt) && decor.getBonus() == null) {
                suitableDecor.add(decor);
            }
        }

        if (suitableDecor.isEmpty()) {
            System.err.println("Warning: No suitable position to spawn a random bomb on level " + levelNumber);
            return;
        }

        Random random = new Random();
        Decor targetDecor = suitableDecor.get(random.nextInt(suitableDecor.size()));
        Position bombPos = targetDecor.getPosition();

        InsecticideBomb bomb = new InsecticideBomb(bombPos, targetDecor);
        targetDecor.setBonus(bomb);
        targetDecor.setModified(true); // To update decor sprite if it changes (e.g. to show bomb)
        bomb.setModified(true);      // To render the new bomb sprite
        System.out.println("Spawned random insecticide bomb at " + bombPos);
    }

}
