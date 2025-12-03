package fr.ubx.poo.ubgarden.game.go.personage;

import fr.ubx.poo.ubgarden.game.*;
import fr.ubx.poo.ubgarden.game.go.GameObject;
import fr.ubx.poo.ubgarden.game.go.Movable;
import fr.ubx.poo.ubgarden.game.go.decor.*;
import fr.ubx.poo.ubgarden.game.go.personage.Gardener;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class Wasp extends GameObject implements Movable {
    private Direction direction;
    private int moveTimerCount = 0;
    private final int moveFrequency;
    private int stepsInCurrentDirection = 0; // Đếm số bước đã đi thẳng
    private final int MAX_STEPS_BEFORE_TURN = 3; // Số bước tối đa trước khi cố gắng rẽ

    public Wasp(Game game, Position position) {
        super(game, position);
        this.direction = Direction.random();
        this.moveFrequency = game.configuration().waspMoveFrequency();
    }

    public Direction getDirection() {
        return direction;
    }

    @Override
    public void update(long now) {

        super.update(now);
        if (isDeleted()) return;
        moveTimerCount++;
        if (moveTimerCount >= moveFrequency) {
            moveTimerCount = 0;
            attemptSmartMove();
        }
    }
    private void attemptSmartMove() {
        // Ưu tiên 1: Nếu đã đi đủ MAX_STEPS_BEFORE_TURN, cố gắng rẽ
        if (stepsInCurrentDirection >= MAX_STEPS_BEFORE_TURN) {
            Direction turnDirection = findNewTurnDirection(); // Tìm hướng rẽ (trái, phải, hoặc tiến)
            if (turnDirection != null) {
                // System.out.println(this.getClass().getSimpleName() + " at " + getPosition() + " turning to " + turnDirection + " after " + stepsInCurrentDirection + " steps."); // DEBUG
                this.direction = turnDirection;
                move(this.direction);
                stepsInCurrentDirection = 1; // Reset bộ đếm bước cho hướng mới (đã đi 1 bước)
                return; // Đã di chuyển và rẽ
            }
        }

        // Ưu tiên 2: Thử di chuyển theo hướng hiện tại nếu chưa đến lúc phải rẽ HOẶC nếu không rẽ được
        if (canMove(this.direction)) {
            move(this.direction);
            stepsInCurrentDirection++; // Tăng số bước đã đi theo hướng này
            // System.out.println(this.getClass().getSimpleName() + " at " + getPosition() + " moved straight. Steps: " + stepsInCurrentDirection); // DEBUG
        } else {
            // Ưu tiên 3: Nếu không thể đi thẳng (bị chặn), tìm một hướng ngẫu nhiên mới (ưu tiên không quay đầu)
            // System.out.println(this.getClass().getSimpleName() + " at " + getPosition() + " blocked straight, finding new random direction."); // DEBUG
            Direction newValidDirection = findNewRandomValidDirectionWhenBlocked(); // Đổi tên để rõ ràng hơn
            if (newValidDirection != null) {
                this.direction = newValidDirection;
                move(this.direction);
                stepsInCurrentDirection = 1; // Reset bộ đếm bước cho hướng mới
            } else {
                // Bị kẹt hoàn toàn, chỉ đổi hướng sprite ngẫu nhiên
                this.direction = Direction.random();
                setModified(true);
                stepsInCurrentDirection = 0; // Reset vì không di chuyển
                // System.out.println(this.getClass().getSimpleName() + " at " + getPosition() + " is completely stuck, randomly facing " + this.direction); // DEBUG
            }
        }
    }

    // Hàm này được gọi khi đã đi đủ MAX_STEPS_BEFORE_TURN
    private Direction findNewTurnDirection() {
        List<Direction> turnOptions = new ArrayList<>();
        // Các lựa chọn ưu tiên: đi thẳng tiếp, rẽ trái, rẽ phải
        turnOptions.add(this.direction);          // 1. Tiếp tục đi thẳng
        turnOptions.add(this.direction.turnLeft()); // 2. Rẽ trái
        turnOptions.add(this.direction.turnRight());// 3. Rẽ phải

        Collections.shuffle(turnOptions); // Xáo trộn để việc chọn (thẳng, trái, phải) là ngẫu nhiên

        for (Direction newDir : turnOptions) {
            if (canMove(newDir)) {
                return newDir;
            }
        }
        // Nếu không thể đi thẳng, trái, hoặc phải, không làm gì ở đây,
        // logic attemptSmartMove sẽ xử lý việc bị chặn.
        return null;
    }

    // Đổi tên hàm cũ để rõ ràng hơn: đây là hàm tìm hướng khi bị chặn đường đi thẳng
    private Direction findNewRandomValidDirectionWhenBlocked() {
        List<Direction> allDirections = new ArrayList<>(Arrays.asList(Direction.values()));
        Collections.shuffle(allDirections);

        Direction oppositeDirection = this.direction.opposite();
        Direction preferredNewDirection = null;

        // 1. Ưu tiên thử các hướng không phải là hướng đối diện
        for (Direction newDir : allDirections) {
            if (newDir != oppositeDirection) {
                if (canMove(newDir)) {
                    preferredNewDirection = newDir;
                    break;
                }
            }
        }

        if (preferredNewDirection != null) {
            return preferredNewDirection;
        }

        // 2. Nếu không có lựa chọn nào khác, kiểm tra xem có thể đi lùi không
        if (canMove(oppositeDirection)) {
            return oppositeDirection;
        }

        return null; // Bị kẹt hoàn toàn
    }



    @Override
    public boolean canMove(Direction direction) {
        Position nextPos = direction.nextPosition(getPosition());
        Map currentMap = game.world().getGrid(getPosition().level());

        if (currentMap == null || !currentMap.inside(nextPos)) {
            return false; // Outside map or map doesn't exist
        }

        Decor decorAtNextPos = currentMap.get(nextPos);
        if (decorAtNextPos == null) {
            return false; // Should not happen on a valid map
        }

        // Wasps cannot go through Trees, Nests, or Doors (open or closed)
        return !(decorAtNextPos instanceof Tree) &&
                !(decorAtNextPos instanceof NestWasp) &&
                !(decorAtNextPos instanceof NestHornet) &&
                !(decorAtNextPos instanceof Door) &&
                !(decorAtNextPos instanceof Hedgehog);
    }

    @Override
    public Position move(Direction direction) {
        Position newPosition = direction.nextPosition(getPosition());
        setPosition(newPosition);
        return newPosition;
    }

    public void sting(Gardener gardener) {
        System.out.println("Wasp stings gardener at " + gardener.getPosition());
        gardener.stungByWasp(); // Gardener takes damage
        this.remove(); // Wasp dies after stinging
        setModified(true);
    }

    // Called when wasp hits a bomb or is otherwise destroyed
    public void die() {
        System.out.println("Wasp at " + getPosition() + " dies.");
        this.remove();
        setModified(true);
    }
}