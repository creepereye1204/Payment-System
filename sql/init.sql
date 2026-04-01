CREATE DATABASE IF NOT EXISTS pg_gateway;
USE pg_gateway;

CREATE TABLE IF NOT EXISTS blacklist (
    id       INT AUTO_INCREMENT PRIMARY KEY,
    card_no  VARCHAR(20) NOT NULL UNIQUE,
    reason   VARCHAR(100),
    added_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS daily_limit (
    id            INT AUTO_INCREMENT PRIMARY KEY,
    card_no       VARCHAR(20) NOT NULL,
    limit_date    DATE NOT NULL,
    total_amount  BIGINT DEFAULT 0,
    UNIQUE KEY uq_card_date (card_no, limit_date)
);

CREATE TABLE IF NOT EXISTS transactions (
    id          INT AUTO_INCREMENT PRIMARY KEY,
    txn_id      VARCHAR(32) NOT NULL UNIQUE,
    card_no     VARCHAR(20) NOT NULL,
    amount      INT NOT NULL,
    merchant_id VARCHAR(20) NOT NULL,
    status      TINYINT NOT NULL,
    reason      VARCHAR(64),
    created_at  DATETIME DEFAULT CURRENT_TIMESTAMP
);

INSERT INTO blacklist (card_no, reason) VALUES
('1111-1111-1111-1111', 'TEST_BLACKLIST'),
('9999-9999-9999-9999', 'FRAUD_DETECTED');
