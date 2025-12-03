package fr.ubx.poo.ubgarden.game.go.decor;

import fr.ubx.poo.ubgarden.game.Game;
import fr.ubx.poo.ubgarden.game.Position;
import fr.ubx.poo.ubgarden.game.engine.Timer;

public class NestHornet extends Decor {
    private Timer spawnTimer;
    private final long spawnIntervalMillis = 10000; // 10 giây


    public NestHornet(Game game, Position position) {
        super(game, position);
        if (this.game != null) {
            // Lấy spawn interval từ config nếu muốn, hiện tại là hardcode 10s
            // long interval = game.configuration().hornetSpawnIntervalMillis(); // Cần thêm vào Config
            this.spawnTimer = new Timer(spawnIntervalMillis);
            this.spawnTimer.start();
        } else {
            System.err.println("Warning: NestHornet created without Game reference, cannot spawn hornets.");
        }
    }

    @Override
    public void update(long now) {
        super.update(now);
        if (spawnTimer != null && this.game != null && !isDeleted()) {
            spawnTimer.update(now);
            if (!spawnTimer.isRunning()) {
                System.out.println("NestHornet at " + getPosition() + " spawns a hornet.");
                this.game.spawnHornet(getPosition()); // Game sẽ xử lý việc tạo Hornet

                // Mỗi frelon tạo ra hai bom
                System.out.println("NestHornet at " + getPosition() + " triggers 2 random bomb spawns.");
                this.game.spawnRandomBomb(getPosition().level());
                this.game.spawnRandomBomb(getPosition().level());

                spawnTimer.start(); // Khởi động lại timer
            }
        }
    }
}