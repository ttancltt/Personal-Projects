package fr.ubx.poo.ubgarden.game.go.decor;

import fr.ubx.poo.ubgarden.game.Configuration;
import fr.ubx.poo.ubgarden.game.Game;
import fr.ubx.poo.ubgarden.game.Position;
// Import Wasp
import fr.ubx.poo.ubgarden.game.go.personage.Wasp;

import fr.ubx.poo.ubgarden.game.engine.Timer;

public class NestWasp extends Decor {
    private Timer spawnTimer;
    private Game gameInstanceRef;

    public NestWasp(Position position) {
        super(position); // GameObject constructor sẽ gán game nếu được truyền vào
        // Giả sử game object được truyền vào NestWasp constructor hoặc lấy từ global
        // Nếu không, bạn cần một cách để truy cập Configuration
        // Tạm thời hardcode, tốt hơn là lấy từ game.configuration().waspSpawnInterval()

    }

    // Cần truyền Game vào để NestWasp có thể truy cập Configuration và danh sách ong
    // Cách tốt hơn là GameObject constructor nhận Game, và NestWasp gọi super(game, position)
    // Giả sử GameObject đã có tham chiếu 'game':
    public NestWasp(Game game, Position position) {
        super(game, position); // Assuming GameObject constructor takes Game
        this.gameInstanceRef = game; // Or just use this.game if inherited
        if (this.game != null) { // Check if game reference is available
            this.spawnTimer = new Timer(this.game.configuration().waspSpawnIntervalMillis());
            this.spawnTimer.start();
        } else {
            System.err.println("Warning: NestWasp created without Game reference, cannot spawn wasps.");
        }
    }
    @Override
    public void update(long now) {
        super.update(now);
        if (spawnTimer != null && this.game != null) { // Use this.game
            spawnTimer.update(now);
            if (!spawnTimer.isRunning()) {
                // Spawn Wasp
                System.out.println("Nest at " + getPosition() + " spawns a wasp.");
                this.game.spawnWasp(getPosition()); // Game handles adding to list and sprite creation later

                // Spawn Random Bomb
                System.out.println("Nest at " + getPosition() + " triggers random bomb spawn.");
                this.game.spawnRandomBomb(getPosition().level());

                spawnTimer.start(); // Restart timer for next spawn
            }
        }
    }
}