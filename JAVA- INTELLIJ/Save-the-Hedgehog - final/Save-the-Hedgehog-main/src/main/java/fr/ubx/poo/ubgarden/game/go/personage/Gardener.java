/*
 * Copyright (c) 2020. Laurent Réveillère
 */

package fr.ubx.poo.ubgarden.game.go.personage;

import fr.ubx.poo.ubgarden.game.Direction;
import fr.ubx.poo.ubgarden.game.Game;
import fr.ubx.poo.ubgarden.game.Map;
import fr.ubx.poo.ubgarden.game.Position;
import fr.ubx.poo.ubgarden.game.go.GameObject;
import fr.ubx.poo.ubgarden.game.go.Movable;
import fr.ubx.poo.ubgarden.game.go.PickupVisitor;
import fr.ubx.poo.ubgarden.game.go.WalkVisitor;
import fr.ubx.poo.ubgarden.game.go.bonus.Carrot;
import fr.ubx.poo.ubgarden.game.go.bonus.EnergyBoost;
import fr.ubx.poo.ubgarden.game.go.bonus.InsecticideBomb;
import fr.ubx.poo.ubgarden.game.go.bonus.PoisonedApple;
import fr.ubx.poo.ubgarden.game.go.decor.*;
import fr.ubx.poo.ubgarden.game.Configuration;

import java.util.ArrayList;
import java.util.List;

public class Gardener extends GameObject implements Movable, PickupVisitor, WalkVisitor {

    private long lastMoveTime;
    private int energy;
    private final int maxEnergy;
    private Direction direction;
    private boolean moveRequested = false;
    private int diseaseLevel = 0;

    // Store the nanoTime when each disease effect should end
    private final List<Long> diseaseExpiryTimestamps = new ArrayList<>();
    private long lastDiseaseDrainTime = 0; // To manage drain interval in update

    private final List<Long> diseaseExpiry = new ArrayList<>();
// Potential future additions: energy recovery timer, disease level etc.

    private boolean switchLevelRequested = false;
    private int targetLevel = -1;
    private int insecticideBombCount = 0;

    public Gardener(Game game, Position position) {
        super(game, position);
        this.direction = Direction.DOWN;
        Configuration config = game.configuration();
        this.maxEnergy = config.gardenerEnergy();
        this.energy = this.maxEnergy;
        this.lastMoveTime = System.nanoTime();
        this.diseaseLevel = 1; // Initialize to 1 (healthy)
        this.lastDiseaseDrainTime = System.nanoTime();
    }

    public void pickUp(EnergyBoost energyBoost) {
        System.out.println("I am taking the apple, +50 energy and curing diseases.");
        this.energy = Math.min(maxEnergy, energy + 50);

        // Cure diseases
        this.diseaseExpiryTimestamps.clear(); // Clear all active poison timers
        this.diseaseLevel = 1; // Reset to base healthy level

        energyBoost.remove();
        setModified(true); // To update status bar
    }

    public void pickUp(PoisonedApple pApple) {
        System.out.println("I am taking the poisoned apple!");
        long diseaseDurationMs = game.configuration().diseaseDuration();
        long diseaseDurationNs = diseaseDurationMs * 1_000_000L;
        long expiryTime = System.nanoTime() + diseaseDurationNs;
        this.diseaseExpiryTimestamps.add(expiryTime);
        // REMOVE: this.diseaseLevel+=1; // diseaseLevel is now derived in update()

        pApple.remove();
        System.out.println("Current active disease effects: " + diseaseExpiryTimestamps.size());
        setModified(true);
    }

    public void pickUp(Carrot carrot) {
        System.out.println("Picked up a carrot!");
        // Remove the carrot first
        carrot.remove();
        // Tell the game to check if this completed the carrot collection for the level
        game.checkCarrotCompletion(this.getPosition().level());
        // No energy change or direct effect on gardener needed based on requirements
        setModified(true); // Gardener state might indirectly change if a door opens elsewhere
    }

    public void setEnergy(int energy) {
        this.energy = energy;
    }

    public int getEnergy() {
        return this.energy;
    }

    public int getDiseaseLevel() {
        return diseaseLevel;
    }

    public void hurt(int damage) {
        this.energy -= damage;
        System.out.println("Gardener hurt! Energy remaining: " + this.energy); // For debugging
        setModified(true); // Modified state to potentially update UI (like status bar)
    }

    public void requestMove(Direction direction) {
        if (direction != this.direction) {
            this.direction = direction;
            setModified(true);
        }
        moveRequested = true;
    }

    @Override
    public final boolean canMove(Direction direction) {
        Position currentPos = getPosition();
        Position nextPos = direction.nextPosition(currentPos);
        Map currentMap = game.world().getGrid();

        // 1. Boundary Check
        if (!currentMap.inside(nextPos)) {
            return false;
        }

        // 2. Obstacle Check
        Decor targetDecor = currentMap.get(nextPos);

        if (targetDecor == null) {
            return false;
        }

        return targetDecor.walkableBy(this);
    }

    @Override
    public Position move(Direction direction) {
        Position nextPos = direction.nextPosition(getPosition());
        Decor nextDecor = game.world().getGrid().get(nextPos);

        if (nextDecor != null) {
            int baseCost = nextDecor.energyConsumptionWalk();
            // Use getDiseaseLevel() which should reflect the rule (1 for healthy, +1 per poison)
            int actualCost = baseCost * getDiseaseLevel();
            hurt(actualCost); // hurt() already calls setModified(true)
        }

        setPosition(nextPos);

        if (nextDecor != null) {
            nextDecor.pickUpBy(this);
        }

        return nextPos;
    }

    public void update(long now) {
        // --- Handle Movement Request ---
        boolean movedThisFrame = false;
        if (moveRequested) {
            if (canMove(direction)) {
                move(direction);
                movedThisFrame = true;
            }
            moveRequested = false;
        }

        // --- Energy Regeneration Logic ---
        if (movedThisFrame) {
            this.lastMoveTime = now;
        } else {
            long recoveryThresholdNs = game.configuration().energyRecoverDuration() * 1_000_000L;
            if (energy < maxEnergy && (now - lastMoveTime) > recoveryThresholdNs) {
                energy++;
                setModified(true);
                this.lastMoveTime = now; // Reset timer for next potential recovery
            }
        }

        // --- Process Disease Effects ---
        boolean diseaseStateChanged = false;

        java.util.Iterator<Long> iterator = diseaseExpiryTimestamps.iterator();
        while (iterator.hasNext()) {
            long expiryTime = iterator.next();
            if (expiryTime <= now) {
                iterator.remove();
                diseaseStateChanged = true;
            }
        }

        int activePoisonEffects = diseaseExpiryTimestamps.size();
        int newCalculatedDiseaseLevel = 1 + activePoisonEffects; // Base level 1 + active effects

        if (this.diseaseLevel != newCalculatedDiseaseLevel || diseaseStateChanged) {
            System.out.println("Disease level updated from " + this.diseaseLevel + " to " + newCalculatedDiseaseLevel);
            this.diseaseLevel = newCalculatedDiseaseLevel;
            setModified(true);
        }

        // Apply energy drain based on *number of active poison effects*
        if (activePoisonEffects > 0) {
            long drainIntervalNs = 1_000_000_000L; // 1 second
            if (now - lastDiseaseDrainTime >= drainIntervalNs) {
                int drainPerApplePerSecond = 5; // Should be from Configuration ideally
                int totalDrain = drainPerApplePerSecond * activePoisonEffects;

                if (totalDrain > 0) {
                    System.out.println("Draining " + totalDrain + " energy due to " + activePoisonEffects + " poisoned apple effects.");
                    hurt(totalDrain);
                    lastDiseaseDrainTime = now;
                }
            }
        }
        // --- End of Disease Processing ---
    }

    public void hurt() {
        // Default hurt, maybe for wasps/hornets later
        hurt(1);
    }

    public Direction getDirection() {
        return direction;
    }

    // --- WalkVisitor Implementation ---
// Allow walking on any Decor by default unless overridden
    @Override
    public boolean canWalkOn(Decor decor) {
        return true; // Default: can walk on any decor
    }

    // Explicitly prevent walking on Trees
    @Override
    public boolean canWalkOn(Tree tree) {
        return false;
    }

    // Explicitly prevent walking on Nests (Wasp)
    @Override
    public boolean canWalkOn(NestWasp nest) {
        return false;
    }

    // Explicitly prevent walking on Nests (Hornet)
    @Override
    public boolean canWalkOn(NestHornet nest) {
        return false;
    }

    // Allow walking on Doors only if they are opened
    @Override
    public boolean canWalkOn(Door door) {
        return door.isOpened();
    }

    // Phương thức để Door gọi khi người chơi đi vào
    public void requestSwitchLevel(int level) {
        this.switchLevelRequested = true;
        this.targetLevel = level;
        System.out.println("Gardener internal state: switch level requested to " + level);
        // Không cần gọi game.requestSwitchLevel trực tiếp từ đây nữa
        // GameEngine sẽ kiểm tra cờ này của Gardener
    }

    // Phương thức để GameEngine kiểm tra
    public boolean isSwitchLevelRequested() {
        return switchLevelRequested;
    }

    public int getTargetLevel() {
        return targetLevel;
    }

    // Phương thức để GameEngine gọi sau khi xử lý xong yêu cầu
    public void clearSwitchLevelRequest() {
        this.switchLevelRequested = false;
        this.targetLevel = -1;
    }

    public void pickUp(InsecticideBomb bomb) { // Modified from original
        System.out.println("Gardener picked up an insecticide bomb!");
        if (this.insecticideBombCount < game.configuration().gardenerBombCapacity()) {
            this.insecticideBombCount++;
            bomb.remove(); // Remove the bomb from the map
        } else {
            System.out.println("Bomb capacity full! Cannot pick up.");
            // Bomb remains on the ground if capacity is full
        }
        setModified(true); // To update status bar
    }

    public int getInsecticideBombCount() {
        return insecticideBombCount;
    }

    public boolean useBomb() {
        if (insecticideBombCount > 0) {
            insecticideBombCount--;
            System.out.println("Gardener used a bomb. Bombs left: " + insecticideBombCount);
            setModified(true); // To update status bar
            return true;
        }
        return false;
    }

    public void stungByWasp() {
        hurt(game.configuration().waspDamage()); // Use damage from config
        System.out.println("OUCH! Gardener stung by wasp! Energy: " + getEnergy());
    }
    public void stungByHornet() {
        int damage = game.configuration().hornetDamage(); // Lấy sát thương từ config
        hurt(damage);
        System.out.println("OUCH! Gardener stung by HORNET! Energy: " + getEnergy() + ", Damage: " + damage);
    }


    public int getActivePoisonEffectCount() {
        return this.diseaseExpiryTimestamps.size();
    }

}