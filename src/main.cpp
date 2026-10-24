// Infinite Layout Studio: C++ compiled to WebAssembly.
// Every website layout is a pure function of a 31-bit seed. The page is generated as
// real HTML + CSS, can be exported, and a small on-device AI learns which layouts you like.
#include <emscripten/emscripten.h>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace {

const char* NAV[]   = {"top bar", "centered logo", "floating pill"};
const char* HERO[]  = {"split hero", "reversed split hero", "centered hero", "oversized-type hero"};
const char* FONTS[] = {"system-ui,-apple-system,'Segoe UI',sans-serif", "Georgia,'Times New Roman',serif",
                       "'Trebuchet MS',Verdana,sans-serif"};

std::string fmt(const char* f, ...) {
  char buf[1024];
  va_list ap; va_start(ap, f);
  vsnprintf(buf, sizeof buf, f, ap);
  va_end(ap);
  return buf;
}

struct Rng {  // xorshift64: tiny, fast, deterministic
  uint64_t s;
  explicit Rng(uint32_t seed) : s((uint64_t)seed * 0x9E3779B97F4A7C15ULL + 0x1234567ULL) { next(); next(); }
  uint32_t next() { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return (uint32_t)(s >> 16); }
  double f() { return (next() & 0xFFFFFF) / 16777216.0; }
  int irange(int a, int b) { return a + (int)(f() * (b - a + 1)); }
};

struct Layout {
  int nav, hero, cols, card, radius, space, width, hue, hue2, font;
  bool dark, stats, quote, footCols;
};

Layout make(uint32_t seed) {
  Rng r(seed);
  Layout d;
  d.nav = r.irange(0, 2);   d.hero = r.irange(0, 3);  d.cols = r.irange(2, 4);   d.card = r.irange(0, 1);
  d.radius = r.irange(0, 5) * 4;  d.space = r.irange(48, 112);  d.width = 960 + 40 * r.irange(0, 8);
  d.hue = r.irange(0, 359);  d.hue2 = (d.hue + r.irange(30, 180)) % 360;  d.font = r.irange(0, 2);
  d.dark = r.f() < 0.5;  d.stats = r.f() < 0.5;  d.quote = r.f() < 0.5;  d.footCols = r.f() < 0.5;
  return d;
}

std::string vars(const Layout& d, bool dark) {
  std::string bg = dark ? fmt("hsl(%d,25%%,9%%)", d.hue) : fmt("hsl(%d,40%%,97%%)", d.hue);
  std::string fg = dark ? "#eef1f6" : "#18202b";
  std::string sf = dark ? fmt("hsl(%d,22%%,14%%)", d.hue) : "#ffffff";
  std::string ln = dark ? fmt("hsl(%d,18%%,24%%)", d.hue) : fmt("hsl(%d,25%%,86%%)", d.hue);
  return "--bg:" + bg + ";--fg:" + fg + ";--sf:" + sf + ";--ln:" + ln;
}

std::string css(const Layout& d, bool withJs) {
  std::string o = ":root{" + vars(d, d.dark) + ";" +
                  fmt("--ac:hsl(%d,70%%,40%%);--ac2:hsl(%d,75%%,58%%);--r:%dpx;--sp:%dpx;--w:%dpx}\n",
                      d.hue, d.hue2, d.radius, d.space, d.width);
  o += fmt("*{box-sizing:border-box}\nbody{margin:0;background:var(--bg);color:var(--fg);font-family:%s;line-height:1.55}\n", FONTS[d.font]);
  o += ".c{max-width:var(--w);margin:0 auto;padding:0 24px}\n"
       "section{padding:var(--sp) 0}\n"
       "h1{font-size:clamp(34px,5vw,60px);line-height:1.05;margin:0 0 16px}\n"
       "h2{font-size:clamp(26px,3.5vw,38px);margin:0 0 28px}\n"
       "nav .c{display:flex;align-items:center;justify-content:space-between;padding-top:20px;padding-bottom:20px}\n"
       "nav a{margin-left:20px;color:inherit;text-decoration:none}\n"
       "nav.center .c{flex-direction:column;gap:12px}\n"
       "nav.center a{margin:0 10px}\n"
       "nav.pill .c{margin-top:16px;max-width:calc(var(--w) - 80px);background:var(--sf);border:1px solid var(--ln);border-radius:999px;padding:12px 28px}\n"
       ".btn{display:inline-block;background:var(--ac);color:#fff;padding:12px 22px;border-radius:var(--r);text-decoration:none;font-weight:600}\n"
       ".hero .c{display:grid;grid-template-columns:1fr 1fr;gap:48px;align-items:center}\n"
       ".hero.rev .art{order:-1}\n"
       ".hero.mid .c{grid-template-columns:1fr;text-align:center;justify-items:center}\n"
       ".hero.mid .art{width:100%}\n"
       ".hero.big .c{grid-template-columns:1fr}\n"
       ".hero.big h1{font-size:clamp(48px,9vw,112px);line-height:.95}\n"
       ".art{aspect-ratio:4/3;width:100%;border-radius:var(--r);background:linear-gradient(135deg,var(--ac),var(--ac2))}\n";
  o += fmt(".grid{display:grid;grid-template-columns:repeat(%d,1fr);gap:24px}\n", d.cols);
  o += ".card{padding:24px;border-radius:var(--r);background:var(--sf);border:1px solid var(--ln)}\n"
       ".lines .card{background:none;border:0;border-top:2px solid var(--fg);border-radius:0;padding-left:0}\n"
       ".card h3{margin:0 0 6px}.card p{margin:0;opacity:.75}\n"
       ".icon{width:40px;height:40px;border-radius:var(--r);background:var(--ac2);margin-bottom:14px}\n"
       ".stats .c{display:flex;justify-content:space-around;text-align:center;flex-wrap:wrap;gap:24px}\n"
       ".stats b{display:block;font-size:44px;color:var(--ac2)}\n"
       ".quote{font-size:clamp(22px,3vw,34px);max-width:760px;margin:0 auto 12px;text-align:center}\n"
       ".cta .c{background:var(--ac);color:#fff;border-radius:var(--r);padding:56px 24px;text-align:center}\n"
       ".cta h2{margin-bottom:20px}.cta .btn{background:#fff;color:var(--ac)}\n"
       "footer{border-top:1px solid var(--ln);padding:32px 0;font-size:14px;opacity:.85}\n"
       "footer .cols{display:grid;grid-template-columns:repeat(3,1fr);gap:24px}\n"
       "@media(max-width:760px){.hero .c,.grid,footer .cols{grid-template-columns:1fr}nav a{margin-left:12px}}\n";
  if (withJs) {  // styles for the optional JavaScript features
    o += ":root[data-theme=dark]{" + vars(d, true) + "}\n:root[data-theme=light]{" + vars(d, false) + "}\n";
    o += "nav .c{flex-wrap:wrap}\n"
         ".tools{display:flex;gap:8px;align-items:center}\n"
         ".tools button{font:inherit;background:none;border:1px solid var(--ln);color:inherit;border-radius:var(--r);padding:6px 12px;cursor:pointer}\n"
         "a:focus-visible,button:focus-visible{outline:3px solid var(--ac2);outline-offset:2px}\n"
         ".menu{display:none}\n"
         ".hide{opacity:0;transform:translateY(16px);transition:opacity .5s,transform .5s}\n"
         ".hide.in{opacity:1;transform:none}\n"
         "@media(max-width:760px){.menu{display:inline-block}.links{display:none;width:100%;order:3}"
         ".links.open{display:flex;flex-direction:column;gap:12px;padding-bottom:12px}.links a{margin:0}}\n"
         "@media(prefers-reduced-motion:reduce){.hide{opacity:1;transform:none;transition:none}}\n";
  }
  return o;
}

std::string body(const Layout& d, bool withJs) {
  const char* navCls[]  = {"", "center", "pill"};
  const char* heroCls[] = {"", "rev", "mid", "big"};
  std::string o = fmt("<nav class=\"%s\"><div class=\"c\"><strong>Brandname</strong><div class=\"links\"><a href=\"#\">Product</a>"
                      "<a href=\"#\">Pricing</a><a href=\"#\">About</a><a href=\"#\" class=\"btn\">Sign up</a></div>%s</div></nav>\n",
                      navCls[d.nav],
                      withJs ? "<div class=\"tools\"><button class=\"theme\" aria-label=\"Toggle light or dark theme\">Theme</button>"
                               "<button class=\"menu\" aria-label=\"Open menu\" aria-expanded=\"false\">Menu</button></div>" : "");
  o += fmt("<section class=\"hero %s\"><div class=\"c\"><div><h1>Build something people remember</h1>"
           "<p>A short line that explains what you do and who it is for.</p><a href=\"#\" class=\"btn\">Get started</a></div>%s</div></section>\n",
           heroCls[d.hero], d.hero == 3 ? "" : "<div class=\"art\"></div>");
  if (d.stats)
    o += "<section class=\"stats\"><div class=\"c\"><div><b data-to=\"12\" data-suffix=\"k\">12k</b>customers</div><div><b data-to=\"99.9\" data-suffix=\"%\">99.9%</b>uptime</div><div><b data-to=\"4.8\">4.8</b>average rating</div></div></section>\n";
  int n = d.cols == 4 ? 4 : d.cols * 2;
  o += fmt("<section><div class=\"c\"><h2>What you get</h2><div class=\"grid%s\">\n", d.card ? " lines" : "");
  for (int i = 1; i <= n; i++)
    o += fmt("<div class=\"card\"><div class=\"icon\"></div><h3>Feature %d</h3><p>One sentence on the benefit.</p></div>\n", i);
  o += "</div></div></section>\n";
  if (d.quote)
    o += "<section><div class=\"c\"><p class=\"quote\">“This changed how our whole team ships.”</p><p style=\"text-align:center;opacity:.7\">Name, Company</p></div></section>\n";
  o += "<section class=\"cta\"><div class=\"c\"><h2>Ready to start?</h2><a href=\"#\" class=\"btn\">Create your account</a></div></section>\n";
  o += d.footCols ? "<footer><div class=\"c cols\"><div><strong>Brandname</strong><br>Short tagline.</div><div>Product<br>Pricing<br>Docs</div><div>Company<br>Contact<br>Careers</div></div></footer>\n"
                  : "<footer><div class=\"c\">© Brandname. All rights reserved.</div></footer>\n";
  return o;
}

const char* JS_CORE = R"JS(
const $ = (s, r = document) => r.querySelector(s);
const $$ = (s, r = document) => [...r.querySelectorAll(s)];

// 1. Mobile menu
const menu = $('.menu'), links = $('.links');
menu.addEventListener('click', () => {
  const open = links.classList.toggle('open');
  menu.setAttribute('aria-expanded', open);
});
$$('.links a').forEach(a => a.addEventListener('click', () => {
  links.classList.remove('open');
  menu.setAttribute('aria-expanded', false);
}));

// 2. Light/dark theme toggle (remembers the choice)
const root = document.documentElement;
try { const t = localStorage.getItem('theme'); if (t) root.dataset.theme = t; } catch (e) {}
$('.theme').addEventListener('click', () => {
  const next = (root.dataset.theme || (DEFAULT_DARK ? 'dark' : 'light')) === 'dark' ? 'light' : 'dark';
  root.dataset.theme = next;
  try { localStorage.setItem('theme', next); } catch (e) {}
});

// 3. Fade cards in as they scroll into view
const reveal = new IntersectionObserver(entries => entries.forEach(e => {
  if (e.isIntersecting) { e.target.classList.add('in'); reveal.unobserve(e.target); }
}), { threshold: 0.15 });
$$('.card').forEach(c => { c.classList.add('hide'); reveal.observe(c); });
)JS";

const char* JS_STATS = R"JS(
// 4. Count the numbers up when they appear
$$('[data-to]').forEach(el => {
  const to = +el.dataset.to, dec = (el.dataset.to.split('.')[1] || '').length, suf = el.dataset.suffix || '';
  new IntersectionObserver(([e], o) => {
    if (!e.isIntersecting) return;
    o.disconnect();
    const t0 = performance.now();
    const step = t => {
      const p = Math.min((t - t0) / 1200, 1);
      el.textContent = (to * p).toFixed(dec) + suf;
      if (p < 1) requestAnimationFrame(step);
    };
    requestAnimationFrame(step);
  }).observe(el);
});
)JS";

std::string js(const Layout& d, uint32_t seed) {
  std::string o = fmt("// Layout #%u behavior. Use together with the exported HTML and CSS.\nconst DEFAULT_DARK = %s;\n",
                      seed, d.dark ? "true" : "false");
  o += JS_CORE;
  if (d.stats) o += JS_STATS;
  return o;
}

// withJs: add menu/theme buttons + script. linked: reference style.css / script.js instead of inlining.
std::string page(const Layout& d, uint32_t seed, bool withJs, bool linked) {
  std::string o = "<!doctype html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n"
                  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n<title>Brandname</title>\n";
  o += linked ? std::string("<link rel=\"stylesheet\" href=\"style.css\">\n")
              : "<style>\n" + css(d, withJs) + "</style>\n";
  o += "</head>\n<body>\n" + body(d, withJs);
  if (withJs)
    o += linked ? std::string("<script src=\"script.js\"></script>\n")
                : "<script>\n" + js(d, seed) + "</script>\n";
  o += "</body>\n</html>\n";
  return o;
}

// ---- The small AI: online logistic regression over layout features ----
const int NF = 17;
std::vector<double> W(NF, 0.0);
int nLikes = 0, nDislikes = 0;

std::vector<double> feat(const Layout& d) {
  std::vector<double> x(NF, 0.0);
  x[d.nav] = 1;  x[3 + d.hero] = 1;
  x[7] = (d.cols - 2) / 2.0;  x[8] = d.card;  x[9] = d.radius / 20.0;  x[10] = (d.space - 48) / 64.0;
  x[11] = d.dark;  x[12] = d.stats;  x[13] = d.quote;  x[14] = d.footCols;  x[15] = d.font == 1;
  x[16] = 1;
  return x;
}

double score(const std::vector<double>& x) {
  double s = 0; for (int i = 0; i < NF; i++) s += W[i] * x[i]; return s;
}

std::string out;  // buffer for strings returned to JS

}  // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE const char* render_page(uint32_t seed, int withJs, int linked) { out = page(make(seed), seed, withJs, linked); return out.c_str(); }
EMSCRIPTEN_KEEPALIVE const char* export_css(uint32_t seed, int withJs) { out = css(make(seed), withJs); return out.c_str(); }
EMSCRIPTEN_KEEPALIVE const char* export_js(uint32_t seed) { out = js(make(seed), seed); return out.c_str(); }

EMSCRIPTEN_KEEPALIVE const char* describe(uint32_t seed) {
  Layout d = make(seed);
  out = fmt("%s, %s, %d-column features, %s", NAV[d.nav], HERO[d.hero], d.cols, d.dark ? "dark" : "light");
  return out.c_str();
}

EMSCRIPTEN_KEEPALIVE void ai_learn(uint32_t seed, int liked) {
  std::vector<double> x = feat(make(seed));
  double p = 1.0 / (1.0 + exp(-score(x)));
  for (int i = 0; i < NF; i++) W[i] += 0.8 * ((liked ? 1.0 : 0.0) - p) * x[i];
  liked ? nLikes++ : nDislikes++;
}

// Samples 400 candidate layouts and returns the one the model likes best (plus a little noise to explore).
EMSCRIPTEN_KEEPALIVE uint32_t ai_suggest(uint32_t counter) {
  Rng r(counter * 7919u + 13u);
  uint32_t best = r.next() & 0x7FFFFFFF; double bestScore = -1e9;
  for (int i = 0; i < 400; i++) {
    uint32_t s = r.next() & 0x7FFFFFFF;
    double sc = score(feat(make(s))) + 0.3 * r.f();
    if (sc > bestScore) { bestScore = sc; best = s; }
  }
  return best;
}

EMSCRIPTEN_KEEPALIVE const char* ai_report() {
  out = fmt("Learned from %d likes and %d skips.", nLikes, nDislikes);
  if (nLikes + nDislikes < 3) { out += " Rate a few layouts and I will start suggesting ones you may like."; return out.c_str(); }
  int bn = 0; for (int i = 1; i < 3; i++) if (W[i] > W[bn]) bn = i;
  int bh = 0; for (int i = 1; i < 4; i++) if (W[3 + i] > W[3 + bh]) bh = i;
  out += fmt(" Leaning toward a %s with a %s", NAV[bn], HERO[bh]);
  if (W[11] > 0.2) out += ", dark theme"; else if (W[11] < -0.2) out += ", light theme";
  if (W[9] > 0.2) out += ", rounded corners"; else if (W[9] < -0.2) out += ", sharp corners";
  out += ".";
  return out.c_str();
}

}
