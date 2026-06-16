#!/bin/bash
set -e

case "${MYSQL_DATABASE:-}" in
  ""|*[!A-Za-z0-9_]*)
    echo "MYSQL_DATABASE must contain only letters, numbers, and underscores" >&2
    exit 1
    ;;
esac

mysql=(mysql --protocol=socket -uroot -p"${MYSQL_ROOT_PASSWORD}")

"${mysql[@]}" <<EOSQL
CREATE DATABASE IF NOT EXISTS \`${MYSQL_DATABASE}\`
  CHARACTER SET utf8mb4
  COLLATE utf8mb4_unicode_ci;

USE \`${MYSQL_DATABASE}\`;

CREATE TABLE IF NOT EXISTS users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(255) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_username (username)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS chat_message (
    message_id BIGINT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL,
    username VARCHAR(255) NOT NULL,
    is_user TINYINT(1) NOT NULL DEFAULT 1,
    content TEXT NOT NULL,
    ts BIGINT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_user_id (user_id),
    INDEX idx_username (username),
    INDEX idx_ts (ts),
    CONSTRAINT fk_chat_message_user
      FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
EOSQL
