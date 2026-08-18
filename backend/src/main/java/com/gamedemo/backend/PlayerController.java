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

    public PlayerController(OnlinePlayerService onlinePlayerService) {
        this.onlinePlayerService = onlinePlayerService;
    }

    @GetMapping("/{playerId}/online")
    public OnlineResponse online(@PathVariable String playerId) {
        return new OnlineResponse(playerId, onlinePlayerService.isOnline(playerId));
    }

    @PostMapping("/{playerId}/offline")
    public void offline(@PathVariable String playerId) {
        onlinePlayerService.markOffline(playerId);
    }

    public record OnlineResponse(String playerId, boolean online) {
    }
}
