package fr.ubx.poo.ubgarden.game.view;

import fr.ubx.poo.ubgarden.game.Direction;
import fr.ubx.poo.ubgarden.game.go.personage.Hornet;
import javafx.scene.layout.Pane;
import javafx.scene.image.Image;

public class SpriteHornet extends Sprite {

    public SpriteHornet(Pane layer, Hornet hornet) {
        super(layer, null, hornet); // Image sẽ được set bởi updateImage
        updateImage();
    }

    @Override
    public void updateImage() {
        Hornet hornet = (Hornet) getGameObject();
        if (hornet.isDeleted()) { // Không cập nhật nếu đã bị xóa
            // Cân nhắc remove() sprite ở đây nếu đối tượng đã bị xóa trước khi cleanupSprites chạy
            return;
        }
        Image image = getImage(hornet.getDirection());
        // TODO: Nếu muốn thay đổi sprite khi Hornet bị thương, thêm logic ở đây
        // Ví dụ: if (hornet.getHealth() < MAX_HEALTH_HORNET) { image = getInjuredImage(hornet.getDirection()); }
        setImage(image);
    }

    private Image getImage(Direction direction) {
        // Giả sử bạn có ảnh cho Hornet trong ImageResource và ImageResourceFactory
        return ImageResourceFactory.getInstance().getHornet(direction);
    }
}
