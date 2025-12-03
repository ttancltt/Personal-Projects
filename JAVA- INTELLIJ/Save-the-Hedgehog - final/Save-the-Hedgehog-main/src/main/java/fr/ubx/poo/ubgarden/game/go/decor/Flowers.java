// In class: fr.ubx.poo.ubgarden.game.go.decor.Flowers
package fr.ubx.poo.ubgarden.game.go.decor;

import fr.ubx.poo.ubgarden.game.Position;
import fr.ubx.poo.ubgarden.game.go.personage.Gardener; // Make sure this import is present

public class Flowers extends Decor {
    public Flowers(Position position) {
        super(position);
    }

    @Override
    public boolean walkableBy(Gardener gardener) {
        // Gardener cannot walk on Flowers. This overrides the default Decor behavior.
        return false;
    }
}