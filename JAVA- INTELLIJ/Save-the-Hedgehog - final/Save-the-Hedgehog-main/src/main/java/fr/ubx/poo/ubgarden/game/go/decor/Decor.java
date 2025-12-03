package fr.ubx.poo.ubgarden.game.go.decor;

import fr.ubx.poo.ubgarden.game.Game;
import fr.ubx.poo.ubgarden.game.Position;
import fr.ubx.poo.ubgarden.game.go.GameObject;
import fr.ubx.poo.ubgarden.game.go.Pickupable;
import fr.ubx.poo.ubgarden.game.go.Walkable;
import fr.ubx.poo.ubgarden.game.go.bonus.Bonus;
import fr.ubx.poo.ubgarden.game.go.personage.Gardener;

public abstract class Decor extends GameObject implements Walkable, Pickupable {

    private Bonus bonus;

    public Decor(Position position) {
        super(position);
    }

    public Decor(Position position, Bonus bonus) {
        super(position);
        this.bonus = bonus;
    }

    // NEW: Constructor for Decors that DO need a Game reference (e.g., for configuration)
    // This will be called by subclasses like NestWasp.
    public Decor(Game game, Position position) {
        super(game, position); // Pass game to GameObject
    }

    // NEW: Constructor for Decors with a bonus that also need a Game reference.
    public Decor(Game game, Position position, Bonus bonus) {
        super(game, position); // Pass game to GameObject
        this.bonus = bonus;
    }


    public Bonus getBonus() {
        return bonus;
    }

    public void setBonus(Bonus bonus) {
        this.bonus = bonus;
    }

    @Override
    public boolean walkableBy(Gardener gardener) {
        return gardener.canWalkOn(this);
    }

    @Override
    public void update(long now) {
        super.update(now);
        if (bonus != null) bonus.update(now);
    }



}