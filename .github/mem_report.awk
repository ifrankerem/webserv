# Turns the 42 tester memory log into a GitHub job summary (markdown).
# Input lines: <unix time> <webserv RSS kB> <CGI children RSS kB> <plain|cgi>
# Vars: dur (tester seconds), hwm (webserv VmHWM kB), rc (tester exit code)

function mb(kb) { return sprintf("%.1f MB", kb / 1024) }
function hms(s) { return sprintf("%dm %02ds", int(s / 60), int(s) % 60) }

function add(g, v, dt) {
	if (!(g in n) || v < lo[g]) lo[g] = v
	if (!(g in n) || v > hi[g]) hi[g] = v
	n[g]++; vsum[g] += v; wsum[g] += v * dt; wt[g] += dt
}

function row(label, g) {
	if (!(g in n)) { print "| " label " | - | - | - | - |"; return }
	# time-weighted mean, so a sampler slowed down by load does not skew it
	avg = wt[g] > 0 ? wsum[g] / wt[g] : vsum[g] / n[g]
	print "| " label " | " hms(tm[substr(g, 1, index(g, " ") - 1)]) " | " mb(lo[g]) " | " mb(avg) " | " mb(hi[g]) " |"
}

function chart(title, arr,    b, i, line, v) {
	for (b = 0; b < B; b++) peak[b] = -1
	for (i = 1; i <= k; i++) {
		b = int((T[i] - T[1]) / span * B); if (b >= B) b = B - 1
		if (arr[i] > peak[b]) peak[b] = arr[i]
	}
	line = ""; v = 0
	for (b = 0; b < B; b++) {
		if (peak[b] >= 0) v = peak[b]   # empty bucket keeps the previous value
		line = line (b ? ", " : "") sprintf("%.1f", v / 1024)
	}
	print "```mermaid"
	print "xychart-beta"
	print "  title \"" title " (MB, peak per " sprintf("%.1f", span / B) " s)\""
	print "  x-axis \"time (s)\" 0 --> " int(span)
	print "  y-axis \"MB\""
	print "  line [" line "]"
	print "```"
}

NF == 4 {
	t = $1; ws = $2; tot = $2 + $3; kind = $4
	dt = k ? t - T[k] : 0
	k++; T[k] = t; WS[k] = ws; TOT[k] = tot
	tm[kind] += dt; tm["all"] += dt
	add(kind " ws", ws, dt); add("all ws", ws, dt)
	if (kind == "cgi") add("cgi tot", tot, dt)
	add("all tot", tot, dt)
}

END {
	print "## 42 tester memory"
	print ""
	if (!k) { print "No samples: webserv was not running."; exit }
	print "| tests | time | min | avg | peak |"
	print "|---|---|---|---|---|"
	row("non-CGI tests: webserv", "plain ws")
	row("CGI tests (.bla): webserv", "cgi ws")
	row("CGI tests (.bla): webserv + CGI processes", "cgi tot")
	row("all tests: webserv", "all ws")
	row("all tests: webserv + CGI processes", "all tot")
	print ""
	print "- webserv exact peak (kernel VmHWM): " (hwm ? mb(hwm) : "n/a, webserv died")
	print "- tester run time: " hms(dur) ", exit code " rc
	print "- " k " samples, one every ~0.5 s; peaks of CGI processes shorter than that can be missed"
	print ""
	span = T[k] - T[1]
	if (span <= 0) exit
	B = k < 60 ? k : 60
	chart("webserv RSS", WS)
	chart("webserv + CGI processes RSS", TOT)
}
