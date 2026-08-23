package com.gamedemo.backend;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.servlet.AutoConfigureMockMvc;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.http.MediaType;
import org.springframework.test.web.servlet.MockMvc;
import org.springframework.transaction.annotation.Transactional;

import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.post;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.get;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.content;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.jsonPath;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.status;

@SpringBootTest
@AutoConfigureMockMvc
class HealthControllerTest {
    @Autowired
    private MockMvc mockMvc;

    @Test
    void returnsUpStatus() throws Exception {
        mockMvc.perform(get("/api/health"))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.status").value("UP"));
    }

    @Test
    void servesBrowserClient() throws Exception {
        mockMvc.perform(get("/index.html"))
                .andExpect(status().isOk())
                .andExpect(content().string(org.hamcrest.Matchers.containsString("WebSocket")));
    }

    @Test
    @Transactional
    void createsPlayerOnFirstLogin() throws Exception {
        mockMvc.perform(post("/api/auth/login")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{\"playerId\":\"test-player\"}"))
                .andExpect(status().isOk())
                .andExpect(content().contentTypeCompatibleWith(MediaType.APPLICATION_JSON))
                .andExpect(jsonPath("$.playerId").value("test-player"))
                .andExpect(jsonPath("$.coins").value(1000))
                .andExpect(jsonPath("$.created").value(true));
    }

    @Test
    @Transactional
    void settlesGameAndUpdatesCoins() throws Exception {
        mockMvc.perform(post("/api/auth/login")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{\"playerId\":\"settle-player\"}"))
                .andExpect(status().isOk());

        mockMvc.perform(post("/api/games/settle")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{\"gameId\":\"game-1\",\"playerId\":\"settle-player\",\"coinChange\":120,\"result\":\"WIN\",\"durationSeconds\":60}"))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.playerId").value("settle-player"))
                .andExpect(jsonPath("$.coins").value(1120));

        mockMvc.perform(post("/api/games/settle")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{\"gameId\":\"game-1\",\"playerId\":\"settle-player\",\"coinChange\":120,\"result\":\"WIN\",\"durationSeconds\":60}"))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.coins").value(1120));

        mockMvc.perform(get("/api/games/settle-player/history"))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.length()").value(1))
                .andExpect(jsonPath("$[0].gameId").value("game-1"))
                .andExpect(jsonPath("$[0].coinChange").value(120));
    }

    @Test
    void marksPlayerOnlineAfterLogin() throws Exception {
        mockMvc.perform(post("/api/auth/login")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{\"playerId\":\"redis-player\"}"))
                .andExpect(status().isOk());

        mockMvc.perform(get("/api/players/redis-player/online"))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.online").value(true));

        mockMvc.perform(post("/api/players/redis-player/offline"))
                .andExpect(status().isOk());

        mockMvc.perform(get("/api/players/redis-player/online"))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.online").value(false));
    }

    @Test
    void rejectsBlankPlayerIdWithJsonError() throws Exception {
        mockMvc.perform(post("/api/auth/login")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{\"playerId\":\"\"}"))
                .andExpect(status().isBadRequest())
                .andExpect(jsonPath("$.code").value("BAD_REQUEST"));
    }

    @Test
    void returnsPlayerProfile() throws Exception {
        mockMvc.perform(post("/api/auth/login")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{\"playerId\":\"profile-player\"}"))
                .andExpect(status().isOk());

        mockMvc.perform(get("/api/players/profile-player"))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.playerId").value("profile-player"))
                .andExpect(jsonPath("$.coins").value(1000));
    }

    @Test
    void returnsPlayerStats() throws Exception {
        mockMvc.perform(post("/api/auth/login")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{\"playerId\":\"stats-player\"}"))
                .andExpect(status().isOk());
        mockMvc.perform(post("/api/games/settle")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{\"gameId\":\"stats-game-1\",\"playerId\":\"stats-player\",\"coinChange\":100,\"result\":\"WIN\",\"durationSeconds\":10}"))
                .andExpect(status().isOk());

        mockMvc.perform(get("/api/players/stats-player/stats"))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.games").value(1))
                .andExpect(jsonPath("$.wins").value(1))
                .andExpect(jsonPath("$.coinChange").value(100));
    }

    @Test
    void rejectsSettlementThatWouldMakeCoinsNegative() throws Exception {
        mockMvc.perform(post("/api/auth/login")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{\"playerId\":\"balance-player\"}"))
                .andExpect(status().isOk());

        mockMvc.perform(post("/api/games/settle")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{\"gameId\":\"balance-game\",\"playerId\":\"balance-player\",\"coinChange\":-1001,\"result\":\"LOSE\",\"durationSeconds\":10}"))
                .andExpect(status().isBadRequest())
                .andExpect(jsonPath("$.code").value("BAD_REQUEST"));
    }
}
