package com.gamedemo.backend;

import java.time.Duration;
import org.springframework.data.redis.core.StringRedisTemplate;
import org.springframework.stereotype.Service;

@Service
public class OnlinePlayerService {
    private static final Duration ONLINE_TTL = Duration.ofMinutes(5);

    private final StringRedisTemplate redisTemplate;

    public OnlinePlayerService(StringRedisTemplate redisTemplate) {
        this.redisTemplate = redisTemplate;
    }

    public void markOnline(String playerId) {
        redisTemplate.opsForValue().set(key(playerId), "1", ONLINE_TTL);
    }

    public boolean isOnline(String playerId) {
        return Boolean.TRUE.equals(redisTemplate.hasKey(key(playerId)));
    }

    public void markOffline(String playerId) {
        redisTemplate.delete(key(playerId));
    }

    private String key(String playerId) {
        return "online:player:" + playerId;
    }
}
