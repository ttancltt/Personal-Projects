package fr.ubx.poo.ubgarden.game;

public record Configuration(int gardenerEnergy,
                            int energyBoost,
                            long energyRecoverDuration,
                            long diseaseDuration,
                            int waspMoveFrequency,
                            int hornetMoveFrequency,
                            long waspSpawnIntervalMillis,
                            int waspDamage,
                            int hornetDamage,// Damage dealt by a wasp sting
                            long hornetStingCooldownMillis,
                            int gardenerBombCapacity  ) {
}
