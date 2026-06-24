<?php
require_once 'db.php';
header('Content-Type: application/json');

$filter = isset($_GET['filter']) ? $_GET['filter'] : 'today';

// Set date condition
$dateCondition = '1=1';
if ($filter === 'today') {
    $dateCondition = 'DATE(scanned_at) = CURDATE()';
} else if ($filter === 'week') {
    $dateCondition = 'scanned_at >= DATE_SUB(NOW(), INTERVAL 7 DAY)';
} else if ($filter === 'month') {
    $dateCondition = 'scanned_at >= DATE_SUB(NOW(), INTERVAL 30 DAY)';
}

try {
    // 1. Total scans
    $stmt = $pdo->query("SELECT COUNT(*) as total FROM scans WHERE $dateCondition");
    $totalScans = (int)$stmt->fetch()['total'];

    // 2. Success/Deny Counts
    $stmt = $pdo->query("SELECT SUM(CASE WHEN access = 1 THEN 1 ELSE 0 END) as success, SUM(CASE WHEN access = 0 THEN 1 ELSE 0 END) as deny FROM scans WHERE $dateCondition");
    $res = $stmt->fetch();
    $successCount = isset($res['success']) ? (int)$res['success'] : 0;
    $denyCount = isset($res['deny']) ? (int)$res['deny'] : 0;
    $successRate = $totalScans > 0 ? round(($successCount / $totalScans) * 100, 1) . '%' : '100%';

    // 3. Unique Users Count
    $stmt = $pdo->query("SELECT COUNT(DISTINCT uid) as unique_count FROM scans WHERE $dateCondition");
    $uniqueCount = (int)$stmt->fetch()['unique_count'];

    // 4. Hourly/Daily/Weekly Access counts
    $timeLabels = [];
    $timeData = [];
    if ($filter === 'today') {
        // Aggregate by hour
        $stmt = $pdo->query("
            SELECT HOUR(scanned_at) as hour, COUNT(*) as count 
            FROM scans 
            WHERE $dateCondition 
            GROUP BY HOUR(scanned_at) 
            ORDER BY HOUR(scanned_at)
        ");
        $hourly = [];
        while ($row = $stmt->fetch()) {
            $hourly[(int)$row['hour']] = (int)$row['count'];
        }
        for ($h = 8; $h <= 18; $h++) {
            $timeLabels[] = sprintf("%02d:00", $h);
            $timeData[] = isset($hourly[$h]) ? $hourly[$h] : 0;
        }
    } else if ($filter === 'week') {
        // Aggregate by day of week
        $stmt = $pdo->query("
            SELECT DATE_FORMAT(scanned_at, '%w') as daynum, DATE_FORMAT(scanned_at, '%a') as dayname, DATE(scanned_at) as dt, COUNT(*) as count 
            FROM scans 
            WHERE $dateCondition 
            GROUP BY DATE(scanned_at) 
            ORDER BY DATE(scanned_at)
        ");
        while ($row = $stmt->fetch()) {
            $timeLabels[] = $row['dayname'];
            $timeData[] = (int)$row['count'];
        }
    } else if ($filter === 'month') {
        // Aggregate by week
        $stmt = $pdo->query("
            SELECT WEEK(scanned_at) as wk, DATE_FORMAT(scanned_at, '%v주차') as label, COUNT(*) as count 
            FROM scans 
            WHERE $dateCondition 
            GROUP BY WEEK(scanned_at) 
            ORDER BY WEEK(scanned_at)
        ");
        while ($row = $stmt->fetch()) {
            $timeLabels[] = $row['label'];
            $timeData[] = (int)$row['count'];
        }
    }

    // 5. Access count by department
    $deptLabels = [];
    $deptData = [];
    $stmt = $pdo->query("
        SELECT COALESCE(SUBSTRING_INDEX(u.role, ' / ', 1), '미등록/외부') as dept, COUNT(*) as count 
        FROM scans s 
        LEFT JOIN users u ON s.uid = u.uid 
        WHERE $dateCondition 
        GROUP BY dept 
        ORDER BY count DESC
    ");
    while ($row = $stmt->fetch()) {
        $deptLabels[] = $row['dept'];
        $deptData[] = (int)$row['count'];
    }

    // 6. Peak access time estimation
    $peakTime = '없음';
    if ($totalScans > 0) {
        $stmt = $pdo->query("
            SELECT HOUR(scanned_at) as hr, COUNT(*) as cnt 
            FROM scans 
            WHERE $dateCondition 
            GROUP BY hr 
            ORDER BY cnt DESC 
            LIMIT 1
        ");
        $peakHr = $stmt->fetch();
        if ($peakHr) {
            $peakTime = sprintf("%02d:00 - %02d:00", $peakHr['hr'], $peakHr['hr'] + 1);
        }
    }

    echo json_encode([
        'totalScans' => $totalScans . '회',
        'successRate' => $successRate,
        'successSub' => '거부 ' . $denyCount . '회 발생',
        'uniqueUsers' => $uniqueCount . '명',
        'peakTime' => $peakTime,
        'timeLabels' => $timeLabels,
        'timeData' => $timeData,
        'statusData' => [$successCount, $denyCount],
        'deptLabels' => $deptLabels,
        'deptData' => $deptData
    ]);
} catch (\PDOException $e) {
    http_response_code(500);
    echo json_encode(['error' => $e->getMessage()]);
}
?>
