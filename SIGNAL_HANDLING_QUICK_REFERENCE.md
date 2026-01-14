# Quick Reference: Signal Handling (v1.0.8)

## Tabla Rápida de Cambios

| Aspecto | Detalles |
|---------|----------|
| **Problema** | Solo HiloIntArranque manejaba SIGINT/SIGTERM; otros threads terminaban abruptamente |
| **Solución** | Bloquear señales en 7 hilos, desbloquear en HiloIntArranque |
| **Técnica** | `pthread_sigmask(SIG_BLOCK/UNBLOCK, ...)` |
| **Threads afectados** | 8 de 8: Hilo, Hilo2in, HiloPID, HiloSignal, HiloSwitch, HiloTransmisor, HiloReceptor, HiloIntArranque |
| **Línea de código** | 1 línea por thread: `bloquear_signals();` o `desbloquear_signals();` al inicio de `run()` |
| **Beneficio principal** | Terminación ordenada garantizada con `running=false` bajo mutex |

## Archivos de Referencia

### Para entender la implementación:
- [SIGNAL_HANDLING.md](SIGNAL_HANDLING.md) - Guía técnica completa
- [ARCHITECTURE.md](ARCHITECTURE.md#terminación-ordenada) - Sección "Terminación ordenada"
- [CHANGELOG.md](CHANGELOG.md#108---2026-01-14) - v1.0.8 cambios

### Para probar:
```bash
# Script automatizado
./test_signal_handling.sh

# Manual con sigint
./bin/testHilo
# En otra terminal: pkill -SIGINT $(pgrep testHilo)
```

## Código Clave

**Bloquear señales (en otros 7 hilos):**
```cpp
bloquear_signals();  // SIGINT, SIGTERM bloqueadas en este thread
```

**Desbloquear para recibir (solo en HiloIntArranque):**
```cpp
desbloquear_signals();  // Puede recibir SIGINT/SIGTERM
```

**Implementación en Hilo.cpp:**
```cpp
void bloquear_signals() {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &set, nullptr);
}

void desbloquear_signals() {
    // ... same pero SIG_UNBLOCK
}
```

## Comportamiento Esperado

**Antes (v1.0.7):**
```
Ctrl+C → Random thread recibe SIGINT → Termina abruptamente → Posible corrupción
```

**Después (v1.0.8):**
```
Ctrl+C → HiloIntArranque recibe SIGINT → g_signal_run=0 → running=false
       → Otros 7 hilos leen running=false → Terminan limpiamente
```

## Testing

### Quick Test (1 minuto):
```bash
./test_signal_handling.sh
# Salida: "✓ Timeout ejecutado (simula Ctrl+C)"
# Salida: "✓ El programa fue detenido limpiamente"
```

### Full Test (5 minutos):
```bash
./bin/testHilo &
PID=$!
sleep 2
kill -SIGINT $PID  # Observar terminación limpia
wait $PID
```

## Checklist de Verificación

- ✓ `include/hilos/Hilo.h` tiene declaraciones de `bloquear_signals()`, `desbloquear_signals()`
- ✓ `src/hilos/Hilo.cpp` tiene implementaciones
- ✓ Los 8 `src/hilos/*.cpp` tienen `#include "hilos/Hilo.h"`
- ✓ Los 7 threads tienen `bloquear_signals();` al inicio de `run()`
- ✓ `HiloIntArranque` tiene `desbloquear_signals();` al inicio de `run()`
- ✓ Compilación: `make -j4` sin errores
- ✓ Runtime: `./bin/testHilo` ejecuta sin problemas
- ✓ Terminación: Ctrl+C causa parada limpia (visible en logs)

## Troubleshooting

| Problema | Causa | Solución |
|----------|-------|----------|
| Error al compilar: undefined reference to `bloquear_signals` | Falta `#include "hilos/Hilo.h"` | Agregar include en .cpp |
| Hilo sigue corriendo después de Ctrl+C | `desbloquear_signals()` falta | Verificar HiloIntArranque::run() |
| Compilación lenta | Rebuild innecesario | `cd build && make clean && make -j4` |

## Performance Impact

- **Startup**: +0 overhead (llamadas ocurren una sola vez en thread creation)
- **Main loop**: Zero overhead (signal masking es per-thread property)
- **Memory**: +0 (no dynamic allocation)

---

**Versión**: 1.0.8  
**Última actualización**: 2026-01-14  
**Estado**: ✅ Production Ready
