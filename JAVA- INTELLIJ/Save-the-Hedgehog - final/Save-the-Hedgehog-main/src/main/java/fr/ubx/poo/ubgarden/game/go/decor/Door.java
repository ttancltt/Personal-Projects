package fr.ubx.poo.ubgarden.game.go.decor;

import fr.ubx.poo.ubgarden.game.Position;
import fr.ubx.poo.ubgarden.game.go.personage.Gardener;

public class Door extends Decor {
    private boolean opened;
    private boolean isNextDoor;

    public Door(Position position, boolean opened, boolean isNextDoor) {
        super(position);
        this.opened = opened;
        this.isNextDoor = isNextDoor;
    }

    public boolean isOpened() {
        return opened;
    }

    public boolean isNextDoor() {
        return isNextDoor;
    }
    public void setOpened(boolean opened) {
        // Chỉ thay đổi nếu trạng thái thực sự khác
        if (this.opened != opened) {
            this.opened = opened;
            setModified(true); // Đánh dấu để sprite cập nhật
        }
    }


    // Ghi đè walkableBy để chỉ cho phép đi qua khi cửa mở
    @Override
    public boolean walkableBy(Gardener gardener) {
        return this.opened; // Chỉ đi được khi cửa mở
    }

    // Ghi đè pickUpBy để xử lý việc đi qua cửa
    @Override
    public void pickUpBy(Gardener gardener) {
        // Hành động chỉ xảy ra khi Gardener *đi vào* ô cửa đã mở
        if (this.opened) {
            int currentLevel = getPosition().level();
            int targetLevel = isNextDoor ? currentLevel + 1 : currentLevel - 1;

            // Kiểm tra xem level mục tiêu có hợp lệ không (ví dụ: không nhỏ hơn 1)
            // (Việc kiểm tra level tối đa sẽ thực hiện trong GameEngine)
            if (targetLevel >= 1) {
                System.out.println("Gardener entering door to level " + targetLevel);
                gardener.requestSwitchLevel(targetLevel); // Yêu cầu chuyển level
            } else {
                System.out.println("Cannot go to level " + targetLevel + " (invalid level).");
            }
        }
        // Không gọi super.pickUpBy(gardener) vì cửa không có bonus
    }
}
