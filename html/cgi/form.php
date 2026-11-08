<?php
header("Content-Type: text/html; charset=UTF-8");

$method = $_SERVER['REQUEST_METHOD'];
$name = isset($_REQUEST['name']) ? htmlspecialchars($_REQUEST['name']) : '';
$body = file_get_contents('php://input');
?>
<html>
<body>
<h1>PHP CGI</h1>
<p>Method: <?= $method ?></p>
<p>Query: <?= htmlspecialchars($_SERVER['QUERY_STRING']) ?></p>
<p>Name: <?= $name !== '' ? $name : '(none)' ?></p>
<p>Body (<?= strlen($body) ?> bytes): <?= htmlspecialchars($body) ?></p>

<form method="POST" action="form.php">
    <input name="name" placeholder="name">
    <button>Send</button>
</form>
</body>
</html>
