<?php
require_once 'db.php';
header('Content-Type: application/json');

try {
    // Fetch scans joined with user details, ordering by scanned_at DESC
    $stmt = $pdo->query('
        SELECT s.id, DATE_FORMAT(s.scanned_at, "%H:%i:%s") as time, s.uid, 
               COALESCE(u.name, "미등록 사용자") as name, 
               COALESCE(u.role, "알 수 없음") as role, 
               s.access 
        FROM scans s 
        LEFT JOIN users u ON s.uid = u.uid 
        ORDER BY s.scanned_at DESC 
        LIMIT 50
    ');
    $scans = $stmt->fetchAll();

    foreach ($scans as &$scan) {
        $scan['access'] = (bool)$scan['access'];
    }

    echo json_encode($scans);
} catch (\PDOException $e) {
    http_response_code(500);
    echo json_encode(['error' => $e->getMessage()]);
}
?>
