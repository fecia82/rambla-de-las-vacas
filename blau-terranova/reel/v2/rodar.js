const AQUI = __dirname;
const { chromium } = require('/opt/node22/lib/node_modules/playwright');
const fs = require('fs');
const FPS = 30, DUR = 26.0;
const modo = process.argv[2] || 'previo';
const nom = t => 'prev-' + String(t.toFixed(2)).replace('.', '_') + '.jpg';
(async () => {
  const errs = [];
  const b = await chromium.launch();
  const p = await b.newPage({ viewport:{width:1080,height:1920}, deviceScaleFactor:1 });
  p.on('pageerror', e => errs.push(e.message));
  await p.goto('file://' + AQUI + '/film.html');
  await p.evaluate(() => window.listo);
  await p.waitForTimeout(500);
  if (modo === 'previo') {
    const ts = [0.10, 0.65, 1.30, 3.20, 5.20, 7.40, 9.60, 11.80, 15.80, 19.00, 21.00, 24.60, 25.90];
    for (const t of ts) {
      await p.evaluate(t => window.pinta(t), t);
      await p.waitForTimeout(90);
      await p.screenshot({ path: `${AQUI}/${nom(t)}`, type:'jpeg', quality:88 });
    }
    console.log('previos:', ts.map(nom).join(' '));
  } else {
    fs.rmSync(AQUI + '/frames', { recursive:true, force:true });
    fs.mkdirSync(AQUI + '/frames', { recursive:true });
    const N = Math.round(DUR * FPS);
    const t0 = Date.now();
    for (let f = 0; f < N; f++) {
      await p.evaluate(t => window.pinta(t), f / FPS);
      await p.screenshot({ path: `${AQUI}/frames/f${String(f).padStart(5,'0')}.jpg`, type:'jpeg', quality:93 });
      if (f % 120 === 0) console.log('  fotograma', f, '/', N, '·', Math.round((Date.now()-t0)/1000)+'s');
    }
    console.log('fotogramas:', N, 'en', Math.round((Date.now()-t0)/1000), 's');
  }
  await b.close();
  console.log(errs.length ? 'ERRORES: ' + errs.join(' | ') : 'sin errores');
})();
