package com.gamedemo.backend;

import jakarta.transaction.Transactional;
import jakarta.validation.Valid;
import jakarta.validation.constraints.Min;
import jakarta.validation.constraints.NotBlank;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
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
        if (gameRecordRepository.findByGameIdAndPlayerId(request.gameId(), request.playerId()).isPresent()) {
            return new SettlementResponse(player.getId(), player.getCoins());
        }
        player.changeCoins(request.coinChange());
        gameRecordRepository.save(new GameRecord(
                request.gameId(), player.getId(), request.coinChange(), request.result(), request.durationSeconds()));
        return new SettlementResponse(player.getId(), player.getCoins());
    }

    @GetMapping("/{playerId}/history")
    public java.util.List<GameHistoryResponse> history(@PathVariable String playerId) {
        return gameRecordRepository.findTop20ByPlayerIdOrderByIdDesc(playerId).stream()
                .map(record -> new GameHistoryResponse(
                        record.getGameId(), record.getCoinChange(), record.getResult(), record.getDurationSeconds()))
                .toList();
    }

    public record SettlementRequest(
            @NotBlank String gameId,
            @NotBlank String playerId,
            int coinChange,
            @NotBlank String result,
            @Min(0) long durationSeconds) {
    }

    public record SettlementResponse(String playerId, int coins) {
    }

    public record GameHistoryResponse(String gameId, int coinChange, String result, long durationSeconds) {
    }
}
