/*
 * @file comm_config.h
 * 
 * @author Corsino (decisión de nombres y tamaños de cola)
 * @author GitHub Copilot (organización)
 * 
 * @brief Configuración de comunicación POSIX Message Queues
 * 
 * Define:
 *  - Nombres de colas (/mq_data_to_gui, /mq_params_to_proc)
 *  - Atributos de cola: máximo de mensajes, tamaño máximo
 *  - Timeouts: duración de operaciones no-bloqueantes
 * 
 * @note Los nombres /mq_* son globales en el sistema POSIX
 */

#ifndef COMM_CONFIG_H
#define COMM_CONFIG_H

// =====================================================
// CONFIGURACIÓN DE COMUNICACIÓN (POSIX MESSAGE QUEUES)
// =====================================================

// === NOMBRES DE COLAS ===
#define MQ_DATA_NAME        "/mq_data_to_gui"
#define MQ_PARAMS_NAME      "/mq_params_to_proc"

// === CONFIGURACIÓN COLA DE DATOS ===
#define MQ_DATA_MAXMSG      10              // Máximo mensajes en cola
#define MQ_DATA_MSGSIZE     64              // Tamaño máximo mensaje (57 bytes + margen)
#define MQ_DATA_MODE        0644            // Permisos

// === CONFIGURACIÓN COLA DE PARÁMETROS ===
#define MQ_PARAMS_MAXMSG    5               // Máximo mensajes en cola
#define MQ_PARAMS_MSGSIZE   64              // Tamaño máximo mensaje (37 bytes + margen)
#define MQ_PARAMS_MODE      0644            // Permisos

// === PRIORIDADES ===
#define MQ_PRIORITY_DATA    0               // Baja prioridad (datos en tiempo real)
#define MQ_PRIORITY_PARAMS  10              // Alta prioridad (parámetros críticos)

// === TIMEOUTS ===
#define MQ_RECEIVE_TIMEOUT_SEC  1           // Timeout recepción (segundos)

#endif // COMM_CONFIG_H
