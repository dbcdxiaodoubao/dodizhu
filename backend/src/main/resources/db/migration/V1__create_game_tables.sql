CREATE TABLE IF NOT EXISTS players (
    id VARCHAR(255) NOT NULL,
    coins INT NOT NULL,
    PRIMARY KEY (id)
);

CREATE TABLE IF NOT EXISTS game_records (
    id BIGINT NOT NULL AUTO_INCREMENT,
    game_id VARCHAR(255) NOT NULL,
    player_id VARCHAR(255) NOT NULL,
    coin_change INT NOT NULL,
    result VARCHAR(255) NOT NULL,
    duration_seconds BIGINT NOT NULL,
    PRIMARY KEY (id),
    CONSTRAINT uk_game_record_game_player UNIQUE (game_id, player_id)
);
