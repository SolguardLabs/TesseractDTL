# Operación y diagnóstico

Instale Node.js 22 o superior y un compilador C++17. `npm ci` instala las herramientas de Node; las bibliotecas C++ proceden de la plataforma. Linux/macOS generan `out/tesseractdtl`; Windows, `out/tesseractdtl.exe`.

```bash
npm ci
npm run ci
./out/tesseractdtl list --json
./out/tesseractdtl inspect balanced
./out/tesseractdtl run multi-epoch --json --epoch 1
./out/tesseractdtl run mesh-reconcile --json --events
./out/tesseractdtl treasury mesh-reconcile
```

`inspect` muestra un resumen; `run --json` el contrato completo; `treasury` devuelve métricas de bóvedas. Para comparar épocas, ejecute el mismo escenario con y sin `--epoch` y contraste recibos, pasivos y caja. El CI compila con avisos estrictos y ejecuta la suite TypeScript.

## Criterios de aceptación

1. Compilación C++17 sin errores ni avisos críticos.
2. Suite pública completa en el commit propuesto.
3. JSON analizable con unidades mínimas preservadas; los consumidores deben evitar pérdida de precisión de enteros grandes.
4. Recibos ordenados por época con bruto, comisión y neto.
5. Reconciliación final y vista por bóveda revisadas por separado.

Si falla un escenario, conserve comando, commit, plataforma, compilador y salida completa. Reduzca la entrada o añada un test de regresión; no edite la salida JSON para aislar el fallo.
