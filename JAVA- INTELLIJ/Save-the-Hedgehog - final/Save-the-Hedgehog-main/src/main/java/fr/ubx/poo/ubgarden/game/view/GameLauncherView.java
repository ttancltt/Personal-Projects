package fr.ubx.poo.ubgarden.game.view;

import fr.ubx.poo.ubgarden.game.engine.GameEngine;
import fr.ubx.poo.ubgarden.game.Game;
import fr.ubx.poo.ubgarden.game.launcher.GameLauncher;
import fr.ubx.poo.ubgarden.game.launcher.MapException; // Keep this import
import javafx.scene.control.*;
import javafx.scene.control.Alert.AlertType; // Keep this import
import javafx.scene.input.KeyCombination;
import javafx.scene.layout.BorderPane;
import javafx.scene.layout.VBox;
import javafx.scene.text.Text;
import javafx.stage.FileChooser;
import javafx.stage.Stage;

import java.io.File;
import java.util.Objects;

public class GameLauncherView extends BorderPane {
    private final FileChooser fileChooser = new FileChooser();
    private final Stage stage;

    public GameLauncherView(Stage stage) {
        this.stage = stage;
        // Create menu
        MenuBar menuBar = new MenuBar();

        Menu menuGame = new Menu("Game");
        MenuItem loadItem = new MenuItem("Load from file ...");
        MenuItem defaultItem = new MenuItem("Load default configuration");
        MenuItem exitItem = new MenuItem("Exit");
        exitItem.setAccelerator(KeyCombination.keyCombination("Ctrl+Q"));
        menuGame.getItems().addAll(
                loadItem, defaultItem, new SeparatorMenuItem(),
                exitItem);

        Menu menuEditor = new Menu("Editor");
        MenuItem editorOpen = new MenuItem("Open map editor");

        menuEditor.getItems().addAll(
                editorOpen);

        menuBar.getMenus().addAll(menuGame, menuEditor);
        this.setTop(menuBar);

        Text text = new Text("UBGarden 2025");
        text.getStyleClass().add("message");
        VBox scene = new VBox();
        scene.getChildren().add(text);
        scene.getStylesheets().add(Objects.requireNonNull(getClass().getResource("/css/application.css")).toExternalForm());
        scene.getStyleClass().add("message");
        this.setCenter(scene);
        stage.setResizable(false);

        // Load from file
        loadItem.setOnAction(e -> {
            File file = fileChooser.showOpenDialog(stage);
            if (file != null) {
                try {
                    Game game = GameLauncher.getInstance().load(file);
                    if (game != null) {
                        GameEngine engine = new GameEngine(game, stage.getScene());
                        this.setCenter(engine.getRoot());
                        engine.getRoot().requestFocus(); // Ensure engine gets focus
                        engine.start();
                        resizeStage(); // Resize after setting content
                    } else {
                        // Handle cases where load might return null (e.g., parse error handled internally but deemed invalid)
                        showErrorDialog("Loading Error", "Could not load the game from the selected file (loader returned null)."); // Updated message slightly
                    }
                    // *** MERGED CHANGES START ***
                } catch (MapException me) { // Catch specific MapException first
                    System.err.println("Error loading game map/config: " + me.getMessage());
                    me.printStackTrace(); // Log the full stack trace for debugging
                    showErrorDialog("Map Loading Error", "Failed to load game from file:\n" + me.getMessage());
                } catch (RuntimeException re) { // Catch other potential runtime errors during loading/setup
                    System.err.println("Unexpected runtime error loading game: " + re.getMessage());
                    re.printStackTrace(); // Log the full stack trace for debugging
                    showErrorDialog("Loading Error", "An unexpected runtime error occurred while loading:\n" + re.getMessage());
                }
            }
        });

        // Load default configuration (Keeping previous try-catch structure here for consistency)
        defaultItem.setOnAction(e -> {
            try {
                Game game = GameLauncher.getInstance().load();
                if (game != null) { // Check if default load was successful
                    GameEngine engine = new GameEngine(game, stage.getScene());
                    this.setCenter(engine.getRoot());
                    engine.getRoot().requestFocus();
                    engine.start();
                    resizeStage();
                } else {
                    showErrorDialog("Loading Error", "Could not load the default game configuration.");
                }
                // Keep specific catches for default loading too, mirroring the file load logic
            } catch (MapException me) {
                System.err.println("Error loading default game map/config: " + me.getMessage());
                me.printStackTrace();
                showErrorDialog("Loading Error", "Failed to load default game:\n" + me.getMessage());
            } catch (RuntimeException re) {
                System.err.println("Unexpected runtime error loading default game: " + re.getMessage());
                re.printStackTrace();
                showErrorDialog("Loading Error", "An unexpected runtime error occurred while loading the default game:\n" + re.getMessage());
            }
            // Removed generic Exception catch here too
        });

        // Open map editor (still TODO)
        editorOpen.setOnAction(e -> {
            System.err.println("[TODO] Not implemented: Open map editor");
        });

        // Exit
        exitItem.setOnAction(e -> System.exit(0));
    }

    // Helper method to resize the stage after content change
    private void resizeStage() {
        stage.sizeToScene();
        stage.centerOnScreen(); // Good practice to re-center
    }

    // Helper method to show error dialogs
    private void showErrorDialog(String title, String message) {
        Alert alert = new Alert(AlertType.ERROR);
        alert.setTitle(title);
        alert.setHeaderText(null); // No header text
        alert.setContentText(message);
        alert.initOwner(stage); // Associate with the main stage
        alert.showAndWait();
    }
}