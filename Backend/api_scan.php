<?php
require_once 'db.php';
header('Content-Type: application/json');

// Read raw POST body
$input = json_decode(file_get_contents('php://input'), true);
$uid = isset($input['uid']) ? trim($input['uid']) : '';

if (empty($uid)) {
    http_response_code(400);
    echo json_encode(['error' => 'UID is required']);
    exit;
}

try {
    // 1. Check if UID exists in users table
    $stmt = $pdo->prepare('SELECT * FROM users WHERE uid = ?');
    $stmt->execute([$uid]);
    $user = $stmt->fetch();

    $access = 0;
    $name = '미등록 사용자';
    $role = '알 수 없음';
    
    if ($user) {
        $access = 1;
        $name = $user['name'];
        $role = $user['role'];
    }

    // 2. Insert into scans table
    $stmt = $pdo->prepare('INSERT INTO scans (uid, access) VALUES (?, ?)');
    $stmt->execute([$uid, $access]);

    echo json_encode([
        'success' => true,
        'uid' => $uid,
        'access' => (bool)$access,
        'name' => $name,
        'role' => $role
    ]);
} catch (\PDOException $e) {
    http_response_code(500);
    echo json_encode(['error' => $e->getMessage()]);
}
?>
