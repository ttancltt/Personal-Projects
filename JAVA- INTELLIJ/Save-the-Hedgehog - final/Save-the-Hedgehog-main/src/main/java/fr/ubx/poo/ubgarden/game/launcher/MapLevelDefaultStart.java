package fr.ubx.poo.ubgarden.game.launcher;

import fr.ubx.poo.ubgarden.game.go.decor.ground.Ground;

import static fr.ubx.poo.ubgarden.game.launcher.MapEntity.*;

public class MapLevelDefaultStart extends MapLevel {

    private final static int width = 18;
    private final static int height = 8;
    private final MapEntity[][] level1 = {
            {Grass, Grass, Land, Land, Land, Grass, Grass, Grass, Grass, Grass, Flowers, Grass, Grass, Tree, Grass, Grass, NestWasp, Grass},
            {Grass, Gardener, Grass, Grass, PoisonedApple, Apple, Flowers, Grass, Grass, Tree, Grass, Grass, Tree, Tree, Grass, Grass, Grass, Grass},
            {Grass, Grass, Grass, Grass, Grass, Grass, Land, Land, Grass, Grass, Grass, Grass, Grass, Tree, Grass, Grass, Grass, Grass},
            {Grass, Grass, Grass, Grass, Grass, Grass, Grass, Grass, Carrots, Grass, NestHornet, Grass, Grass, Tree, Grass, Grass, Flowers, Grass},
            {Grass, Tree, Grass, Tree, Grass, Grass, Grass, Grass, Grass, Grass, Land, Land, Grass, Tree, Grass, Grass, Apple, Grass},
            {Grass, Tree, Tree, Tree, DoorNextClosed, Grass, Grass, Flowers, Grass, Grass, Grass, Grass, Grass, Grass, Grass, Grass, Grass, Grass},
            {Grass, Land, Land, Grass, Grass, Grass, PoisonedApple, Grass, Grass, Grass, Grass, Grass, Carrots, Grass, Grass, NestWasp, Grass, Grass},
            {Grass, Tree, Grass, Tree, Grass, Hedgehog, Grass, Grass, Grass, Grass, Land, Grass, Grass, Grass, Grass, Grass, Grass, Grass}
    };

    public MapLevelDefaultStart() {
        super(width, height);
        for (int i = 0; i < width; i++)
            for (int j = 0; j < height; j++)
                set(i, j, level1[j][i]);
    }
}
