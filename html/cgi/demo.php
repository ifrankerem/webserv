<?php
$name = isset($_POST['name']) ? htmlspecialchars(trim($_POST['name'])) : '';
?>
<!DOCTYPE html>
<html lang="tr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Webserv PHP Demo</title>
    <link rel="stylesheet" href="demo.css">
</head>
<body>
    <div class="card">
        <header>
            <h1>Webserv + PHP</h1>
            <p>Bu sayfa CGI ile php-cgi tarafından üretildi.</p>
        </header>
        <section>
            <table>
                <tr><td>Method</td><td><?= $_SERVER['REQUEST_METHOD'] ?></td></tr>
                <tr><td>Saat</td><td><?= date('H:i:s') ?></td></tr>
                <tr><td>PHP sürümü</td><td><?= phpversion() ?></td></tr>
                <tr><td>Sunucu</td><td><?= htmlspecialchars($_SERVER['SERVER_NAME'] . ':' . $_SERVER['SERVER_PORT']) ?></td></tr>
                <tr><td>İstemci</td><td><?= htmlspecialchars($_SERVER['REMOTE_ADDR']) ?></td></tr>
            </table>

            <form method="POST" action="demo.php">
                <input name="name" placeholder="Adını yaz" required>
                <button>Gönder</button>
            </form>

            <?php if ($name !== ''): ?>
                <div class="greet">Merhaba, <strong><?= $name ?></strong>! POST isteğin sunucudan PHP'ye ulaştı.</div>
            <?php endif; ?>
        </section>
    </div>
</body>
</html>
