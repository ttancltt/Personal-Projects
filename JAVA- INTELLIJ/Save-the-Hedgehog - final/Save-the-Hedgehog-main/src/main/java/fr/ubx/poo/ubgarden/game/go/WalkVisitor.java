package fr.ubx.poo.ubgarden.game.go;

import fr.ubx.poo.ubgarden.game.go.decor.*;

public interface WalkVisitor {

    /**
     * Determines whether the visitor can walk on the given {@link Decor}.
     *
     * @param decor the decor to evaluate
     * @return true if the visitor can walk on the decor, false by default
     */
    default boolean canWalkOn(Decor decor) {
        return true;
    }

    /**
     * Determines whether the visitor can walk on the given {@link Tree}.
     *
     * @param tree the tree to evaluate
     * @return true if the visitor can walk on the tree, false by default
     */
    default boolean canWalkOn(Tree tree) {
        return false;
    }

    // Explicitly prevent walking on Nests (Wasp)
    boolean canWalkOn(NestWasp nest);

    // Explicitly prevent walking on Nests (Hornet)
    boolean canWalkOn(NestHornet nest);

    // Allow walking on Doors only if they are opened
    boolean canWalkOn(Door door);

    // TODO
}