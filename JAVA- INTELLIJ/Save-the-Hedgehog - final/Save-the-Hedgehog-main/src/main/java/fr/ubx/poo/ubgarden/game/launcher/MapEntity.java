package fr.ubx.poo.ubgarden.game.launcher;

public enum MapEntity {
    PoisonedApple('-'),
    Apple('+'),
    Carrots('F'), // Thêm Carrot nếu chưa có
    Flowers('O'),
    Grass('G'),
    Land('L'), // Đổi Dirt thành Land cho khớp cấu hình
    Tree('T'),
    Gardener('P'),
    Hedgehog('H'),

    // Cập nhật các loại cửa
    DoorPrevOpened('<'),  // Cửa về level trước, đã mở
    DoorNextOpened('>'),  // Cửa đến level sau, đã mở
    DoorNextClosed('D'),  // Cửa đến level sau, đang đóng

    NestWasp('n'),
    NestHornet('N'),
    InsecticideBomb('B'); // Thêm nếu cần

    private final char code;

    MapEntity(char c) {
        this.code = c;
    }

    public static MapEntity fromCode(char c) {
        for (MapEntity mapEntity : values()) {
            if (mapEntity.code == c)
                return mapEntity;
        }
        // Ném ngoại lệ rõ ràng hơn
        throw new MapException("Invalid map entity code: '" + c + "'");
    }

    public char getCode() {
        return this.code;
    }

    @Override
    public String toString() {
        return Character.toString(code);
    }

    // Phương thức tiện ích để kiểm tra loại cửa
    public boolean isDoor() {
        return this == DoorPrevOpened || this == DoorNextOpened || this == DoorNextClosed;
    }

    public boolean isNextDoor() {
        return this == DoorNextOpened || this == DoorNextClosed;
    }

    public boolean isPrevDoor() {
        return this == DoorPrevOpened;
    }
}