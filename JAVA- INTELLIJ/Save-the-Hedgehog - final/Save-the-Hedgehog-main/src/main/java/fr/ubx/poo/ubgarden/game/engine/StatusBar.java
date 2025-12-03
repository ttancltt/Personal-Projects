/*
 * Copyright (c) 2020. Laurent Réveillère
 */

package fr.ubx.poo.ubgarden.game.engine;

import fr.ubx.poo.ubgarden.game.Game;
import fr.ubx.poo.ubgarden.game.view.ImageResource;
import fr.ubx.poo.ubgarden.game.view.ImageResourceFactory;
import javafx.scene.Group;
import javafx.scene.effect.DropShadow;
import javafx.scene.image.Image;
import javafx.scene.image.ImageView;
import javafx.scene.layout.HBox;
import javafx.scene.paint.Color;
import javafx.scene.text.Text;

public class StatusBar {
    public static final int height = 55;
    private final HBox levelDisplayHBox; // The HBox for "Level: X"
    private final Text energyText;
    private final Text diseaseLevelText;
    private final Text insecticideNumberText;
    private int displayedGameLevel = -1; // To track displayed level and avoid needless updates


    public StatusBar(HBox levelDisplayHBox, Text energyText, Text diseaseLevelText, Text insecticideNumberText) {
        this.levelDisplayHBox = levelDisplayHBox;
        this.energyText = energyText;
        this.diseaseLevelText = diseaseLevelText;
        this.insecticideNumberText = insecticideNumberText;
    }

    private void updateLevelDisplay(int newLevelNum) {
        if (newLevelNum != displayedGameLevel) { // Only update if changed
            levelDisplayHBox.getChildren().clear();
            // Logic to add correct digit ImageViews based on newLevelNum
            // Example for single digit, expand for multi-digit if needed
            String levelStr = String.valueOf(newLevelNum);
            for (char c : levelStr.toCharArray()) {
                int digit = Character.getNumericValue(c);
                if (digit >= 0 && digit <= 9) {
                    levelDisplayHBox.getChildren().add(new ImageView(ImageResourceFactory.getInstance().getDigit(digit)));
                }
            }
            displayedGameLevel = newLevelNum;
        }
    }


    public void update(Game game) {
        energyText.setText(String.valueOf(game.getGardener().getEnergy()));
        insecticideNumberText.setText(String.valueOf(game.getGardener().getInsecticideBombCount()));
        diseaseLevelText.setText("x" + game.getGardener().getActivePoisonEffectCount());
        updateLevelDisplay(game.world().currentLevel());
    }
}