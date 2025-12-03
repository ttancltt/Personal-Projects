package fr.ubx.poo.ubgarden.game.view;



import fr.ubx.poo.ubgarden.game.go.decor.Door; // Import lớp Door
import javafx.scene.image.Image;
import javafx.scene.layout.Pane;

// Lớp SpriteDoor kế thừa từ Sprite
public class SpriteDoor extends Sprite {

    /**
     * Constructor cho SpriteDoor.
     * @param layer Pane chứa sprite.
     * @param door Đối tượng Door cần hiển thị.
     */
    public SpriteDoor(Pane layer, Door door) {
        // Gọi constructor của lớp cha (Sprite)
        // Truyền vào layer, hình ảnh ban đầu (dựa trên trạng thái door), và đối tượng door
        super(layer, getImageForDoor(door), door);
    }

    /**
     * Phương thức tĩnh helper để lấy Image phù hợp dựa trên trạng thái của Door.
     * @param door Đối tượng Door cần kiểm tra.
     * @return Image tương ứng (cửa mở hoặc đóng).
     */
    private static Image getImageForDoor(Door door) {
        // Chọn ImageResource dựa vào door.isOpened()
        ImageResource resource = door.isOpened() ? ImageResource.DOOR_OPENED : ImageResource.DOOR_CLOSED;
        // Lấy đối tượng Image từ factory
        return ImageResourceFactory.getInstance().get(resource);
    }

    /**
     * Ghi đè phương thức updateImage từ lớp Sprite.
     * Phương thức này sẽ được gọi bởi render() khi đối tượng Door được đánh dấu là modified.
     */
    @Override
    public void updateImage() {
        // Lấy đối tượng Door được liên kết với sprite này
        Door door = (Door) getGameObject();
        // Lấy hình ảnh mới dựa trên trạng thái *hiện tại* của Door
        Image newImage = getImageForDoor(door);
        // Gọi phương thức setImage() của lớp cha (Sprite) để cập nhật hình ảnh
        // mà render() sẽ sử dụng để tạo ImageView mới.
        setImage(newImage);
    }
}