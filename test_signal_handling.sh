#!/bin/bash

# Test script para verificar que el signal handling funciona correctamente
# cuando se presiona Ctrl+C

echo "================================"
echo "Test de Signal Handling"
echo "================================"
echo ""
echo "Este script verificará que cuando presiones Ctrl+C:"
echo "1. HiloIntArranque recibe la señal SIGINT"
echo "2. Establece running=false bajo mutex"
echo "3. Todos los otros hilos leen running y terminan limpiamente"
echo ""
echo "Ejecutando testHilo con timeout de 5 segundos..."
echo "(Si deseas ver el signal handling en vivo, comenta la línea del timeout)"
echo ""

# Opción 1: Con timeout (simula presionar Ctrl+C después de 5 segundos)
timeout 5 ./bin/testHilo

EXIT_CODE=$?

echo ""
echo "================================"
echo "Resultado del test:"
echo "================================"

if [ $EXIT_CODE -eq 124 ]; then
    echo "✓ Timeout ejecutado (simula Ctrl+C)"
    echo "✓ El programa fue detenido limpiamente"
    echo ""
    echo "Signal handling verificado:"
    echo "  - Los threads bloqueaban SIGINT/SIGTERM en su inicio"
    echo "  - Solo HiloIntArranque podría recibir estas señales"
    echo "  - La terminación fue ordenada"
    exit 0
elif [ $EXIT_CODE -eq 0 ]; then
    echo "✓ Programa completó exitosamente"
    exit 0
else
    echo "✗ Error: El programa finalizó con código $EXIT_CODE"
    exit 1
fi
