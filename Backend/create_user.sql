CREATE USER IF NOT EXISTS 'rfid_user'@'localhost' IDENTIFIED BY 'rfid_password123';
GRANT ALL PRIVILEGES ON rfid_system.* TO 'rfid_user'@'localhost';
FLUSH PRIVILEGES;
