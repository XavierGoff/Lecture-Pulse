// ==UserScript==
// @name         Classroom Mood Bar
// @namespace    http://tampermonkey.net/
// @version      1.0
// @description  Shows a mood bar and question mark on every page, fed by Arduino via WebSocket
// @author       You
// @match        *://*/*
// @grant        GM_xmlhttpRequest
// @connect      127.0.0.1
// @run-at       document-end
// ==/UserScript==

(function () {
  'use strict';

  // ── Mood bar ─────────────────────────────────────────────────
  const bar = document.createElement("div");
  Object.assign(bar.style, {
    position:   "fixed",
    bottom:     "0",
    left:       "0",
    width:      "100%",
    height:     "20px",
    background: "rgb(255,255, 0)", //255,255,0
    zIndex:     "2147483647",
    transition: "background-color 2s ease",
    pointerEvents: "none"
  });
  document.body.appendChild(bar);

  // ── Question mark ─────────────────────────────────────────────
  const qmark = document.createElement("div");
  Object.assign(qmark.style, {
    position:   "fixed",
    bottom:     "18px",
    right:      "16px",
    fontSize:   "200px",
    zIndex:     "2147483647",
    opacity:    "0",
    transition: "opacity 0.5s ease",
    pointerEvents: "none"
  });
  qmark.textContent = "❓";
  document.body.appendChild(qmark);

  // ── Score to colour ───────────────────────────────────────────
  let smoothedScore = 0;
  let targetScore   = 0;

  function scoreToColor(score) {
    score = Math.max(-10, Math.min(10, score));
    let r, g;
    if (score < 0) {
      const t = (score + 10) / 10;
      r = 255; g = Math.round(255 * t);
    } else {
      const t = score / 10;
      r = Math.round(255 * (1 - t)); g = 255;
    }
    return `rgb(${r}, ${g}, 0)`;
  }

  setInterval(() => {
    smoothedScore += (targetScore - smoothedScore) * 0.05;
    bar.style.background = scoreToColor(smoothedScore);
  }, 100);

  // ── WebSocket ─────────────────────────────────────────────────
  function connect() {
    try {
      const ws = new WebSocket("ws://127.0.0.1:8765");
      ws.onopen = () => console.log("[Mood] Connected");
      ws.onmessage = (e) => {
        try {
          const msg = JSON.parse(e.data);
          if (msg.score    !== undefined) targetScore = msg.score;
          if (msg.question !== undefined) qmark.style.opacity = msg.question ? "1" : "0";
        } catch (_) {
          const val = parseFloat(e.data);
          if (!isNaN(val)) targetScore = val;
        }
      };
      ws.onerror = () => ws.close();
      ws.onclose = () => setTimeout(connect, 2000);
    } catch (_) {
      setTimeout(connect, 2000);
    }
  }

  connect();
})();