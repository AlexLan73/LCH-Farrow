#ifndef PROFILING_ENGINE_H
#define PROFILING_ENGINE_H

#include <string>
#include <map>
#include <chrono>
#include <vector>

/**
* @brief Мы должны измерить следующие параметры
*   Загрузка на GPU 
*   Постановка в очередь 
*   ожидание очереди
*   расчет дробного смещения сигнала
*   выгрузка на CPU 
 */


/**
 * @brief Структура для хранения метрики времени
 */
 
struct TimingMetric {
    std::string name;           // Имя метрики
    double time_ms;            // Время в миллисекундах
    size_t call_count;         // Количество вызовов
    double min_time_ms;        // Минимальное время
    double max_time_ms;        // Максимальное время
    double avg_time_ms;        // Среднее время
    
    TimingMetric() : time_ms(0.0), call_count(0), 
                     min_time_ms(0.0), max_time_ms(0.0), avg_time_ms(0.0) {}
};

/**
 * @brief Структура для всех метрик профилирования
 */
struct ProfilingMetrics {
    std::map<std::string, TimingMetric> metrics;
    double total_time_ms;
    
    ProfilingMetrics() : total_time_ms(0.0) {}
};

/**
 * @brief Класс для профилирования производительности
 * 
 * Поддерживает CPU и GPU профилирование.
 * GPU профилирование через OpenCL Events.
 */
class ProfilingEngine {
public:
    /**
     * @brief Конструктор
     */
    ProfilingEngine();
    
    /**
     * @brief Деструктор
     */
    ~ProfilingEngine() = default;
    
    /**
     * @brief Начать измерение времени (CPU)
     * @param name Имя метрики
     */
    void StartTimer(const std::string& name);
    
    /**
     * @brief Остановить измерение времени (CPU)
     * @param name Имя метрики
     */
    void StopTimer(const std::string& name);
    
    /**
     * @brief Записать GPU событие для профилирования (OpenCL)
     * @param event_name Имя события
     * @param time_ms Время в миллисекундах
     */
    void RecordGpuEvent(const std::string& event_name, double time_ms);
    
    /**
     * @brief Вывести отчёт в консоль
     */
    void ReportMetrics() const;
    
    /**
     * @brief Сохранить отчёт в JSON файл
     * @param filename Имя файла
     * @return true если успешно
     */
    bool SaveReportToJson(const std::string& filename) const;
    
    /**
     * @brief Получить метрику по имени
     * @param name Имя метрики
     * @return Константная ссылка на метрику
     */
    const TimingMetric& GetMetric(const std::string& name) const;
    
    /**
     * @brief Получить все метрики
     * @return Константная ссылка на структуру метрик
     */
    const ProfilingMetrics& GetAllMetrics() const { return metrics_; }
    
    /**
     * @brief Сбросить все метрики
     */
    void Reset();
    
    /**
     * @brief Включить/выключить профилирование
     * @param enable true для включения
     */
    void EnableProfiling(bool enable) { profiling_enabled_ = enable; }

private:
    ProfilingMetrics metrics_;
    std::map<std::string, std::chrono::high_resolution_clock::time_point> start_times_;
    bool profiling_enabled_;
    
    /**
     * @brief Обновить статистику метрики
     * @param name Имя метрики
     * @param time_ms Время в миллисекундах
     */
    void UpdateMetricStats(const std::string& name, double time_ms);
};

#endif // PROFILING_ENGINE_H

