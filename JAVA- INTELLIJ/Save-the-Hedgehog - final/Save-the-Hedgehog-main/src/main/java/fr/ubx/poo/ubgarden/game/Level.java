package fr.ubx.poo.ubgarden.game;

import fr.ubx.poo.ubgarden.game.go.bonus.Carrot;
import fr.ubx.poo.ubgarden.game.go.bonus.EnergyBoost;
import fr.ubx.poo.ubgarden.game.go.bonus.PoisonedApple;
import fr.ubx.poo.ubgarden.game.go.decor.*;
import fr.ubx.poo.ubgarden.game.go.decor.ground.Dirt;
import fr.ubx.poo.ubgarden.game.go.decor.ground.Grass;
import fr.ubx.poo.ubgarden.game.launcher.MapEntity;
import fr.ubx.poo.ubgarden.game.launcher.MapLevel;

import java.util.Collection;
import java.util.HashMap;

public class Level implements Map {

    private final int level;
    private final int width;

    private final int height;

    private final java.util.Map<Position, Decor> decors = new HashMap<>();

    public Level(Game game, int level, MapLevel entities) {
        this.level = level;
        this.width = entities.width();
        this.height = entities.height();

        for (int i = 0; i < width; i++)
            for (int j = 0; j < height; j++) {
                Position position = new Position(level, i, j);
                MapEntity mapEntity = entities.get(i, j);
                Decor baseDecor = null;
                switch (mapEntity) {
                    case Grass:
                        decors.put(position, new Grass(position));
                        break;
                    case Land:
                        decors.put(position, new Dirt(position));
                        break;
                    case Tree:
                        decors.put(position, new Tree(position));
                        break;
                    case Flowers:
                        decors.put(position, new Flowers(position));
                        break;
                    case Hedgehog:
                        decors.put(position, new Hedgehog(position));
                        break;
                    case DoorNextClosed:
                        // Cửa đóng tới level sau, đặt trên nền cỏ? (Tùy thiết kế)
                        baseDecor = new Grass(position); // Nền cỏ
                        decors.put(position, new Door(position, false, true)); // false=đóng, true=tới
                        baseDecor = null; // Đã xử lý
                        break;
                    case DoorNextOpened:
                        baseDecor = new Grass(position); // Nền cỏ
                        decors.put(position, new Door(position, true, true)); // true=mở, true=tới
                        baseDecor = null; // Đã xử lý
                        break;
                    case DoorPrevOpened:
                        baseDecor = new Grass(position); // Nền cỏ
                        decors.put(position, new Door(position, true, false)); // true=mở, false=lùi
                        baseDecor = null; // Đã xử lý
                        break;
                    case NestWasp:
                        decors.put(position, new NestWasp(game,position));
                        break;
                    case NestHornet:
                        decors.put(position, new NestHornet(game,position));
                        break;
                    case Apple: {
                        Decor grass = new Grass(position);
                        grass.setBonus(new EnergyBoost(position, grass));
                        decors.put(position, grass);
                        break;
                    }
                    case PoisonedApple: {
                        Decor grass = new Grass(position);
                        grass.setBonus(new PoisonedApple(position, grass));
                        decors.put(position, grass);
                        break;
                    }
                    case Carrots: {
                        Decor grass = new Grass(position);
                        grass.setBonus(new Carrot(position, grass));
                        decors.put(position, grass);
                        break;
                    }

                    // Add more cases as needed
                    default:
                        throw new RuntimeException("EntityCode " + mapEntity.name() + " not processed");
                }

            }
    }

    @Override
    public int width() {
        return this.width;
    }

    @Override
    public int height() {
        return this.height;
    }

    public Decor get(Position position) {
        return decors.get(position);
    }

    public Collection<Decor> values() {
        return decors.values();
    }


    @Override
    public boolean inside(Position position) {
        return position.x() >= 0 && position.x() < this.width &&
                position.y() >= 0 && position.y() < this.height;
    }


}