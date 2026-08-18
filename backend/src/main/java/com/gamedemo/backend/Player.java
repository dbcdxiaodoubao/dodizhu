package com.gamedemo.backend;

import jakarta.persistence.Entity;
import jakarta.persistence.Id;
import jakarta.persistence.Table;

@Entity
@Table(name = "players")
public class Player {
    @Id
    private String id;

    private int coins;

    protected Player() {
    }

    public Player(String id, int coins) {
        this.id = id;
        this.coins = coins;
    }

    public String getId() {
        return id;
    }

    public int getCoins() {
        return coins;
    }

    public void changeCoins(int amount) {
        coins += amount;
    }
}
