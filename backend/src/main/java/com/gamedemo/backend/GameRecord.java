package com.gamedemo.backend;

import jakarta.persistence.Entity;
import jakarta.persistence.GeneratedValue;
import jakarta.persistence.GenerationType;
import jakarta.persistence.Id;
import jakarta.persistence.Table;
import jakarta.persistence.UniqueConstraint;

@Entity
@Table(name = "game_records", uniqueConstraints = @UniqueConstraint(columnNames = {"gameId", "playerId"}))
public class GameRecord {
    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    private String gameId;
    private String playerId;
    private int coinChange;
    private String result;
    private long durationSeconds;

    protected GameRecord() {
    }

    public GameRecord(String gameId, String playerId, int coinChange, String result, long durationSeconds) {
        this.gameId = gameId;
        this.playerId = playerId;
        this.coinChange = coinChange;
        this.result = result;
        this.durationSeconds = durationSeconds;
    }

    public String getGameId() {
        return gameId;
    }

    public int getCoinChange() {
        return coinChange;
    }

    public String getResult() {
        return result;
    }

    public long getDurationSeconds() {
        return durationSeconds;
    }
}
