<?php
require_once 'db.php';

header('Content-Type: text/csv; charset=utf-8');
header('Content-Disposition: attachment; filename=rfid_access_logs.csv');

// Output byte order mark (BOM) for Excel UTF-8 compatibility
echo "\xEF\xBB\xBF";

$output = fopen('php://output', 'w');

// Column headers
fputcsv($output, ['시간', '카드 UID', '소유자', '소속 (부서/직급)', '결과']);

try {
    $stmt = $pdo->query('
        SELECT s.scanned_at, s.uid, 
               COALESCE(u.name, "미등록 사용자") as name, 
               COALESCE(u.role, "알 수 없음") as role, 
               s.access 
        FROM scans s 
        LEFT JOIN users u ON s.uid = u.uid 
        ORDER BY s.scanned_at DESC
    ');

    while ($row = $stmt->fetch()) {
        $result = $row['access'] ? '승인됨' : '거부됨';
        fputcsv($output, [
            $row['scanned_at'],
            $row['uid'],
            $row['name'],
            $row['role'],
            $result
        ]);
    }
} catch (\PDOException $e) {
    // Fail silently
}
fclose($output);
?>
