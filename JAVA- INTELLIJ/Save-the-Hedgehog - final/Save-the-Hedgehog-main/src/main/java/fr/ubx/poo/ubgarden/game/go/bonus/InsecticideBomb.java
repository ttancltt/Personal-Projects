package fr.ubx.poo.ubgarden.game.go.bonus;

import fr.ubx.poo.ubgarden.game.Position;
import fr.ubx.poo.ubgarden.game.go.decor.Decor;
import fr.ubx.poo.ubgarden.game.go.personage.Gardener;

public class InsecticideBomb extends Bonus {
    public InsecticideBomb(Position position, Decor decor) {
        super(position, decor);
    }

    @Override
    public void pickUpBy(Gardener gardener) {
        // Implement insecticide bomb effect
        System.out.println("Picked up an insecticide bomb!");
        gardener.pickUp(this);
    }
}