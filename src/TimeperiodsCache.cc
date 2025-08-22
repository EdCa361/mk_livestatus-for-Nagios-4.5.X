// Copyright (C) 2023 Checkmk GmbH - Licencia: GNU General Public License v2
// Este archivo forma parte de Checkmk (https://checkmk.com). Está sujeto a los
// términos y condiciones definidos en el archivo COPYING, que es parte de este
// paquete de código fuente.
#include "TimeperiodsCache.h"
#include <compare>
#include <utility>
#include "Logger.h"
using namespace std::chrono_literals;

// Variable global que contiene la lista de todos los periodos de tiempo
extern timeperiod *timeperiod_list;

TimeperiodsCache::TimeperiodsCache(Logger *logger) : _logger(logger) {}

void TimeperiodsCache::logCurrentTimeperiods() {
    const std::lock_guard<std::mutex> lg(_mutex);
    // Recorre todos los periodos de tiempo y calcula si actualmente estamos dentro.
    // Detecta el caso en que no se conocen periodos de tiempo (¡aún!).
    // Esto podría ocurrir cuando un mensaje del broker llega *antes* del inicio
    // del bucle de eventos.
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    
    for (timeperiod *tp = timeperiod_list; tp != nullptr; tp = tp->next) {
        const bool is_in = check_time_against_period(now, tp) == 0;
        
        // CORRECCIÓN: Acceso seguro al caché
        auto it = _cache.find(tp);
        if (it == _cache.end()) {  // primer ingreso
            logTransition(tp->name, -1, is_in ? 1 : 0);
            _cache.emplace(tp, is_in);
        } else if (it->second != is_in) {  // Solo registrar transiciones
            logTransition(tp->name, it->second ? 1 : 0, is_in ? 1 : 0);
            it->second = is_in;
        }
    }
}

void TimeperiodsCache::update(std::chrono::system_clock::time_point now) {
    const std::lock_guard<std::mutex> lg(_mutex);
    // Actualiza la caché solo una vez por minuto. Las definiciones de periodos
    // de tiempo tienen granularidad de 1 minuto, por lo que no se necesita
    // resolución de 1 segundo.
    if (now < _last_update + 1min) {
        return;
    }
    _last_update = now;
    
    bool had_timeperiods = !_cache.empty();
    
    // Recorre todos los periodos de tiempo y calcula si actualmente estamos dentro.
    // Detecta el caso en que no se conocen periodos de tiempo (¡aún!).
    // Esto podría ocurrir cuando un mensaje del broker llega *antes* del inicio
    // del bucle de eventos.
    for (timeperiod *tp = timeperiod_list; tp != nullptr; tp = tp->next) {
        const bool is_in = check_time_against_period(
            std::chrono::system_clock::to_time_t(now), tp) == 0;
        
        // Verifica el estado anterior y registra la transición si el estado cambió
        auto it = _cache.find(tp);
        if (it == _cache.end()) {  // primer ingreso
            logTransition(tp->name, -1, is_in ? 1 : 0);
            _cache.emplace(tp, is_in);
        } else if (it->second != is_in) {
            logTransition(tp->name, it->second ? 1 : 0, is_in ? 1 : 0);
            it->second = is_in;
        }
    }
    
    // CORRECCIÓN: Mensaje de log más preciso
    if (_cache.empty() && had_timeperiods) {
        Informational(_logger) << "Se han eliminado todos los periodos de tiempo de la configuración";
    } else if (_cache.empty()) {
        Debug(_logger) << "Caché de periodos de tiempo actualizada, pero no hay periodos definidos";
    }
}

bool TimeperiodsCache::inTimeperiod(const std::string &tpname) const {
    for (timeperiod *tp = timeperiod_list; tp != nullptr; tp = tp->next) {
        if (tpname == tp->name) {
            return inTimeperiod(tp);
        }
    }
    return true;  // periodo desconocido se asume siempre activo (24x7)
}

bool TimeperiodsCache::inTimeperiod(const timeperiod *tp) const {
    // CORRECCIÓN: Manejo consistente de punteros nulos
    if (tp == nullptr) {
        return true;  // periodo desconocido se asume siempre activo (24x7)
    }
    
    const std::lock_guard<std::mutex> lg(_mutex);
    auto it = _cache.find(tp);
    if (it == _cache.end()) {
        Warning(_logger) << "No hay información disponible para el periodo " << tp->name 
                         << ". Se asume que está activo (24x7).";
        return true;  // Consistente con el comportamiento para nombres desconocidos
    }
    return it->second;
}

void TimeperiodsCache::logTransition(char *name, int from, int to) const {
    Informational(_logger) << "TRANSICIÓN DE PERIODO DE TIEMPO: " << name << ";" << from
                           << ";" << to;
}
