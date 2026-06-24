CREATE DATABASE IF NOT EXISTS rfid_system;
USE rfid_system;

-- 사용자 테이블
CREATE TABLE IF NOT EXISTS users (
  uid VARCHAR(50) PRIMARY KEY,
  name VARCHAR(100) NOT NULL,
  role VARCHAR(100) NOT NULL,
  phone VARCHAR(50) NOT NULL,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 출입 기록 테이블
CREATE TABLE IF NOT EXISTS scans (
  id INT AUTO_INCREMENT PRIMARY KEY,
  uid VARCHAR(50) NOT NULL,
  access TINYINT(1) NOT NULL,
  scanned_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 초기 테스트용 샘플 사용자 데이터 삽입
INSERT IGNORE INTO users (uid, name, role, phone) VALUES 
('4A E1 8B 2F', '정재우', '개발팀 / 과장', '010-1234-5678'),
('8C F9 10 A2', '김태희', '인사팀 / 대리', '010-9876-5432'),
('03 D8 D4 5E', '박지성', '마케팅팀 / 부장', '010-5555-4444'),
('5B 7A C1 34', '이영희', '보안팀 / 사원', '010-1111-2222');

-- 초기 테스트용 샘플 출입 기록 삽입
INSERT INTO scans (uid, access, scanned_at) VALUES
('4A E1 8B 2F', 1, NOW() - INTERVAL 1 HOUR),
('8C F9 10 A2', 1, NOW() - INTERVAL 45 MINUTE),
('FF 3D E4 78', 0, NOW() - INTERVAL 30 MINUTE),
('03 D8 D4 5E', 1, NOW() - INTERVAL 10 MINUTE);
