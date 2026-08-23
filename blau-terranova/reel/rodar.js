const { chromium } = require('/opt/node22/lib/node_modules/playwright');
const fs = require('fs');
const SP = process.env.SP, FPS = 30, DUR = 24.0;
const modo = process.argv[2] || 'previo';
(async () => {
  const errs = [];
  const b = await chromium.launch();
  const p = await b.newPage({ viewport:{width:1080,height:1920}, deviceScaleFactor:1 });
  p.on('pageerror', e => errs.push(e.message));
  await p.goto('file://' + SP + '/reel/film.html');
  await p.evaluate(() => window.listo);
  await p.waitForTimeout(500);
  if (modo === 'previo') {
    const ts = [1.6, 3.6, 5.2, 6.6, 9.4, 12.2, 14.8, 17.6, 20.2, 22.4];
    for (const t of ts) {
      await p.evaluate(t => window.pinta(t), t);
      await p.waitForTimeout(90);
      await p.screenshot({ path: `${SP}/reel/prev-${String(t).replace('.','_')}.jpg`, type:'jpeg', quality:88 });
    }
    console.log('previos:', ts.length);
  } else {
    fs.mkdirSync(SP + '/reel/frames', { recursive:true });
    const N = Math.round(DUR * FPS);
    const t0 = Date.now();
    for (let f = 0; f < N; f++) {
      await p.evaluate(t => window.pinta(t), f / FPS);
      await p.screenshot({ path: `${SP}/reel/frames/f${String(f).padStart(5,'0')}.jpg`, type:'jpeg', quality:93 });
      if (f % 90 === 0) console.log('  fotograma', f, '/', N, '·', Math.round((Date.now()-t0)/1000)+'s');
    }
    console.log('fotogramas:', N, 'en', Math.round((Date.now()-t0)/1000), 's');
  }
  await b.close();
  console.log(errs.length ? 'ERRORES: ' + errs.join(' | ') : 'sin errores');
})();
