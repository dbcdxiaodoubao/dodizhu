package com.gamedemo.backend;

import org.springframework.data.jpa.repository.JpaRepository;

import java.util.Optional;
import java.util.List;

public interface GameRecordRepository extends JpaRepository<GameRecord, Long> {
    Optional<GameRecord> findByGameIdAndPlayerId(String gameId, String playerId);
    List<GameRecord> findTop20ByPlayerIdOrderByIdDesc(String playerId);
}
