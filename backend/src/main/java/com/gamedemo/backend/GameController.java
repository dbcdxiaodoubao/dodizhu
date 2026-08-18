package com.gamedemo.backend;

import jakarta.transaction.Transactional;
import jakarta.validation.Valid;
import jakarta.validation.constraints.Min;
import jakarta.validation.constraints.NotBlank;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/api/games")
public class GameController {
    private final PlayerRepository playerRepository;
    private final GameRecordRepository gameRecordRepository;

    public GameController(PlayerRepository playerRepository, GameRecordRepository gameRecordRepository) {
        this.playerRepository = playerRepository;
        this.gameRecordRepository = gameRecordRepository;
    }

    @Transactional
    @PostMapping("/settle")
    public SettlementResponse settle(@Valid @RequestBody SettlementRequest request) {
        Player player = playerRepository.findById(request.playerId())
                .orElseThrow(() -> new IllegalArgumentException("player not found"));
        player.changeCoins(request.coinChange());
        gameRecordRepository.save(new GameRecord(
                player.getId(), request.coinChange(), request.result(), request.durationSeconds()));
        return new SettlementResponse(player.getId(), player.getCoins());
    }

    public record SettlementRequest(
            @NotBlank String playerId,
            int coinChange,
            @NotBlank String result,
            @Min(0) long durationSeconds) {
    }

    public record SettlementResponse(String playerId, int coins) {
    }
}
