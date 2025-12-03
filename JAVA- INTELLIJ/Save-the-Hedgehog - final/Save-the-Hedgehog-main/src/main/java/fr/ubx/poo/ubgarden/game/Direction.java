
package fr.ubx.poo.ubgarden.game;

import java.util.Random;

public enum Direction {
    UP {
        @Override public Position nextPosition(Position pos, int delta) { return new Position(pos.level(), pos.x(), pos.y() - delta); }
        @Override public Direction opposite() { return DOWN; }
        @Override public Direction turnLeft() { return LEFT; }   // Rẽ trái từ UP là LEFT
        @Override public Direction turnRight() { return RIGHT; }  // Rẽ phải từ UP là RIGHT
    },
    RIGHT {
        @Override public Position nextPosition(Position pos, int delta) { return new Position(pos.level(), pos.x() + delta, pos.y()); }
        @Override public Direction opposite() { return LEFT; }
        @Override public Direction turnLeft() { return UP; }     // Rẽ trái từ RIGHT là UP
        @Override public Direction turnRight() { return DOWN; }   // Rẽ phải từ RIGHT là DOWN
    },
    DOWN {
        @Override public Position nextPosition(Position pos, int delta) { return new Position(pos.level(), pos.x(), pos.y() + delta); }
        @Override public Direction opposite() { return UP; }
        @Override public Direction turnLeft() { return RIGHT; }  // Rẽ trái từ DOWN là RIGHT
        @Override public Direction turnRight() { return LEFT; }   // Rẽ phải từ DOWN là LEFT
    },
    LEFT {
        @Override public Position nextPosition(Position pos, int delta) { return new Position(pos.level(), pos.x() - delta, pos.y()); }
        @Override public Direction opposite() { return RIGHT; }
        @Override public Direction turnLeft() { return DOWN; }   // Rẽ trái từ LEFT là DOWN
        @Override public Direction turnRight() { return UP; }     // Rẽ phải từ LEFT là UP
    },
    ;

    private static final Random randomGenerator = new Random();

    public static Direction random() {
        int i = randomGenerator.nextInt(values().length);
        return values()[i];
    }

    public abstract Position nextPosition(Position pos, int delta);
    public abstract Direction opposite();
    public abstract Direction turnLeft();   // THÊM phương thức abstract
    public abstract Direction turnRight();  // THÊM phương thức abstract

    public Position nextPosition(Position pos) {
        return nextPosition(pos, 1);
    }
}