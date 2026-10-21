# Arquitectura y ciclo de vida

`Registry` mantiene parámetros y permisos; `Ledger`, cuentas, caja y obligaciones; `RouteBook`, órdenes y planes. `SettlementEngine` obtiene instrucciones de `PlanReducer` y emite recibos por época. `Reconciler` calcula los agregados posteriores. `ScenarioBuilder` genera cuatro conjuntos reproducibles en memoria, sin conexiones persistentes.

```mermaid
sequenceDiagram
    participant C as Cuenta
    participant L as Ledger
    participant R as RouteBook
    participant V as RouteValidator
    participant S as SettlementEngine
    participant Q as Reconciler
    C->>L: Depósito de unidades mínimas
    L-->>R: Obligación asociada a orden
    R->>V: Plan de ruta
    V-->>S: Época admisible
    S->>L: Pago, comisión y cierre
    S-->>Q: Recibo
    Q-->>C: Estado contable
```

La orden avanza `Draft → Deposited → Routed → Settled`. Los eventos recogen altas, depósitos, reducción, ejecución y reconciliación. El CLI adapta el estado a texto o JSON.

| Módulo       | Responsabilidad                                   | Salida               |
| ------------ | ------------------------------------------------- | -------------------- |
| `registry`   | Activos, dominios, operadores, ventanas y bóvedas | Parámetros           |
| `ledger`     | Balances, pasivos, pagos y comisiones             | Saldos               |
| `route`      | Órdenes, tramos y validación                      | Plan admisible       |
| `reducer`    | Agrupación operativa                              | Instrucciones        |
| `settlement` | Aplicación por época                              | Recibos              |
| `reconcile`  | Agregados de conservación                         | Informe              |
| `treasury`   | Cobertura y concentración                         | JSON de solo lectura |
