<?php
require_once 'db.php';
header('Content-Type: application/json');

$input = json_decode(file_get_contents('php://input'), true);
$uid = isset($input['uid']) ? trim($input['uid']) : '';
$name = isset($input['name']) ? trim($input['name']) : '';
$role = isset($input['role']) ? trim($input['role']) : '';
$phone = isset($input['phone']) ? trim($input['phone']) : '';

if (empty($uid) || empty($name) || empty($role) || empty($phone)) {
    http_response_code(400);
    echo json_encode(['error' => 'All fields (uid, name, role, phone) are required']);
    exit;
}

try {
    // Insert or update user
    $stmt = $pdo->prepare('
        INSERT INTO users (uid, name, role, phone) 
        VALUES (?, ?, ?, ?) 
        ON DUPLICATE KEY UPDATE name = VALUES(name), role = VALUES(role), phone = VALUES(phone)
    ');
    $stmt->execute([$uid, $name, $role, $phone]);

    echo json_encode(['success' => true, 'message' => 'User registered successfully']);
} catch (\PDOException $e) {
    http_response_code(500);
    echo json_encode(['error' => $e->getMessage()]);
}
?>
