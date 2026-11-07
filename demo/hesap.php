<?php
header("Content-Type: text/html; charset=UTF-8");

$a = isset($_GET["a"]) ? $_GET["a"] : "";
$b = isset($_GET["b"]) ? $_GET["b"] : "";
$op = isset($_GET["op"]) ? $_GET["op"] : "topla";
$symbols = ["topla" => "+", "cikar" => "-", "carp" => "*", "bol" => "/"];

$result = "";
if ($a !== "" && $b !== "") {
    if (!is_numeric($a) || !is_numeric($b)) {
        $result = "Sayı girmelisin.";
    } else {
        $x = (float)$a;
        $y = (float)$b;
        if ($op === "topla") $r = $x + $y;
        elseif ($op === "cikar") $r = $x - $y;
        elseif ($op === "carp") $r = $x * $y;
        elseif ($y == 0) $r = null;
        else $r = $x / $y;
        $sym = isset($symbols[$op]) ? $symbols[$op] : "?";
        $result = $r === null ? "Sıfıra bölünemez." : "$x $sym $y = $r";
    }
}

function option($value, $label, $op) {
    $selected = $value === $op ? " selected" : "";
    return "<option value=\"$value\"$selected>$label</option>";
}
?>
<!DOCTYPE html>
<html lang="tr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Hesap makinesi (PHP)</title>
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    <header><a href="/">webserv demo</a> / Hesap makinesi</header>
    <main>
        <section>
            <h2>Hesap makinesi — PHP</h2>
            <p class="muted">Çalışan yorumlayıcı: PHP <?= PHP_VERSION ?></p>
            <form method="GET" action="/hesap.php">
                <input name="a" value="<?= htmlspecialchars($a) ?>" placeholder="a" size="8">
                <select name="op"><?= option("topla", "+", $op) . option("cikar", "-", $op) . option("carp", "×", $op) . option("bol", "÷", $op) ?></select>
                <input name="b" value="<?= htmlspecialchars($b) ?>" placeholder="b" size="8">
                <button>Hesapla</button>
            </form>
            <pre><?= htmlspecialchars($result !== "" ? $result : "Sonuç burada görünecek.") ?></pre>
            <p><a href="/hesap.py?<?= htmlspecialchars($_SERVER["QUERY_STRING"] ?? "") ?>">Aynı hesabı Python ile yap</a></p>
        </section>
    </main>
</body>
</html>
