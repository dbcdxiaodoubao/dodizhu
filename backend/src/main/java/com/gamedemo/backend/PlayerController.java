package com.gamedemo.backend;

import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/api/players")
public class PlayerController {
    private final OnlinePlayerService onlinePlayerService;
    private final PlayerRepository playerRepository;
    private final GameRecordRepository gameRecordRepository;

    public PlayerController(OnlinePlayerService onlinePlayerService,
                            PlayerRepository playerRepository,
                            GameRecordRepository gameRecordRepository) {
        this.onlinePlayerService = onlinePlayerService;
        this.playerRepository = playerRepository;
        this.gameRecordRepository = gameRecordRepository;
    }

    @GetMapping("/{playerId}/online")
    public OnlineResponse online(@PathVariable String playerId) {
        return new OnlineResponse(playerId, onlinePlayerService.isOnline(playerId));
    }

    @PostMapping("/{playerId}/offline")
    public void offline(@PathVariable String playerId) {
        onlinePlayerService.markOffline(playerId);
    }

    @GetMapping("/{playerId}")
    public ProfileResponse profile(@PathVariable String playerId) {
        Player player = playerRepository.findById(playerId)
                .orElseThrow(() -> new IllegalArgumentException("player not found"));
        return new ProfileResponse(player.getId(), player.getCoins(), onlinePlayerService.isOnline(playerId));
    }

    @GetMapping("/{playerId}/stats")
    public StatsResponse stats(@PathVariable String playerId) {
        var records = gameRecordRepository.findByPlayerId(playerId);
        int wins = (int) records.stream().filter(record -> "WIN".equals(record.getResult())).count();
        int coinChange = records.stream().mapToInt(GameRecord::getCoinChange).sum();
        return new StatsResponse(records.size(), wins, coinChange);
    }

    public record OnlineResponse(String playerId, boolean online) {
    }

    public record ProfileResponse(String playerId, int coins, boolean online) {
    }

    public record StatsResponse(int games, int wins, int coinChange) {
    }
}
