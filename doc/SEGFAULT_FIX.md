# Fix: Segmentation Fault en Ctrl+C (v1.0.8.1)

## Problema Reportado

Al presionar **Ctrl+C** en `testHilo`, el programa terminaba con:

```
Transmisor: Cerrado correctamente
Receptor: Cerrado correctamente
Segmentation fault (core dumped)
```

## Causa Raíz

**Double `pthread_join()`** - El problema estaba en `test/testHilo.cpp`:

1. El código estaba haciendo `pthread_join()` **manualmente** para cada hilo:
```cpp
// En testHilo.cpp (INCORRECTO)
pthread_join(hiloRef.getThread(), nullptr);
pthread_join(hiloPlanta.getThread(), nullptr);
pthread_join(hiloAD.getThread(), nullptr);
pthread_join(hiloPID.getThread(), nullptr);
pthread_join(hiloDA.getThread(), nullptr);
pthread_join(hiloSumador.getThread(), nullptr);
pthread_join(hiloTransmisor.getThread(), nullptr);
pthread_join(hiloReceptor.getThread(), nullptr);
```

2. **Pero** todos los destructores de Hilo ya estaban haciendo `pthread_join()`:
```cpp
// En Hilo.cpp (y todos los demás hilos)
Hilo::~Hilo() {
    int ret = pthread_join(thread_, nullptr);  // ← AQUÍ
    if (ret != 0) {
        std::cerr << "[Hilo::~Hilo] Error: pthread_join falló con código " << ret << std::endl;
    }
}
```

3. Cuando `main()` termina, los objetos se destruyen automáticamente (scope exit):
   - `hiloRef` → destructor llama `pthread_join()`
   - `hiloPlanta` → destructor llama `pthread_join()`
   - etc.

4. **Resultado**: `pthread_join()` se llamaba **dos veces** por thread:
   - Una en el código del main (línea 301-308)
   - Otra en el destructor (línea 59 de Hilo.cpp, etc.)

5. Llamar `pthread_join()` en un thread que ya fue joined causa **undefined behavior** → **segmentation fault**

## Solución Implementada (v1.0.8.1 + v1.0.10)

### v1.0.8.1: Remover `pthread_join()` manuales en testHilo.cpp

Simplemente **remover todos los `pthread_join()` manuales**. El destructor ya se encarga:

```cpp
// En testHilo.cpp (CORRECTO)
// Señalizar a todos los hilos que deben terminar
pthread_mutex_lock(mtx.get());
*running = false;
pthread_mutex_unlock(mtx.get());

// Los threads se limpian automáticamente cuando los objetos salen del scope.
// NO hacer pthread_join() manual para evitar double-join que causa segfault.

// Destructor mutex
pthread_mutex_destroy(mtx.get());

return 0;
```

### v1.0.10: Remover llamadas redundantes y actualizar testSystem.cpp

**Cambios adicionales**:
1. **Removidas llamadas manuales a `cerrar()`**: Los destructores de Transmisor/Receptor ya llaman a `cerrar()` automáticamente
2. **testSystem.cpp también corregido**: Tenía el mismo problema de `pthread_join()` manual (líneas 320-323)
3. **Mensajes de cierre clarificados**:
   - `HiloTransmisor`/`HiloReceptor`: "Hilo X: Cerrado correctamente" (pthread_join)
   - `Transmisor`/`Receptor`: "Cola X: Cerrado correctamente" (mqueue close)

```cpp
// testSystem.cpp (v1.0.10 - CORRECTO)
// Los threads se limpian automáticamente cuando los objetos salen del scope.
// NO hacer pthread_join() manual para evitar double-join que causa segfault.
// Transmisor y receptor se cierran automáticamente en sus destructores.

pthread_mutex_destroy(mtx.get());
return 0;
```

## Diagrama del Flujo INCORRECTO (v1.0.8)

```
main():
  ├─ Crear objetos Hilo (hiloRef, hiloPlanta, ...)
  │
  ├─ Bucle principal
  │  └─ Detecta running=false → sale del bucle
  │
  ├─ pthread_join(hiloRef)  ✓ OK
  ├─ pthread_join(hiloPlanta)  ✓ OK
  ├─ ...
  │
  └─ FIN DE MAIN (scope exit)
      ├─ ~hiloRef()
      │   └─ pthread_join(thread_) ✗ ERROR: thread ya fue joined
      │       → Segmentation fault
      ├─ ~hiloPlanta()
      │   └─ pthread_join(thread_) ✗ ERROR: thread ya fue joined
      │       → Segmentation fault
```

## Diagrama del Flujo CORRECTO (v1.0.8.1)

```
main():
  ├─ Crear objetos Hilo (hiloRef, hiloPlanta, ...)
  │
  ├─ Bucle principal
  │  └─ Detecta running=false → sale del bucle
  │
  ├─ NO hay pthread_join() manual
  │
  └─ FIN DE MAIN (scope exit)
      ├─ ~hiloRef()
      │   └─ pthread_join(thread_) ✓ OK: hilo termina ordadamente
      ├─ ~hiloPlanta()
      │   └─ pthread_join(thread_) ✓ OK: hilo termina ordadamente
      ├─ ~hiloAD()
      │   └─ pthread_join(thread_) ✓ OK
      ├─ ~hiloPID()
      │   └─ pthread_join(thread_) ✓ OK
      ├─ ~hiloDA()
      │   └─ pthread_join(thread_) ✓ OK
      ├─ ~hiloSumador()
      │   └─ pthread_join(thread_) ✓ OK
      ├─ ~hiloTransmisor()
      │   └─ pthread_join(thread_) ✓ OK
      ├─ ~hiloReceptor()
      │   └─ pthread_join(thread_) ✓ OK
      │
      └─ Destrucción de mutexes, shared_ptr, etc.
```

## Cambios Realizados

| Archivo | Cambio |
|---------|--------|
| `test/testHilo.cpp` | Removidas 8 líneas de `pthread_join()` manual (L301-308) |
| Compilación | 100% exitosa, sin cambios en headers |

## Testing

### Antes (v1.0.8)
```bash
$ ./bin/testHilo
k=269 | ...
Transmisor: Cerrado correctamente
Receptor: Cerrado correctamente
Segmentation fault (core dumped)  ✗
```

### Después (v1.0.8.1)
```bash
$ ./bin/testHilo
k=301 | ...
Transmisor: Cerrado correctamente
Receptor: Cerrado correctamente
[Normal exit, exit code 0]  ✓
```

## Por qué esto NO rompió otros tests

Otros tests como `testSystem.cpp`, `testPID.cpp`, etc., probablemente no tenían el problema porque:
1. No crean tantos Hilo objects
2. O no llamaban `pthread_join()` manualmente
3. O el número menor de threads hacía que no se manifestara el bug

## Lecciones Aprendidas

### ✓ Buena Práctica: RAII para Threads
Los destructores de Hilo siguen el principio **RAII (Resource Acquisition Is Initialization)**:
- Constructor: crea thread con `pthread_create()`
- Destructor: limpia thread con `pthread_join()`

### ✓ NO hacer cleanup manual si RAII lo hace
Si un destructor ya gestiona un recurso (como `pthread_join()`), **no hacer cleanup manual** del mismo recurso en el caller.

### ✓ El patrón correcto:
```cpp
// INCORRECTO:
{
    Hilo h(...);
    // ... usar h ...
    pthread_join(h.getThread(), nullptr);  // ✗ Don't do this if ~Hilo() does it
}
// ~Hilo() llama pthread_join() de nuevo → Double join → Segfault

// CORRECTO:
{
    Hilo h(...);
    // ... usar h ...
}
// ~Hilo() llama pthread_join() automáticamente al salir del scope → ✓ OK
```

## Verificación de Otros Tests

```bash
cd build && make -j4
# ✅ 14 tests compilados sin errores

# Verificar que otros tests siguen funcionando:
./bin/testPID       ✓
./bin/testTF        ✓
./bin/testSS        ✓
./bin/testSystem    ✓ (v1.0.10: corregido double-join)
./bin/testHilo      ✓ (v1.0.8.1: sin segfault, v1.0.10: mensajes clarificados)
```

## Salida de Cierre Esperada (v1.0.10)

```
Hilo Receptor: Cerrado correctamente      ← pthread_join del wrapper
Cola Receptor: Cerrado correctamente      ← mqueue close del objeto IPC
Hilo Transmisor: Cerrado correctamente    ← pthread_join del wrapper
Cola Transmisor: Cerrado correctamente    ← mqueue close del objeto IPC
Sumador: Cerrado correctamente
hiloDA: Cerrado correctamente
hiloPID: Cerrado correctamente
hiloAD: Cerrado correctamente
hiloPlanta: Cerrado correctamente
hiloRef: Cerrado correctamente
hiloInterruptor: Cerrado correctamente
```

## Impacto en v1.0.9 y v1.0.10

- **Signal Handling (v1.0.8)**: Ya funcionaba correctamente
- **Cleanup Fix (v1.0.8.1)**: Completa el ciclo de vida correcto
- **Signal Refactoring (v1.0.9)**: Separación modular de headers
- **Closure Clarity (v1.0.10)**: Mensajes diferenciados para thread vs IPC queue
- **Resultado**: Terminación **100% ordenada y limpia** con Ctrl+C, mensajes de diagnóstico claros

---

**Versión**: 1.0.8.1 → 1.0.10  
**Fecha**: 2026-01-14  
**Issue**: Segmentation fault en Ctrl+C + Mensajes duplicados confusos  
**Status**: ✅ FIXED
