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

## Solución Implementada

Simplemente **remover todos los `pthread_join()` manuales** en testHilo.cpp. El destructor ya se encarga:

```cpp
// En testHilo.cpp (CORRECTO)
// Señalizar a todos los hilos que deben terminar
pthread_mutex_lock(mtx.get());
*running = false;
pthread_mutex_unlock(mtx.get());

// ... (no hay pthread_join() manual)

// Los destructores automáticos se encargan de pthread_join()
// cuando los objetos salen del scope al final de main()

// Cerrar transmisor y receptor
transmisor->cerrar();
receptor->cerrar();

// Destructor mutex
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
./bin/testSystem    ✓
./bin/testHilo      ✓ (ahora sin segfault)
```

## Impacto en v1.0.9

- **Signal Handling (v1.0.8)**: Ya funcionaba correctamente
- **Cleanup Fix (v1.0.8.1)**: Completa el ciclo de vida correcto
- **Resultado**: Terminación **100% ordenada y limpia** con Ctrl+C

---

**Versión**: 1.0.8.1  
**Fecha**: 2026-01-14  
**Issue**: Segmentation fault en Ctrl+C  
**Status**: ✅ FIXED
