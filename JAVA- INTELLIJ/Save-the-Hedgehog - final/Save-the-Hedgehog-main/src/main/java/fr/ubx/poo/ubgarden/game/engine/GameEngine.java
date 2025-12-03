package fr.ubx.poo.ubgarden.game.engine;

// ... các import khác ...
import fr.ubx.poo.ubgarden.game.*; // Import tất cả trong game
import fr.ubx.poo.ubgarden.game.go.bonus.Bonus;
import fr.ubx.poo.ubgarden.game.go.bonus.InsecticideBomb;
import fr.ubx.poo.ubgarden.game.go.decor.Decor;
import fr.ubx.poo.ubgarden.game.go.decor.Door;
import fr.ubx.poo.ubgarden.game.go.decor.Hedgehog;
import fr.ubx.poo.ubgarden.game.go.personage.Gardener;
import fr.ubx.poo.ubgarden.game.go.personage.Hornet;
import fr.ubx.poo.ubgarden.game.go.personage.Wasp;
import fr.ubx.poo.ubgarden.game.view.*;
import fr.ubx.poo.ubgarden.game.Map;

import java.util.*;

import javafx.animation.AnimationTimer;
import javafx.application.Platform;
import javafx.scene.Group;
import javafx.scene.Scene;
import javafx.scene.layout.Pane;
import javafx.scene.layout.StackPane;
import javafx.scene.paint.Color;
import javafx.scene.text.Font;
import javafx.scene.text.Text;
import javafx.scene.text.TextAlignment;
import javafx.scene.layout.HBox;
import javafx.scene.image.ImageView;
import javafx.scene.image.Image;
import javafx.scene.effect.DropShadow;
import javafx.scene.effect.Effect;


public final class GameEngine {

    private static AnimationTimer gameLoop;
    private final Game game;
    private final Gardener gardener;
    // Sử dụng List thay vì Set để có thể chứa SpriteGardener riêng
    private final List<Sprite> sprites = new LinkedList<>();
    private final Set<Sprite> cleanUpSprites = new HashSet<>(); // Giữ lại Set để xóa hiệu quả
    private boolean gameRunning = true;

    private final Scene scene;
    private StatusBar statusBar;
    private final Pane rootPane = new Pane();
    private final Group root = new Group();
    private Pane layer = new Pane(); // layer có thể thay đổi khi chuyển level
    private Input input;
    private SpriteGardener spriteGardener; // Giữ tham chiếu đến sprite của Gardener
    private HBox statusBarHBox; // Store the HBox for the status bar to relocate it


    public GameEngine(Game game, Scene scene) {
        this.game = game;
        this.scene = scene;
        this.gardener = game.getGardener();
        // Không gọi initialize() ở đây nữa, gọi trong buildAndSetGameLoop trước khi start
        buildAndSetGameLoop();
    }
    void buildAndSetGameLoop() {
        gameLoop = new AnimationTimer() {
            long lastUpdateTime = 0;

            public void handle(long now) {
                processInput();

                if (gameRunning) {
                    checkAndHandleSwitchLevel();

                    if (gameRunning && !gardener.isSwitchLevelRequested()) {
                        gardener.update(now);
                        Map currentMap = game.world().getGrid();
                        if (currentMap != null) {
                            currentMap.values().forEach(decor -> decor.update(now)); // NestWasp.update() is called here
                        }
                        game.updateDynamicEntities(now); // Updates positions, removes dead ones from game list
                        //    (e.g., wasps spawned by Nests in their update)
                        for (Wasp wasp : game.getActiveWasps()) {
                            boolean spriteExists = sprites.stream().anyMatch(s -> s.getGameObject() == wasp);
                            // ADD THIS CHECK: ensure wasp is on the current level
                            if (!spriteExists && wasp.getPosition().level() == game.world().currentLevel()) {
                                sprites.add(SpriteFactory.create(layer, wasp));
                                wasp.setModified(true); // Ensure initial render
                            }
                        }

                        for (Hornet hornet : game.getActiveHornets()) {
                            boolean spriteExists = sprites.stream().anyMatch(s -> s.getGameObject() == hornet);
                            if (!spriteExists) {
                                System.out.println("GameEngine: Creating sprite for new Hornet at " + hornet.getPosition()); // LOG
                                sprites.add(SpriteFactory.create(layer, hornet));
                                hornet.setModified(true); // Đảm bảo render lần đầu
                            }
                        }
                        // Phải lấy currentMap ở đây một lần nữa nếu nó có thể thay đổi (ví dụ sau khi chuyển level)
                        // Hoặc đảm bảo currentMap đã được lấy ở đầu handle() và vẫn còn hợp lệ.
                        Map currentMapForBonusCheck = game.world().getGrid(); // Lấy map hiện tại
                        if (currentMapForBonusCheck != null) {
                            for (Decor decor : currentMapForBonusCheck.values()) {
                                Bonus bonus = decor.getBonus();
                                // Kiểm tra bonus có tồn tại, chưa bị xóa, và chưa có sprite không
                                if (bonus != null && !bonus.isDeleted()) {
                                    boolean spriteExistsForBonus = sprites.stream()
                                            .anyMatch(s -> s.getGameObject() == bonus);
                                    if (!spriteExistsForBonus) {
                                        System.out.println("GameEngine: Creating sprite for new bonus " + bonus.getClass().getSimpleName() + " at " + bonus.getPosition()); // LOG
                                        sprites.add(SpriteFactory.create(layer, bonus));
                                        bonus.setModified(true); // Đảm bảo render lần đầu cho sprite bonus mới
                                    }
                                }
                            }
                        }
                        // Similar logic for other future dynamic entities if any
                        checkCollisions();
                        checkGameOutcome(); // checkGameOutcome should run AFTER collisions that might end game
                    }
                    cleanupSprites(false);
                    render();
                    if (statusBar != null) statusBar.update(game);
                }
            }
        };
        initializeGameView();
    }


    private void checkCollisions() {
        if (!gameRunning) return;

        Gardener gardener = game.getGardener();
        Map currentMap = game.world().getGrid();
        if (currentMap == null) return;

        // Iterate over a copy of wasps if modification occurs during iteration
        List<Wasp> waspsToCheck = new ArrayList<>(game.getActiveWasps());

        for (Wasp wasp : waspsToCheck) {
            if (wasp.isDeleted()) continue;

            // Wasp vs Gardener
            if (wasp.getPosition().equals(gardener.getPosition())) {
                if (gardener.getInsecticideBombCount() > 0) {
                    gardener.useBomb();
                    wasp.die();
                    System.out.println("Gardener auto-used bomb on wasp at " + wasp.getPosition());
                } else {
                    wasp.sting(gardener); // Gardener gets stung, wasp dies (sting() calls remove())
                }
                continue; // Wasp is dealt with, move to next wasp
            }

            // Wasp vs Bomb on Ground
            Decor decorAtWaspPos = currentMap.get(wasp.getPosition());
            if (decorAtWaspPos != null && decorAtWaspPos.getBonus() instanceof InsecticideBomb) {
                System.out.println("Wasp at " + wasp.getPosition() + " flew into a bomb!");
                wasp.die(); // Wasp dies
                decorAtWaspPos.getBonus().remove(); // Bomb is used up
                decorAtWaspPos.setBonus(null);
                decorAtWaspPos.setModified(true); // Decor needs redraw (bonus removed)
                // Wasp is now marked deleted.
            }

        }

        // --- HORNET COLLISIONS --- (THÊM MỚI)
        List<Hornet> hornetsToCheck = new ArrayList<>(game.getActiveHornets());
        for (Hornet hornet : hornetsToCheck) {
            if (hornet.isDeleted()) continue;

            // Hornet vs Gardener
            if (hornet.getPosition().equals(gardener.getPosition())) {
                if (gardener.getInsecticideBombCount() > 0 && hornet.canSting()) { // Chỉ dùng bom nếu Hornet còn khả năng đốt
                    gardener.useBomb();
                    hornet.hitByBomb(); // Hornet bị thương bởi bom của gardener
                    System.out.println("Gardener auto-used bomb on hornet at " + hornet.getPosition() + ". Hornet health: " + hornet.getHealth());
                } else if (hornet.canSting() ) { // Nếu không có bom hoặc hornet hết khả năng đốt thì mới bị đốt
                    hornet.sting(gardener); // Gardener bị đốt, Hornet có thể chết nếu hết lượt đốt
                }
                // Hornet không nhất thiết chết ngay, nó có thể tiếp tục di chuyển nếu còn máu/lượt đốt
                // Việc hornet.remove() sẽ được xử lý trong sting() hoặc hitByBomb() -> die()
                continue; // Đã xử lý va chạm với Gardener, chuyển sang Hornet tiếp theo
            }

            // Hornet vs Bomb on Ground
            Decor decorAtHornetPos = currentMap.get(hornet.getPosition());
            if (decorAtHornetPos != null && decorAtHornetPos.getBonus() instanceof InsecticideBomb) {
                System.out.println("Hornet at " + hornet.getPosition() + " flew into a bomb on ground!");
                hornet.hitByBomb(); // Hornet bị thương
                // Bom được sử dụng hết
                decorAtHornetPos.getBonus().remove();
                decorAtHornetPos.setBonus(null);
                decorAtHornetPos.setModified(true); // Decor cần vẽ lại
                // Hornet không chết ngay, nó sẽ bị xóa nếu health <= 0 bên trong hitByBomb()
            }
        }
    }
    // ... getRoot() không đổi ...
    public Pane getRoot() {
        return rootPane;
    }

    // Phương thức tạo Sprite cho một level cụ thể
    private void initializeSpritesForLevel(int levelNumber) {
        // Xóa các sprite cũ khỏi màn hình và danh sách
        cleanupSprites(true); // true = xóa tất cả

        // Lấy map của level mới
        Map currentMap = game.world().getGrid(levelNumber);
        if (currentMap == null) {
            System.err.println("Error: Could not find map for level " + levelNumber);
            // Có thể dừng game hoặc quay lại level trước?
            gameRunning = false; // Dừng game nếu không tìm thấy level
            showMessage("Error: Level " + levelNumber + " not found!", Color.RED);
            if(gameLoop != null) gameLoop.stop();
            return;
        }

        // Cập nhật kích thước layer dựa trên map mới
        int width = currentMap.width();
        int height = currentMap.height();
        int sceneWidth = width * ImageResource.size;
        int sceneHeight = height * ImageResource.size;
        layer.setPrefSize(sceneWidth, sceneHeight); // Cập nhật kích thước layer hiện tại

        // Tạo sprite cho decor và bonus
        for (Decor decor : currentMap.values()) {
            sprites.add(SpriteFactory.create(layer, decor));
            decor.setModified(true); // Đảm bảo vẽ lần đầu
            Bonus bonus = decor.getBonus();
            if (bonus != null) {
                sprites.add(SpriteFactory.create(layer, bonus));
                bonus.setModified(true); // Đảm bảo vẽ lần đầu
            }
        }

        // Tạo hoặc cập nhật SpriteGardener
        spriteGardener = new SpriteGardener(layer, gardener);
        sprites.add(spriteGardener); // Thêm vào danh sách quản lý

        gardener.setModified(true);


        System.out.println("Initialized sprites for level " + levelNumber + ". Total sprites: " + sprites.size());

        // Cập nhật thanh trạng thái để hiển thị đúng level
        if (statusBar != null) {
            statusBar.update(game); // Cập nhật ngay lập tức
        }

        // Cập nhật kích thước cửa sổ nếu cần
        resizeScene(sceneWidth, sceneHeight);
    }


    private void initializeGameView() {
        int currentLevel = game.world().currentLevel();
        Map map = game.world().getGrid(currentLevel);
        if (map == null) throw new IllegalStateException("Initial level " + currentLevel + " not found in world.");

        int width = map.width();
        int height = map.height();
        int sceneWidth = width * ImageResource.size;
        int sceneHeight = height * ImageResource.size;
        gardener.setModified(true);
        System.out.println("Gardener marked as modified for initial render at: " + gardener.getPosition());
        scene.getStylesheets().add(Objects.requireNonNull(getClass().getResource("/css/application.css")).toExternalForm());
        input = new Input(scene);

        // Reset root và layer
        root.getChildren().clear();
        layer = new Pane(); // Tạo layer mới
        root.getChildren().add(layer);

        // --- Status Bar HBox Creation (moved logic from StatusBar constructor here for direct control) ---
        HBox levelDisplay = new HBox(); // Renamed from 'level' to avoid conflict with level number
        levelDisplay.getStyleClass().add("level");
        // Initial digit, will be updated by statusBar.update()
        levelDisplay.getChildren().add(new ImageView(ImageResourceFactory.getInstance().getDigit(currentLevel % 10))); // Example

        DropShadow ds = new DropShadow();
        ds.setRadius(5.0); ds.setOffsetX(3.0); ds.setOffsetY(3.0); ds.setColor(Color.color(0.5f, 0.5f, 0.5f));

        Text energyText = new Text(); // Will be updated by statusBar.update()
        Text diseaseText = new Text(); // Will be updated by statusBar.update()
        Text insecticideText = new Text(); // Will be updated by statusBar.update()

        // Create the actual StatusBar object (for logic and text updates)
        // The StatusBar class will now mainly be responsible for updating the Text nodes.
        // We remove the HBox creation from StatusBar's constructor.
        statusBar = new StatusBar(levelDisplay, energyText, diseaseText, insecticideText); // Modified StatusBar constructor

        HBox statusTextGroup = new HBox();
        statusTextGroup.getStyleClass().add("status");
        HBox insecticideStatus = createStatusGroup(ImageResourceFactory.getInstance().get(ImageResource.INSECTICIDE), insecticideText, ds);
        HBox energyStatus = createStatusGroup(ImageResourceFactory.getInstance().get(ImageResource.ENERGY), energyText, ds);
        HBox diseaseLevelStatus = createStatusGroup(ImageResourceFactory.getInstance().get(ImageResource.POISONED_APPLE), diseaseText, ds);

        statusTextGroup.setSpacing(40.0);
        statusTextGroup.getChildren().addAll(diseaseLevelStatus, insecticideStatus, energyStatus);

        this.statusBarHBox = new HBox(); // This is the main HBox for the entire status bar
        this.statusBarHBox.getChildren().addAll(levelDisplay, statusTextGroup);
        this.statusBarHBox.getStyleClass().add("statusBar");
        this.statusBarHBox.setPrefSize(sceneWidth, StatusBar.height);
        this.statusBarHBox.relocate(0, sceneHeight);
        // --- End of Status Bar HBox Creation ---

        root.getChildren().add(this.statusBarHBox); // Add the status bar HBox to the root group

        rootPane.getChildren().clear();
        rootPane.setPrefSize(sceneWidth, sceneHeight + StatusBar.height);
        rootPane.getChildren().add(root);

        // Khởi tạo sprites cho level đầu tiên
        initializeSpritesForLevel(currentLevel);


        // Đảm bảo gardener được vẽ đúng vị trí ban đầu
        gardener.setModified(true);

        // Cập nhật status bar lần đầu
        statusBar.update(game);

        // Thay đổi kích thước cửa sổ
        resizeScene(sceneWidth, sceneHeight);
    }



    // *** PHƯƠNG THỨC MỚI ĐỂ XỬ LÝ CHUYỂN LEVEL ***
    private void checkAndHandleSwitchLevel() {
        if (gardener.isSwitchLevelRequested()) {
            int targetLevelNum = gardener.getTargetLevel();
            System.out.println("GameEngine: Detected switch level request to " + targetLevelNum);

            // Lấy map của level mới để kiểm tra xem nó có tồn tại không
            Map targetMap = game.world().getGrid(targetLevelNum);
            if (targetMap == null) {
                System.err.println("Error: Cannot switch to level " + targetLevelNum + ". Level does not exist in world.");
                gardener.clearSwitchLevelRequest(); // Hủy yêu cầu nếu level không hợp lệ
                return; // Không làm gì cả
            }


            // Tìm vị trí cửa tương ứng ở level mới để Gardener xuất hiện
            Position newGardenerPos = findSpawnPositionForLevel(targetLevelNum);
            if (newGardenerPos == null) {
                System.err.println("Error: Cannot find spawn door '<' or '>' in target level " + targetLevelNum);
                gardener.clearSwitchLevelRequest(); // Hủy yêu cầu
                return; // Không làm gì cả
            }

            // Thực hiện chuyển đổi
            System.out.println("Switching to level " + targetLevelNum + " at position " + newGardenerPos);

            // 1. Cập nhật level hiện tại trong World
            game.world().setCurrentLevel(targetLevelNum);

            // 2. Cập nhật vị trí Gardener
            gardener.setPosition(newGardenerPos);
            gardener.setModified(true); // Đảm bảo gardener được vẽ lại ở vị trí mới

            // 3. Khởi tạo lại sprites cho level mới
            initializeSpritesForLevel(targetLevelNum); // Hàm này sẽ xóa sprite cũ và tạo sprite mới

            // 4. Xóa yêu cầu chuyển level khỏi Gardener
            gardener.clearSwitchLevelRequest();

            // 5. Đảm bảo scene có kích thước đúng
            int newWidth = targetMap.width();
            int newHeight = targetMap.height();
            int newSceneWidth = newWidth * ImageResource.size;
            int newSceneHeight = newHeight * ImageResource.size;

            // Update layer size
            layer.setPrefSize(newSceneWidth, newSceneHeight);

            // Update StatusBar HBox position and width
            if (statusBarHBox != null) {
                statusBarHBox.relocate(0, newSceneHeight);
                statusBarHBox.setPrefWidth(newSceneWidth); // Ensure it spans the new width
            }

            // Update rootPane size (this is crucial BEFORE resizing the stage)
            rootPane.setPrefSize(newSceneWidth, newSceneHeight + StatusBar.height);
            // resizeScene() will call stage.sizeToScene()
            resizeScene(newSceneWidth, newSceneHeight);

            System.out.println("Level switch to " + targetLevelNum + " completed.");
            statusBar.update(game);

        }
    }

    // *** PHƯƠNG THỨC MỚI ĐỂ TÌM VỊ TRÍ SPAWN ***
    private Position findSpawnPositionForLevel(int targetLevelNum) {
        Map targetMap = game.world().getGrid(targetLevelNum);
        if (targetMap == null) return null;

        int currentLevelNum = gardener.getPosition().level(); // Level hiện tại của gardener *trước khi* chuyển
        boolean comingFromLowerLevel = targetLevelNum > currentLevelNum; // True nếu đi lên level (vd 1 -> 2)

        for (Decor decor : targetMap.values()) {
            if (decor instanceof Door) {
                Door door = (Door) decor;
                // Nếu đi lên level (1->2), tìm cửa lùi '<' (isNextDoor=false) ở level 2
                // Nếu đi xuống level (2->1), tìm cửa tới '>' (isNextDoor=true) ở level 1
                if (comingFromLowerLevel && !door.isNextDoor()) { // Tìm cửa lùi '<'
                    return door.getPosition();
                } else if (!comingFromLowerLevel && door.isNextDoor()) { // Tìm cửa tới '>' hoặc 'D' (vị trí tương ứng)
                    // Cần đảm bảo cửa này đã mở nếu là logic game, nhưng ở đây chỉ cần vị trí
                    return door.getPosition();
                }
            }
        }
        // Nếu không tìm thấy cửa phù hợp
        System.err.println("Could not find matching spawn door in level " + targetLevelNum + " (coming from " + currentLevelNum + ")");
        return null;
    }


    // ... processInput, showMessage không đổi ...
    private void processInput() {
        if (input.isExit()) {
            gameLoop.stop();
            Platform.exit();
            System.exit(0);
        }
        // Chỉ xử lý di chuyển nếu game đang chạy
        if (gameRunning) {
            if (input.isMoveDown()) {
                gardener.requestMove(Direction.DOWN);
            } else if (input.isMoveLeft()) {
                gardener.requestMove(Direction.LEFT);
            } else if (input.isMoveRight()) {
                gardener.requestMove(Direction.RIGHT);
            } else if (input.isMoveUp()) {
                gardener.requestMove(Direction.UP);
            }
        }
        input.clear(); // Luôn xóa input sau khi xử lý
    }

    private void showMessage(String msg, Color color) {
        Text message = new Text(msg);
        message.setTextAlignment(TextAlignment.CENTER);
        message.setFont(new Font(60));
        message.setFill(color);

        StackPane pane = new StackPane(message);
        // Đảm bảo pane này phủ toàn bộ rootPane
        pane.setPrefSize(rootPane.getWidth(), rootPane.getHeight());
        // Xóa nội dung cũ (có thể là layer game) và thêm message pane
        rootPane.getChildren().clear();
        rootPane.getChildren().add(pane);

        // Dừng game loop chính khi hiển thị message toàn màn hình
        if (gameLoop != null) {
            gameLoop.stop();
        }

        // Tạo một loop riêng chỉ để bắt input thoát khi message đang hiển thị
        AnimationTimer messageLoop = new AnimationTimer() {
            public void handle(long now) {
                // Chỉ xử lý thoát khỏi message loop
                if (input.isExit()) {
                    this.stop(); // Dừng message loop
                    Platform.exit();
                    System.exit(0);
                }
                input.clear(); // Xóa input khác
            }
        };
        messageLoop.start();
    }

    private void update(long now) {
        // Chỉ update nếu game đang chạy
        if (!gameRunning) return;

        // Update Gardener trước (xử lý input di chuyển, hồi năng lượng, bệnh tật)
        gardener.update(now);

        // Nếu gardener hết năng lượng sau khi update, kết thúc game
        if (gardener.getEnergy() <= 0) {
            // checkGameOutcome sẽ xử lý việc này, không cần kiểm tra riêng ở đây
            return; // Dừng update frame này nếu hết năng lượng
        }

        // Update các đối tượng khác trên map hiện tại
        Map currentMap = game.world().getGrid(); // Lấy map hiện tại
        if (currentMap != null) {
            currentMap.values().forEach(decor -> decor.update(now));
            // Thêm update cho các đối tượng khác (ong,...) nếu có
        }

    }

    private void checkGameOutcome() {
        if (!gameRunning) return; // Không kiểm tra nếu game đã kết thúc

        // Kiểm tra thua (hết năng lượng)
        if (gardener.getEnergy() <= 0) {
            System.out.println("Game Over - Energy Depleted!");
            gameRunning = false;
            showMessage("Perdu!", Color.RED); // showMessage sẽ dừng game loop
            return; // Thoát sớm
        }

        // Kiểm tra thắng (đến vị trí Hedgehog)
        Position gardenerPos = gardener.getPosition();
        Map currentMap = game.world().getGrid(); // Lấy map hiện tại
        if (currentMap != null) {
            Decor decorAtGardenerPos = currentMap.get(gardenerPos);
            // Kiểm tra xem decor tại vị trí gardener có phải là Hedgehog không
            // Lưu ý: get() trả về Decor, không phải Hedgehog trực tiếp nếu nó nằm trên nền
            // Cần kiểm tra xem decor đó có phải là Hedgehog không
            if (decorAtGardenerPos instanceof Hedgehog) {
                System.out.println("Victory - Hedgehog Found!");
                gameRunning = false;
                showMessage("Victoire!", Color.GREEN); // showMessage sẽ dừng game loop
            }
        }
    }


    // Sửa đổi cleanupSprites để có tùy chọn xóa tất cả
    public void cleanupSprites(boolean removeAll) {
        if (removeAll) {
            // Xóa tất cả sprite khỏi layer và danh sách
            sprites.forEach(Sprite::remove); // Gọi remove() để xóa khỏi layer
            sprites.clear(); // Xóa khỏi danh sách quản lý
            // Quan trọng: đặt lại spriteGardener nếu xóa tất cả
            spriteGardener = null;
        } else {
            // Chỉ xóa những sprite có đối tượng bị đánh dấu deleted
            cleanUpSprites.clear(); // Xóa danh sách cần dọn dẹp cũ
            // Dùng iterator để tránh ConcurrentModificationException
            Iterator<Sprite> iterator = sprites.iterator();
            while (iterator.hasNext()) {
                Sprite sprite = iterator.next();
                if (sprite.getGameObject().isDeleted()) {
                    sprite.remove(); // Xóa khỏi layer
                    iterator.remove(); // Xóa khỏi danh sách sprites
                    // Không cần thêm vào cleanUpSprites nữa
                }
            }
        }
    }


    private void render() {
        // Chỉ render nếu game đang chạy
        if (!gameRunning) return;

        // Duyệt qua tất cả sprite và gọi render của chúng
        // Dùng for-each an toàn vì cleanup đã xử lý xóa
        for (Sprite sprite : sprites) {
            sprite.render(); // Phương thức render của Sprite/SpriteDoor/SpriteGardener sẽ xử lý việc cập nhật hình ảnh và vẽ
        }
    }

    public void start() {
        if (gameLoop == null) {
            System.err.println("Game loop not initialized before starting!");
            return;
        }
        gameRunning = true;
        gameLoop.start();
        // Đảm bảo rootPane có focus để nhận input bàn phím
        Platform.runLater(() -> {
            rootPane.requestFocus();
            System.out.println("Game started and requested focus.");
        });
    }

    private HBox createStatusGroup(Image kind, Text numberText, Effect effect) {
        HBox group = new HBox();
        ImageView img = new ImageView(kind);
        group.setSpacing(3);
        numberText.setEffect(effect);
        numberText.setCache(true);
        numberText.setFill(Color.BLACK);
        numberText.getStyleClass().add("number");
        group.getChildren().addAll(img, numberText);
        return group;
    }

    // resizeScene không đổi
    private void resizeScene(int newMapPixelWidth, int newMapPixelHeight) {
        if (rootPane.getPrefWidth() != newMapPixelWidth ||
                rootPane.getPrefHeight() != (newMapPixelHeight + StatusBar.height)) {
            rootPane.setPrefSize(newMapPixelWidth, newMapPixelHeight + StatusBar.height);
        }
        // The layer's size should also match the new map dimensions
        if (layer != null && (layer.getPrefWidth() != newMapPixelWidth || layer.getPrefHeight() != newMapPixelHeight)) {
            layer.setPrefSize(newMapPixelWidth, newMapPixelHeight);
        }


        Platform.runLater(() -> {
            if (scene.getWindow() != null) {
                scene.getWindow().sizeToScene(); // This makes the window fit rootPane's new prefSize
            }
        });
        System.out.println("resizeScene called. Target map pixels: " + newMapPixelWidth + "x" + newMapPixelHeight +
                ". RootPane target: " + rootPane.getPrefWidth() + "x" + rootPane.getPrefHeight());
    }
}
