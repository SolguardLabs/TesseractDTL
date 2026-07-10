# Seguridad De TesseractDTL

Este documento resume el modelo de seguridad esperado para TesseractDTL y el
alcance de revision recomendado.

## Modelo De Seguridad

TesseractDTL separa cuatro superficies:

- registro de dominios, assets, operadores, ventanas y bovedas;
- admision de ordenes y deposito de obligaciones;
- validacion de rutas multidimensionales;
- settlement por epoch y reconciliacion posterior.

Los escenarios locales representan un entorno controlado donde todos los inputs
son deterministas. Aun asi, las transiciones economicas se tratan como si
fueran parte de una infraestructura de liquidacion real.

## Invariantes Esperadas

- Una orden solo puede usar un asset registrado.
- Una boveda debe pertenecer al asset y dominio declarados.
- Un operador solo puede operar rutas entre dominios autorizados.
- Una ventana solo admite legs dentro de sus epochs abiertos.
- Los importes y fees no pueden ser negativos.
- El fee de un leg no puede superar el importe bruto.
- Una orden no puede cerrar mas obligacion que la admitida.
- La reconciliacion debe terminar sin obligaciones abiertas en escenarios
  completos.

## Validaciones Automatizadas

La suite TypeScript ejecuta escenarios reales contra el binario compilado y
verifica:

- salida JSON estable;
- rutas y deposits esperados;
- settlement parcial por epoch;
- cierre final de liabilities;
- agregados por operador y asset.

El CI usa `scripts/ci.sh` para compilar y ejecutar tests.

## Gestion De Dependencias

El proyecto no usa dependencias C++ externas. Node se emplea para scripts de
build y tests. Dependabot cubre `npm` y GitHub Actions.

## Alcance De Revision

Puntos de interes para revisiones internas:

- consistencia entre route book y ledger;
- limites de ventana y epoch;
- semantica de reduccion de planes;
- contabilizacion de fees por operador;
- reconciliacion de bovedas con obligaciones abiertas y cerradas.

## Reporte Interno

Un reporte de seguridad debe incluir:

- escenario afectado;
- transicion de estado observada;
- cuentas, bovedas y assets implicados;
- impacto economico;
- reproduccion minima;
- mitigacion propuesta y tests de regresion.
