package fr.ubx.poo.ubgarden.game.go.decor.ground;

import fr.ubx.poo.ubgarden.game.Position;

public class Dirt extends Ground {
    public Dirt(Position position) {
        super(position);
    }

    @Override
    public int energyConsumptionWalk() {
        return 2;
    }
}