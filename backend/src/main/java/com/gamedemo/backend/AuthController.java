package com.gamedemo.backend;

import jakarta.validation.Valid;
import jakarta.validation.constraints.NotBlank;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/api/auth")
public class AuthController {
    private static final int INITIAL_COINS = 1000;

    private final PlayerRepository playerRepository;
    private final OnlinePlayerService onlinePlayerService;

    public AuthController(PlayerRepository playerRepository, OnlinePlayerService onlinePlayerService) {
        this.playerRepository = playerRepository;
        this.onlinePlayerService = onlinePlayerService;
    }

    @PostMapping("/login")
    public LoginResponse login(@Valid @RequestBody LoginRequest request) {
        LoginResponse response = playerRepository.findById(request.playerId())
                .map(player -> new LoginResponse(player.getId(), player.getCoins(), false))
                .orElseGet(() -> {
                    Player player = playerRepository.save(new Player(request.playerId(), INITIAL_COINS));
                    return new LoginResponse(player.getId(), player.getCoins(), true);
                });
        onlinePlayerService.markOnline(response.playerId());
        return response;
    }

    public record LoginRequest(@NotBlank String playerId) {
    }

    public record LoginResponse(String playerId, int coins, boolean created) {
    }
}
