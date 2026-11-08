<?php
header("Content-Type: text/html; charset=UTF-8");

$a = isset($_GET["a"]) ? $_GET["a"] : "";
$b = isset($_GET["b"]) ? $_GET["b"] : "";
$op = isset($_GET["op"]) ? $_GET["op"] : "add";
$symbols = ["add" => "+", "sub" => "-", "mul" => "*", "div" => "/"];

$result = "";
if ($a !== "" && $b !== "") {
    if (!is_numeric($a) || !is_numeric($b)) {
        $result = "You must enter numbers.";
    } else {
        $x = (float)$a;
        $y = (float)$b;
        if ($op === "add") $r = $x + $y;
        elseif ($op === "sub") $r = $x - $y;
        elseif ($op === "mul") $r = $x * $y;
        elseif ($y == 0) $r = null;
        else $r = $x / $y;
        $sym = isset($symbols[$op]) ? $symbols[$op] : "?";
        $result = $r === null ? "Cannot divide by zero." : "$x $sym $y = $r";
    }
}

function option($value, $label, $op) {
    $selected = $value === $op ? " selected" : "";
    return "<option value=\"$value\"$selected>$label</option>";
}
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Calculator (PHP)</title>
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    <header><a href="/">webserv demo</a> / Calculator</header>
    <main>
        <section>
            <h2>Calculator — PHP</h2>
            <p class="muted">Running interpreter: PHP <?= PHP_VERSION ?></p>
            <form method="GET" action="/calc.php">
                <input name="a" value="<?= htmlspecialchars($a) ?>" placeholder="a" size="8">
                <select name="op"><?= option("add", "+", $op) . option("sub", "-", $op) . option("mul", "×", $op) . option("div", "÷", $op) ?></select>
                <input name="b" value="<?= htmlspecialchars($b) ?>" placeholder="b" size="8">
                <button>Calculate</button>
            </form>
            <pre><?= htmlspecialchars($result !== "" ? $result : "The result will appear here.") ?></pre>
            <p><a href="/calc.py?<?= htmlspecialchars($_SERVER["QUERY_STRING"] ?? "") ?>">Do the same calculation in Python</a></p>
        </section>
    </main>
</body>
</html>
