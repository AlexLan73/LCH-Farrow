#include <iostream>
#include <memory>
#include <cstring>
#include <vector>
#include "signal_buffer.h"
#include "application.h"
#include "filter_bank.h"
#include "lagrange_matrix.h"
#include "gpu_backend/gpu_factory.h"
#include "profiling_engine.h"
#include "processing_pipeline.h"
#include "lfm_signal_generator.h"
#include "fractional_delay_cpu.h"
#include "result_comparator.h"
//#include "gpu_profiling.h"
//#include "gpu_backend/opencl_backend.h"
#include <iomanip>
#include <ctime>
#include <sstream>
#include <map>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <map>

using namespace radar;

int main(int argc, char* argv[]) {
    Application::Config cfg;
    cfg.f_start = 100.0f;
    cfg.f_stop = 500.0f;
    cfg.sample_rate = 500000.0f;
    cfg.duration = 1.0f;
    cfg.num_beams = 128;
    cfg.steering_angle = 30.0f;

    Application app(cfg);
    return app.Run();
}
