#include "include/lfm_signal_generator.h"
#include <iostream>
#include <iomanip>

int main() {
    using namespace radar;
    
    // Создаем параметры для генератора ЛЧМ сигнала
    LFMParameters lfm_params;
    lfm_params.f_start = 100.0f;
    lfm_params.f_stop = 500.0f;
    lfm_params.sample_rate = 8000.0f;
    lfm_params.duration = 1.0f;
    lfm_params.num_beams = 256;
    
    // Создаем генератор
    LFMSignalGenerator generator(lfm_params);
    
    // Параметры шума
    NoiseParams noise_params;
    noise_params.fd = 8000.0;      // Частота дискретизации
    noise_params.f0 = 100.0;       // Начальная частота
    noise_params.a = 1.0;          // Амплитуда сигнала
    noise_params.an = 0.1;         // Амплитуда шума
    noise_params.ti = 1.0;         // Длительность
    noise_params.phi = 0.0;        // Начальная фаза
    noise_params.fdev = 400.0;     // Девиация частоты
    noise_params.tau = 0.0;        // Временной сдвиг
    
    // Генерируем сигнал с шумом
    auto [signal, time] = generator.GetSignalWithNoise(noise_params);
    
    // Выводим информацию
    std::cout << "Generated signal with " << signal.size() << " samples\n";
    std::cout << "Time vector size: " << time.size() << "\n";
    
    // Выводим первые 5 значений
    std::cout << "\nFirst 5 samples:\n";
    for (size_t i = 0; i < 5 && i < signal.size(); ++i) {
        std::cout << "t[" << i << "] = " << std::fixed << std::setprecision(6) << time[i]
                  << ", signal[" << i << "] = (" << signal[i].real() << ", " << signal[i].imag() << ")\n";
    }
    
    std::cout << "\nSuccess!\n";
    return 0;
}