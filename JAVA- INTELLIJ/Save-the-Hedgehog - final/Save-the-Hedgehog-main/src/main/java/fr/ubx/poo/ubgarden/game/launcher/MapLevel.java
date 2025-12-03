package fr.ubx.poo.ubgarden.game.launcher;


public class MapLevel {

    private final int width;
    private final int height;
    private final MapEntity[][] grid;

    // REMOVED: private Position gardenerPosition = null;

    public MapLevel(int width, int height) {
        this.width = width;
        this.height = height;
        this.grid = new MapEntity[height][width];
    }

    public int width() {
        return width;
    }

    public int height() {
        return height;
    }

    public MapEntity get(int i, int j) {
        // Add bounds check for safety
        if (i < 0 || i >= width || j < 0 || j >= height) {
            // Or return null, or throw exception depending on desired strictness
            throw new ArrayIndexOutOfBoundsException("MapLevel get out of bounds: (" + i + "," + j + ") for size (" + width + "," + height + ")");
        }
        return grid[j][i];
    }

    public void set(int i, int j, MapEntity mapEntity) {
        // Add bounds check for safety
        if (i < 0 || i >= width || j < 0 || j >= height) {
            throw new ArrayIndexOutOfBoundsException("MapLevel set out of bounds: (" + i + "," + j + ") for size (" + width + "," + height + ")");
        }
        grid[j][i] = mapEntity;
    }

}
