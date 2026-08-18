package com.gamedemo.backend;

import jakarta.persistence.Entity;
import jakarta.persistence.GeneratedValue;
import jakarta.persistence.GenerationType;
import jakarta.persistence.Id;
import jakarta.persistence.Table;

@Entity
@Table(name = "game_records")
public class GameRecord {
    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    private String playerId;
    private int coinChange;
    private String result;
    private long durationSeconds;

    protected GameRecord() {
    }

    public GameRecord(String playerId, int coinChange, String result, long durationSeconds) {
        this.playerId = playerId;
        this.coinChange = coinChange;
        this.result = result;
        this.durationSeconds = durationSeconds;
    }
}
