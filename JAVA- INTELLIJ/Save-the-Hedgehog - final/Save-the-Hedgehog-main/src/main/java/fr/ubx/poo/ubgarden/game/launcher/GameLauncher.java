package fr.ubx.poo.ubgarden.game.launcher;

import fr.ubx.poo.ubgarden.game.*;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;
import java.util.Properties;

public class GameLauncher {

    private GameLauncher() {
    }

    public static GameLauncher getInstance() {
        return LoadSingleton.INSTANCE;
    }

    private int integerProperty(Properties properties, String name, int defaultValue) {
        try {
            return Integer.parseInt(properties.getProperty(name, "" + defaultValue));
        } catch (NumberFormatException e) {
            System.err.println("Warning: Bad integer format for '" + name + "'. Using default: " + defaultValue);
            return defaultValue;
        }
    }

    private long longProperty(Properties properties, String name, long defaultValue) {
        try {
            return Long.parseLong(properties.getProperty(name, Long.toString(defaultValue)));
        } catch (NumberFormatException e) {
            System.err.println("Warning: Invalid long format for property '" + name + "'. Using default value: " + defaultValue);
            return defaultValue;
        }
    }

    private boolean booleanProperty(Properties properties, String name, boolean defaultValue) {
        String value = properties.getProperty(name);
        if (value == null) return defaultValue;
        return Boolean.parseBoolean(value);
    }

    private Configuration getConfiguration(Properties properties) {
        // TODO: maybe add more config options?
        int waspMoveFrequency = integerProperty(properties, "waspMoveFrequency", 60);
        int hornetMoveFrequency = integerProperty(properties, "hornetMoveFrequency", 30);
        int gardenerEnergy = integerProperty(properties, "gardenerEnergy", 100);
        int energyBoost = integerProperty(properties, "energyBoost", 50);
        long energyRecoverDuration = longProperty(properties, "energyRecoverDuration", 1000L);
        long diseaseDuration = longProperty(properties, "diseaseDuration", 5000);  // missing L suffix

        long waspSpawnIntervalMillis = longProperty(properties, "waspSpawnIntervalMillis", 5000L);
        int waspDamage = integerProperty(properties, "waspDamage", 20);
        int hornetDamage= integerProperty(properties, "hornetDamage", 30);
        long hornetStingCooldownMillis = longProperty(properties, "hornetStingCooldownMillis", 3000L);
        int gardenerBombCapacity = integerProperty(properties, "gardenerBombCapacity", 5);

        return new Configuration(gardenerEnergy, energyBoost, energyRecoverDuration, diseaseDuration,
                waspMoveFrequency, hornetMoveFrequency,
                waspSpawnIntervalMillis, waspDamage,hornetDamage,hornetStingCooldownMillis,  gardenerBombCapacity);
    }

    public Game load(File file) {
        Properties properties = new Properties();
        try (InputStream input = new FileInputStream(file)) {
            properties.load(input);
        } catch (IOException e) {
            throw new MapException("Error reading map file: " + e.getMessage());
        }

        Configuration config = getConfiguration(properties);
        boolean compression = booleanProperty(properties, "compression", false);
        int numLevels = integerProperty(properties, "levels", 1);
        if (numLevels < 1) {
            throw new MapException("Number of levels must be at least 1.");
        }

        World world = new World(numLevels);
        Position gardenerStartPosition = null;

        var parsedMapLevels = new HashMap<Integer, MapLevel>();

        for (int levelNum = 1; levelNum <= numLevels; levelNum++) {
            String levelKey = "level" + levelNum;
            String mapData = properties.getProperty(levelKey);
            if (mapData == null || mapData.trim().isEmpty()) {
                throw new MapException("Missing or empty map data for " + levelKey);
            }
            if (compression) {
                mapData = decompressMapString(mapData);
            }
            MapLevel mapLevel = parseMapString(mapData, levelNum);
            parsedMapLevels.put(levelNum, mapLevel);
        }

        MapLevel mapLevel1 = parsedMapLevels.get(1);
        if (mapLevel1 == null) {
            throw new MapException("MapLevel for level 1 not found after parsing.");
        }

        gardenerStartPosition = findAndSetGardenerPosition(mapLevel1, 1);
        if (gardenerStartPosition == null) {
            throw new MapException("Gardener ('P') not found on level 1.");
        }

        Game game = new Game(world, config, gardenerStartPosition);

        for (int levelNum = 1; levelNum <= numLevels; levelNum++) {
            MapLevel mapLevelData = parsedMapLevels.get(levelNum);
            if (mapLevelData == null) {
                throw new MapException("Parsed MapLevel data not found for level " + levelNum);
            }
            Map levelGrid = new Level(game, levelNum, mapLevelData);
            world.put(levelNum, levelGrid);
        }

        System.out.println("Game loaded successfully from: " + file.getName());
        return game;
    }

    private static String decompressMapString(String compressed) {
        StringBuilder decompressed = new StringBuilder();
        int i = 0;

        while (i < compressed.length()) {
            char currChar = compressed.charAt(i);
            int j = i + 1;

            while (j < compressed.length() && Character.isDigit(compressed.charAt(j))) {
                j++;
            }

            if (j > i + 1) {
                int count = Integer.parseInt(compressed.substring(i + 1, j));
                if (count <= 0) {
                    throw new MapException("Invalid compression: Count must be positive at index " + (i + 1) + " in '" + compressed + "'");
                }

                for (int k = 0; k < count; k++)
                    decompressed.append(currChar);

                i = j;
            } else {
                decompressed.append(currChar);
                i++;
            }
        }
        return decompressed.toString();
    }

    private static MapLevel parseMapString(String mapData, int levelNum) {
        String[] rows = mapData.split("x");
        if (rows.length == 0) {
            throw new MapException("Map data for level " + levelNum + " is empty or invalid.");
        }

        int height = rows.length;
        int width = rows[0].length();
        MapLevel mapLevel = new MapLevel(width, height);

        for (int j = 0; j < height; j++) {
            if (rows[j].length() != width) {
                throw new MapException("Inconsistent row length at row " + (j + 1) + " for level " + levelNum +
                        ". Expected " + width + ", got " + rows[j].length());
            }

            for (int i = 0; i < width; i++) {
                char code = rows[j].charAt(i);
                try {
                    MapEntity entity = MapEntity.fromCode(code);
                    mapLevel.set(i, j, entity);
                } catch (MapException e) {
                    throw new MapException("Invalid character '" + code + "' at (" + i + "," + j +
                            ") on level " + levelNum + ": " + e.getMessage());
                }
            }
        }
        return mapLevel;
    }

    private static Position findAndSetGardenerPosition(MapLevel mapLevel, int levelNum) {
        Position gardenerPos = null;

        for (int i = 0; i < mapLevel.width(); i++) {
            for (int j = 0; j < mapLevel.height(); j++) {
                if (mapLevel.get(i, j) == MapEntity.Gardener) {
                    if (gardenerPos != null) {
                        throw new MapException("Multiple Gardener ('P') definitions found on level " + levelNum);
                    }
                    gardenerPos = new Position(levelNum, i, j);
                    mapLevel.set(i, j, MapEntity.Grass);
                }
            }
        }
        return gardenerPos;
    }

    public Game load() {
        Properties emptyConfig = new Properties();
        Configuration configuration = getConfiguration(emptyConfig);

        MapLevelDefaultStart mapLevelData = new MapLevelDefaultStart();
        Position gardenerPosition = findAndSetGardenerPosition(mapLevelData, 1);
        if (gardenerPosition == null) {
            throw new RuntimeException("Gardener not found in default map!");
        }

        World world = new World(1);
        Game game = new Game(world, configuration, gardenerPosition);

        Map level = new Level(game, 1, mapLevelData);
        world.put(1, level);

        return game;
    }

    private static class LoadSingleton {
        static final GameLauncher INSTANCE = new GameLauncher();
    }
}
